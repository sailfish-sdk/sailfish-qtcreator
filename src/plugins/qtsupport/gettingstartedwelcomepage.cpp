/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "gettingstartedwelcomepage.h"

#include "exampleslistmodel.h"
#include "screenshotcropper.h"
#include "copytolocationdialog.h"

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/theme/theme.h>
#include <utils/winutils.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/welcomepagehelper.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>

#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTimer>

using namespace Core;
using namespace Utils;

namespace QtSupport {
namespace Internal {

const char C_FALLBACK_ROOT[] = "ProjectsFallbackRoot";

ExamplesWelcomePage::ExamplesWelcomePage(bool showExamples)
    : m_showExamples(showExamples)
{
}

QString ExamplesWelcomePage::title() const
{
    return m_showExamples ? tr("Examples") : tr("Tutorials");
}

int ExamplesWelcomePage::priority() const
{
    return m_showExamples ? 30 : 40;
}

Id ExamplesWelcomePage::id() const
{
    return m_showExamples ? "Examples" : "Tutorials";
}

QString ExamplesWelcomePage::copyToAlternativeLocation(const QFileInfo& proFileInfo, QStringList &filesToOpen, const QStringList& dependencies)
{
    const QString projectDir = proFileInfo.canonicalPath();
    QSettings *settings = ICore::settings();
    CopyToLocationDialog d(ICore::dialogParent());
    d.setSourcePath(projectDir);
    d.setDestinationPath(FilePath::fromVariant(settings->value(QString::fromLatin1(C_FALLBACK_ROOT),
                                         DocumentManager::projectsDirectory().toString())));

    while (QDialog::Accepted == d.exec()) {
        QString exampleDirName = proFileInfo.dir().dirName();
        QString destBaseDir = d.destinationPath().toString();
        settings->setValue(QString::fromLatin1(C_FALLBACK_ROOT), destBaseDir);
        QDir toDirWithExamplesDir(destBaseDir);
        if (toDirWithExamplesDir.cd(exampleDirName)) {
            toDirWithExamplesDir.cdUp(); // step out, just to not be in the way
            QMessageBox::warning(ICore::dialogParent(),
                                 tr("Cannot Use Location"),
                                 tr("The specified location already contains \"%1\" directory. "
                                    "Please specify a valid location.").arg(exampleDirName),
                                 QMessageBox::Ok,
                                 QMessageBox::NoButton);
            continue;
        } else {
            QString error;
            QString targetDir = destBaseDir + QLatin1Char('/') + exampleDirName;
            if (FileUtils::copyRecursively(FilePath::fromString(projectDir),
                    FilePath::fromString(targetDir), &error)) {
                // set vars to new location
                const QStringList::Iterator end = filesToOpen.end();
                for (QStringList::Iterator it = filesToOpen.begin(); it != end; ++it)
                    it->replace(projectDir, targetDir);

                foreach (const QString &dependency, dependencies) {
                    const FilePath targetFile = FilePath::fromString(targetDir)
                            .pathAppended(QDir(dependency).dirName());
                    if (!FileUtils::copyRecursively(FilePath::fromString(dependency), targetFile,
                            &error)) {
                        QMessageBox::warning(ICore::dialogParent(),
                                             tr("Cannot Copy Project"),
                                             error);
                        continue;
                    }
                }


                return targetDir + QLatin1Char('/') + proFileInfo.fileName();
            } else {
                QMessageBox::warning(ICore::dialogParent(), tr("Cannot Copy Project"), error);
                continue;
            }

        }
    }
    return QString();
}

void ExamplesWelcomePage::openProject(const ExampleItem *item)
{
    using namespace ProjectExplorer;
    QString proFile = item->projectPath;
    if (proFile.isEmpty())
        return;

    QStringList filesToOpen = item->filesToOpen;
    if (!item->mainFile.isEmpty()) {
        // ensure that the main file is opened on top (i.e. opened last)
        filesToOpen.removeAll(item->mainFile);
        filesToOpen.append(item->mainFile);
    }

    QFileInfo proFileInfo(proFile);
    if (!proFileInfo.exists())
        return;

#if 0
    // If the Qt is a distro Qt on Linux, it will not be writable, hence compilation will fail
    // Same if it is installed in non-writable location for other reasons
    const bool needsCopy = withNtfsPermissions<bool>([proFileInfo] {
        QFileInfo pathInfo(proFileInfo.path());
        return !proFileInfo.isWritable()
                || !pathInfo.isWritable() /* path of .pro file */
                || !QFileInfo(pathInfo.path()).isWritable() /* shadow build directory */;
    });
    if (needsCopy)
#endif
        proFile = copyToAlternativeLocation(proFileInfo, filesToOpen, item->dependencies);

    // don't try to load help and files if loading the help request is being cancelled
    if (proFile.isEmpty())
        return;
    ProjectExplorerPlugin::OpenProjectResult result = ProjectExplorerPlugin::openProject(proFile);
    if (result) {
        ICore::openFiles(filesToOpen);
        if (result.project()->needsConfiguration() && !item->preferredFeatures.isEmpty())
            result.project()->configureAsExampleProject(Core::Id::fromStringList(item->preferredFeatures));
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
        QUrl docUrl = QUrl::fromUserInput(item->docUrl);
        if (docUrl.isValid())
            HelpManager::showHelpUrl(docUrl, HelpManager::ExternalHelpAlways);
        ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
    } else {
        ProjectExplorerPlugin::showOpenProjectError(result);
    }
}

class ExampleDelegate : public ListItemDelegate
{
public:

