/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "merrunconfiguration.h"

#include "merconstants.h"
#include "merdeployconfiguration.h"
#include "merrunconfigurationaspect.h"
#include "projectexplorer/kitinformation.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <remotelinux/remotelinuxenvironmentaspect.h>
#include <remotelinux/remotelinuxx11forwardingaspect.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Mer {
namespace Internal {

MerRunConfiguration::MerRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto exeAspect = addAspect<ExecutableAspect>();
    exeAspect->setLabelText(tr("Executable on device:"));
    exeAspect->setExecutablePathStyle(OsTypeLinux);
    exeAspect->setPlaceHolderText(tr("Remote path not set"));
    exeAspect->makeOverridable("Mer.RunConfig.AlternateRemoteExecutable",
                               "Mer.RunConfig.UseAlternateRemoteExecutable");
    exeAspect->setHistoryCompleter("Mer.AlternateExecutable.History");

    auto symbolsAspect = addAspect<SymbolFileAspect>();
    symbolsAspect->setLabelText(tr("Executable on host:"));
    symbolsAspect->setDisplayStyle(SymbolFileAspect::LabelDisplay);

    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>();
    if (HostOsInfo::isAnyUnixHost())
        addAspect<TerminalAspect>();
    addAspect<RemoteLinuxEnvironmentAspect>(target);
    if (HostOsInfo::isAnyUnixHost())
        addAspect<X11ForwardingAspect>();

    setUpdater([this, target, exeAspect, symbolsAspect] {
        BuildTargetInfo bti = buildTargetInfo();
        const FilePath localExecutable = bti.targetFilePath;
        DeployableFile depFile = target->deploymentData().deployableForLocalFile(localExecutable);

        exeAspect->setExecutable(FilePath::fromString(depFile.remoteFilePath()));
        symbolsAspect->setFilePath(localExecutable);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    connect(target, &Target::kitChanged, this, &RunConfiguration::update);
    connect(target, &Target::activeDeployConfigurationChanged,
            this, &MerRunConfiguration::updateEnabledState);
}

QString MerRunConfiguration::disabledReason() const
{
    if (!m_disabledReason.isEmpty())
        return m_disabledReason;

    return RunConfiguration::disabledReason();
}

void MerRunConfiguration::updateEnabledState()
{   
    //TODO Hack

    DeployConfiguration* conf = target()->activeDeployConfiguration();
    if(target()->kit())
    {
        if (conf->id() == MerMb2RpmBuildConfigurationFactory::configurationId()) {
            m_disabledReason = tr("This deployment method does not support run configuration");
            setEnabled(false);
            return;
        }
    }

    RunConfiguration::updateEnabledState();
}

Runnable MerRunConfiguration::runnable() const
{
    Runnable r = RunConfiguration::runnable();
    const auto * const forwardingAspect = aspect<X11ForwardingAspect>();
    if (forwardingAspect)
        r.extraData.insert("Ssh.X11ForwardToDisplay", forwardingAspect->display(macroExpander()));

    // required by qtbase not to direct logs to journald
    // for Qt < 5.4
    r.environment.appendOrSet(QLatin1String("QT_NO_JOURNALD_LOG"), QLatin1String("1"));
    // for Qt >= 5.4
    r.environment.appendOrSet(QLatin1String("QT_LOGGING_TO_CONSOLE"), QLatin1String("1"));

    auto merAspect = aspect<MerRunConfigurationAspect>();
    merAspect->applyTo(&r);

    DeployConfiguration* conf = target()->activeDeployConfiguration();
    QTC_ASSERT(conf, return r);;

    if (conf->id() == MerRsyncDeployConfigurationFactory::configurationId()) {
        QString projectName = target()->project()->displayName();
        r.executable = QLatin1String("/opt/sdk/") + projectName + r.executable;
    }

    if (conf->id() == MerRpmDeployConfigurationFactory::configurationId()) {
        //TODO:
        //r.executable = ...;
    }

    return r;
}

} // Internal
} // Mer
