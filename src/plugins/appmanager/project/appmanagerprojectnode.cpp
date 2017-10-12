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

#include "appmanagerprojectnode.h"
#include "appmanagerproject.h"

#include "../appmanagerconstants.h"

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager {

AppManagerProjectNode::AppManagerProjectNode(const FileName &projectFilePath)
    : ProjectNode(projectFilePath)
{
}

QList<ProjectAction> AppManagerProjectNode::supportedActions(Node *node) const
{
    static const QList<ProjectAction> fileActions = { ProjectAction::Rename,
                                                      ProjectAction::RemoveFile
                                                    };
    static const QList<ProjectAction> folderActions = { ProjectAction::AddNewFile,
                                                        ProjectAction::RemoveFile
                                                      };
    switch (node->nodeType()) {
    case FileNodeType:
        return fileActions;
    case FolderNodeType:
    case ProjectNodeType:
        return folderActions;
    default:
        return ProjectNode::supportedActions(node);
    }
}

bool AppManagerProjectNode::addSubProjects(const QStringList &)
{
    return false;
}

bool AppManagerProjectNode::canAddSubProject(const QString &) const
{
    return false;
}

bool AppManagerProjectNode::removeSubProjects(const QStringList &)
{
    return false;
}

bool AppManagerProjectNode::addFiles(const QStringList &, QStringList *)
{
    return true;
}

bool AppManagerProjectNode::removeFiles(const QStringList &, QStringList *)
{
    return true;
}

bool AppManagerProjectNode::deleteFiles(const QStringList &)
{
    return true;
}

bool AppManagerProjectNode::renameFile(const QString &, const QString &)
{
    return true;
}

} // namespace AppManager