    void setShowExamples(bool showExamples) { m_showExamples = showExamples; goon(); }

protected:
    void clickAction(const ListItem *item) const override
    {
        QTC_ASSERT(item, return);
        const auto exampleItem = static_cast<const ExampleItem *>(item);

        if (exampleItem->isVideo)
            QDesktopServices::openUrl(QUrl::fromUserInput(exampleItem->videoUrl));
        else if (exampleItem->hasSourceCode)
            ExamplesWelcomePage::openProject(exampleItem);
        else
            HelpManager::showHelpUrl(QUrl::fromUserInput(exampleItem->docUrl),
                                     HelpManager::ExternalHelpAlways);
    }

    void drawPixmapOverlay(const ListItem *item, QPainter *painter,
                           const QStyleOptionViewItem &option,
                           const QRect &currentPixmapRect) const override
    {
        QTC_ASSERT(item, return);
        const auto exampleItem = static_cast<const ExampleItem *>(item);
        if (exampleItem->isVideo) {
            QFont f = option.widget->font();
            f.setPixelSize(13);
            painter->setFont(f);
            QString videoLen = exampleItem->videoLength;
            painter->drawText(currentPixmapRect.adjusted(0, 0, 0, painter->font().pixelSize() + 3),
                              videoLen, Qt::AlignBottom | Qt::AlignHCenter);
        }
    }

    void adjustPixmapRect(QRect *pixmapRect) const override
    {
        if (!m_showExamples)
            *pixmapRect = pixmapRect->adjusted(6, 20, -6, -15);
    }

    bool m_showExamples = true;
};

class ExamplesPageWidget : public QWidget
{
public:
    ExamplesPageWidget(bool isExamples)
        : m_isExamples(isExamples)
    {
        m_exampleDelegate.setShowExamples(isExamples);
        const int sideMargin = 27;
        static auto s_examplesModel = new ExamplesListModel(this);
        m_examplesModel = s_examplesModel;

        auto filteredModel = new ExamplesListModelFilter(m_examplesModel, !m_isExamples, this);

        auto searchBox = new SearchBox(this);
        m_searcher = searchBox->m_lineEdit;

        auto vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(30, sideMargin, 0, 0);

        auto hbox = new QHBoxLayout;
        if (m_isExamples) {
            m_searcher->setPlaceholderText(ExamplesWelcomePage::tr("Search in Examples..."));

            auto exampleSetSelector = new QComboBox(this);
            exampleSetSelector->setMinimumWidth(GridProxyModel::GridItemWidth);
            exampleSetSelector->setMaximumWidth(GridProxyModel::GridItemWidth);
            ExampleSetModel *exampleSetModel = m_examplesModel->exampleSetModel();
            exampleSetSelector->setModel(exampleSetModel);
            exampleSetSelector->setCurrentIndex(exampleSetModel->selectedExampleSet());
            connect(exampleSetSelector, QOverload<int>::of(&QComboBox::activated),
                    exampleSetModel, &ExampleSetModel::selectExampleSet);
            connect(exampleSetModel, &ExampleSetModel::selectedExampleSetChanged,
                    exampleSetSelector, &QComboBox::setCurrentIndex);

            hbox->setSpacing(17);
            hbox->addWidget(exampleSetSelector);
        } else {
            m_searcher->setPlaceholderText(ExamplesWelcomePage::tr("Search in Tutorials..."));
        }
        hbox->addWidget(searchBox);
        hbox->addSpacing(sideMargin);
        vbox->addItem(hbox);

        m_gridModel.setSourceModel(filteredModel);

        auto gridView = new GridView(this);
        gridView->setModel(&m_gridModel);
        gridView->setItemDelegate(&m_exampleDelegate);
        vbox->addWidget(gridView);

        connect(&m_exampleDelegate, &ExampleDelegate::tagClicked,
                this, &ExamplesPageWidget::onTagClicked);
        connect(m_searcher, &QLineEdit::textChanged,
                filteredModel, &ExamplesListModelFilter::setSearchString);
    }

    int bestColumnCount() const
    {
        return qMax(1, width() / (GridProxyModel::GridItemWidth + GridProxyModel::GridItemGap));
    }

    void resizeEvent(QResizeEvent *ev) final
    {
        QWidget::resizeEvent(ev);
        m_gridModel.setColumnCount(bestColumnCount());
    }

    void onTagClicked(const QString &tag)
    {
        QString text = m_searcher->text();
        m_searcher->setText(text + QString("tag:\"%1\" ").arg(tag));
    }

    const bool m_isExamples;
    ExampleDelegate m_exampleDelegate;
    QPointer<ExamplesListModel> m_examplesModel;
    QLineEdit *m_searcher;
    GridProxyModel m_gridModel;
};

QWidget *ExamplesWelcomePage::createWidget() const
{
    return new ExamplesPageWidget(m_showExamples);
}

} // namespace Internal
} // namespace QtSupport
