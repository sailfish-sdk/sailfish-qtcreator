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

#pragma once

#include <cpptools/projectpart.h>
#include <cpptools/cppcursorinfo.h>
#include <cpptools/cppsymbolinfo.h>

#include <clangsupport/clangcodemodelconnectionclient.h>
#include <clangsupport/filecontainer.h>
#include <clangsupport/clangcodemodelclientinterface.h>
#include <clangsupport/projectpartcontainer.h>

#include <QFuture>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QTextDocument>
#include <QVector>

#include <functional>

namespace Core {
class IEditor;
class IDocument;
}

namespace ClangBackEnd {
class DocumentAnnotationsChangedMessage;
}

namespace TextEditor {
class TextEditorWidget;
class TextDocument;
}

namespace ClangCodeModel {
namespace Internal {

class ModelManagerSupportClang;

class ClangCompletionAssistProcessor;

class IpcReceiver : public ClangBackEnd::ClangCodeModelClientInterface
{
public:
    IpcReceiver();
    ~IpcReceiver();

    using AliveHandler = std::function<void ()>;
    void setAliveHandler(const AliveHandler &handler);

    void addExpectedCodeCompletedMessage(quint64 ticket, ClangCompletionAssistProcessor *processor);
    void deleteProcessorsOfEditorWidget(TextEditor::TextEditorWidget *textEditorWidget);

    QFuture<CppTools::CursorInfo>
    addExpectedReferencesMessage(quint64 ticket,
                                 QTextDocument *textDocument,
                                 const CppTools::SemanticInfo::LocalUseMap &localUses);
    QFuture<CppTools::SymbolInfo> addExpectedRequestFollowSymbolMessage(quint64 ticket);
    bool isExpectingCodeCompletedMessage() const;

    void reset();

private:
    void alive() override;
    void echo(const ClangBackEnd::EchoMessage &message) override;
    void codeCompleted(const ClangBackEnd::CodeCompletedMessage &message) override;

    void documentAnnotationsChanged(const ClangBackEnd::DocumentAnnotationsChangedMessage &message) override;
    void references(const ClangBackEnd::ReferencesMessage &message) override;
    void followSymbol(const ClangBackEnd::FollowSymbolMessage &message) override;

private:
    AliveHandler m_aliveHandler;
    QHash<quint64, ClangCompletionAssistProcessor *> m_assistProcessorsTable;

    struct ReferencesEntry {
        ReferencesEntry() = default;
        ReferencesEntry(QFutureInterface<CppTools::CursorInfo> futureInterface,
                        QTextDocument *textDocument,
                        const CppTools::SemanticInfo::LocalUseMap &localUses)
            : futureInterface(futureInterface)
            , textDocument(textDocument)
            , localUses(localUses) {}
        QFutureInterface<CppTools::CursorInfo> futureInterface;
        QPointer<QTextDocument> textDocument;
        CppTools::SemanticInfo::LocalUseMap localUses;
    };
    QHash<quint64, ReferencesEntry> m_referencesTable;

    QHash<quint64, QFutureInterface<CppTools::SymbolInfo>> m_followTable;
};

class IpcSender : public ClangBackEnd::ClangCodeModelServerInterface
{
public:
    IpcSender(ClangBackEnd::ClangCodeModelConnectionClient *connectionClient);

    void end() override;
    void registerTranslationUnitsForEditor(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &message) override;
    void updateTranslationUnitsForEditor(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &message) override;
    void unregisterTranslationUnitsForEditor(const ClangBackEnd::UnregisterTranslationUnitsForEditorMessage &message) override;
    void registerProjectPartsForEditor(const ClangBackEnd::RegisterProjectPartsForEditorMessage &message) override;
    void unregisterProjectPartsForEditor(const ClangBackEnd::UnregisterProjectPartsForEditorMessage &message) override;
    void registerUnsavedFilesForEditor(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &message) override;
    void unregisterUnsavedFilesForEditor(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &message) override;
    void completeCode(const ClangBackEnd::CompleteCodeMessage &message) override;
    void requestDocumentAnnotations(const ClangBackEnd::RequestDocumentAnnotationsMessage &message) override;
    void requestReferences(const ClangBackEnd::RequestReferencesMessage &message) override;
    void requestFollowSymbol(const ClangBackEnd::RequestFollowSymbolMessage &message) override;
    void updateVisibleTranslationUnits(const ClangBackEnd::UpdateVisibleTranslationUnitsMessage &message) override;

private:
    bool isConnected() const;

private:
    ClangBackEnd::ClangCodeModelConnectionClient *m_connection = nullptr;
};

class IpcCommunicator : public QObject
{
    Q_OBJECT

public:
    using Ptr = QSharedPointer<IpcCommunicator>;
    using FileContainer = ClangBackEnd::FileContainer;
    using FileContainers = QVector<ClangBackEnd::FileContainer>;
    using ProjectPartContainers = QVector<ClangBackEnd::ProjectPartContainer>;

public:
    IpcCommunicator();
    ~IpcCommunicator();

