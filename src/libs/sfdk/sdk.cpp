/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "sdk_p.h"

#include "asynchronous_p.h"
#include "buildengine_p.h"
#include "sfdkconstants.h"
#include "vboxvirtualmachine_p.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

using namespace Utils;

namespace Sfdk {

/*!
 * \class Sdk
 */

Sdk *Sdk::s_instance = nullptr;

Sdk::Sdk(Options options)
    : d_ptr(std::make_unique<SdkPrivate>(this))
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    Q_D(Sdk);

    qCDebug(lib) << "Initializing SDK. Options:" << options;

    d->options_ = options;
    d->commandQueue_ = std::make_unique<CommandQueue>(this);
    d->buildEngineManager = std::make_unique<BuildEngineManager>(this);

    connect(d->buildEngineManager.get(), &BuildEngineManager::buildEngineAdded,
            this, &Sdk::buildEngineAdded);
    connect(d->buildEngineManager.get(), &BuildEngineManager::aboutToRemoveBuildEngine,
            this, &Sdk::aboutToRemoveBuildEngine);
}

Sdk::~Sdk()
{
    Q_D(Sdk);
    d->commandQueue_->wait();
    s_instance = nullptr;
}

Sdk *Sdk::instance()
{
    return s_instance;
}

void Sdk::enableUpdates()
{
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);
    qCDebug(lib) << "Begin enable udates";
    s_instance->d_func()->updatesEnabled = true;
    emit s_instance->d_func()->enableUpdatesRequested();
    qCDebug(lib) << "End enable updates";
}

bool Sdk::saveSettings(QStringList *errorStrings)
{
    QTC_ASSERT(errorStrings, return false);
    qCDebug(lib) << "Begin save settings";
    emit s_instance->d_func()->saveSettingsRequested(errorStrings);
    qCDebug(lib) << "End save settings. Success:" << errorStrings->isEmpty();
    return errorStrings->isEmpty();
}

// FIXME Describe when this becomes initialized
QString Sdk::installationPath()
{
    return BuildEngineManager::installDir();
}

void Sdk::unusedVirtualMachines(const QObject *context,
        const Functor<const QStringList &, bool> &&functor)
{
    const QPointer<const QObject> context_{context};

    VBoxVirtualMachine::fetchRegisteredVirtualMachines(context,
            [=](const QStringList &registeredVms, bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }

        const QSet<QString> usedVms = VBoxVirtualMachine::usedVirtualMachines().toSet();
        const QStringList unusedVms = Utils::filtered(registeredVms,
                [&usedVms](const QString &vm) { return !usedVms.contains(vm); });

        functor(unusedVms, true);
    });
}

QList<BuildEngine *> Sdk::buildEngines()
{
    return BuildEngineManager::buildEngines();
}

BuildEngine *Sdk::buildEngine(const QString &name)
{
    return BuildEngineManager::buildEngine(name);
}

void Sdk::createBuildEngine(const QString &vmName, const QObject *context,
    const Functor<std::unique_ptr<BuildEngine> &&> &functor)
{
    BuildEngineManager::createBuildEngine(vmName, context, functor);
}

int Sdk::addBuildEngine(std::unique_ptr<BuildEngine> &&buildEngine)
{
    return BuildEngineManager::addBuildEngine(std::move(buildEngine));
}

void Sdk::removeBuildEngine(const QString &name)
{
    BuildEngineManager::removeBuildEngine(name);
}

/*!
 * \class SdkPrivate
 */

SdkPrivate::SdkPrivate(Sdk *q)
    : QObject(q)
{
}

SdkPrivate::~SdkPrivate() = default;

Utils::FileName SdkPrivate::libexecPath()
{
    // See ICore::libexecPath()
    return FileName::fromString(QDir::cleanPath(QCoreApplication::applicationDirPath()
                + '/' + RELATIVE_LIBEXEC_PATH));
}

Utils::FileName SdkPrivate::settingsFile(SettingsScope scope, const QString &basename)
{
    const QString prefix = scope == SessionScope
        ? QString::fromLatin1(Constants::LIB_ID) + '-'
        : QString();
    return settingsLocation(scope).appendPath(prefix + basename);
}

Utils::FileName SdkPrivate::settingsLocation(SettingsScope scope)
{
    static FileName systemLocation;
    static FileName userLocation;
    static FileName sessionLocation;

    FileName *location = nullptr;
    switch (scope) {
    case SystemScope:
        location = &systemLocation;
        break;
    case UserScope:
        location = &userLocation;
        break;
    case SessionScope:
        location = &sessionLocation;
        break;
    }
    Q_ASSERT(location);

    if (!location->isNull())
        return *location;

    QTC_CHECK(!QCoreApplication::organizationName().isEmpty());
    QTC_CHECK(!QCoreApplication::applicationName().isEmpty());

    const QSettings::Scope qscope = scope == SystemScope
        ? QSettings::SystemScope
        : QSettings::UserScope;
    const QString applicationName = scope == SessionScope
        ? QCoreApplication::applicationName()
        : QString::fromLatin1(Constants::LIB_ID);

    QSettings settings(QSettings::IniFormat, qscope, QCoreApplication::organizationName(),
            applicationName);

    // See ICore::userResourcePath()
    QTC_CHECK(settings.fileName().endsWith(".ini"));
    const auto iniInfo = QFileInfo(settings.fileName());
    const QString resourceDir = iniInfo.completeBaseName().toLower();
    *location = FileName::fromString(iniInfo.path() + '/' + resourceDir);

    qCDebug(lib) << "Settings location" << scope << *location;

    return *location;
}

} // namespace Sfdk