    void registerTranslationUnitsForEditor(const FileContainers &fileContainers);
    void updateTranslationUnitsForEditor(const FileContainers &fileContainers);
    void unregisterTranslationUnitsForEditor(const FileContainers &fileContainers);
    void registerProjectPartsForEditor(const ProjectPartContainers &projectPartContainers);
    void unregisterProjectPartsForEditor(const QStringList &projectPartIds);
    void registerUnsavedFilesForEditor(const FileContainers &fileContainers);
    void unregisterUnsavedFilesForEditor(const FileContainers &fileContainers);
    void requestDocumentAnnotations(const ClangBackEnd::FileContainer &fileContainer);
    QFuture<CppTools::CursorInfo> requestReferences(
            const FileContainer &fileContainer,
            quint32 line,
            quint32 column,
            QTextDocument *textDocument,
            const CppTools::SemanticInfo::LocalUseMap &localUses);
    QFuture<CppTools::SymbolInfo> requestFollowSymbol(const FileContainer &curFileContainer,
                                                      const QVector<Utf8String> &dependentFiles,
                                                      quint32 line,
                                                      quint32 column);
    void completeCode(ClangCompletionAssistProcessor *assistProcessor, const QString &filePath,
                      quint32 line,
                      quint32 column,
                      const QString &projectFilePath,
                      qint32 funcNameStartLine = -1,
                      qint32 funcNameStartColumn = -1);

    void registerProjectsParts(const QVector<CppTools::ProjectPart::Ptr> projectParts);

    void updateTranslationUnitIfNotCurrentDocument(Core::IDocument *document);
    void updateTranslationUnit(Core::IDocument *document);
    void updateUnsavedFile(Core::IDocument *document);
    void updateTranslationUnitFromCppEditorDocument(const QString &filePath);
    void updateUnsavedFileFromCppEditorDocument(const QString &filePath);
    void updateTranslationUnit(const QString &filePath, const QByteArray &contents, uint documentRevision);
    void updateUnsavedFile(const QString &filePath, const QByteArray &contents, uint documentRevision);
    void updateTranslationUnitWithRevisionCheck(const ClangBackEnd::FileContainer &fileContainer);
    void updateTranslationUnitWithRevisionCheck(Core::IDocument *document);
    void updateChangeContentStartPosition(const QString &filePath, int position);

    void registerFallbackProjectPart();
    void updateTranslationUnitVisiblity();

    bool isNotWaitingForCompletion() const;

public: // for tests
    IpcSender *setIpcSender(IpcSender *ipcSender);
    void killBackendProcess();

signals: // for tests
    void backendReinitialized();

private:
    void initializeBackend();
    void initializeBackendWithCurrentData();
    void registerCurrentProjectParts();
    void restoreCppEditorDocuments();
    void resetCppEditorDocumentProcessors();
    void registerVisibleCppEditorDocumentAndMarkInvisibleDirty();
    void registerCurrentCodeModelUiHeaders();

    void setupDummySender();

    void onConnectedToBackend();
    void onDisconnectedFromBackend();
    void onEditorAboutToClose(Core::IEditor *editor);

    void logExecutableDoesNotExist();
    void logRestartedDueToUnexpectedFinish();
    void logStartTimeOut();
    void logError(const QString &text);

    void updateTranslationUnitVisiblity(const Utf8String &currentEditorFilePath,
                                        const Utf8StringVector &visibleEditorsFilePaths);

private:
    IpcReceiver m_ipcReceiver;
    ClangBackEnd::ClangCodeModelConnectionClient m_connection;
    QTimer m_backendStartTimeOut;
    QScopedPointer<IpcSender> m_ipcSender;
    int m_connectedCount = 0;
};

} // namespace Internal
} // namespace ClangCodeModel
