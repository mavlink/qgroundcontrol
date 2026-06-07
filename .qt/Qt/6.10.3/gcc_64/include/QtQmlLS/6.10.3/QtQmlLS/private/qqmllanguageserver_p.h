// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLLANGUAGESERVER_P_H
#define QQMLLANGUAGESERVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "documentSymbolSupport/qqmldocumentsymbolsupport_p.h"
#include "qlanguageserver_p.h"
#include "qqmlcodemodel_p.h"
#include "qqmlfindusagessupport_p.h"
#include "qtextsynchronization_p.h"
#include "qqmllintsuggestions_p.h"
#include "qworkspace_p.h"
#include "qqmlcompletionsupport_p.h"
#include "qqmlgototypedefinitionsupport_p.h"
#include "qqmlformatting_p.h"
#include "qqmlrangeformatting_p.h"
#include "qqmlgotodefinitionsupport_p.h"
#include "qqmlrenamesymbolsupport_p.h"
#include "qqmlhover_p.h"
#include "qqmlhighlightsupport_p.h"

QT_BEGIN_NAMESPACE

class QQmlToolingSettings;

namespace QmlLsp {

class QQmlLanguageServer : public QLanguageServerModule
{
    Q_OBJECT
public:
    QQmlLanguageServer(std::function<void(const QByteArray &)> sendData,
                       QQmlToolingSettings *settings = nullptr);
    ~QQmlLanguageServer();

    QString name() const final;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) final;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &serverInfo) final;

    int returnValue() const;

    QQmlCodeModel *codeModel();
    QLanguageServer *server();
    TextSynchronization *textSynchronization();
    QmlLintSuggestions *lint();
    WorkspaceHandlers *worspace();

public Q_SLOTS:
    void exit();
    void errorExit();

private:
    QQmlCodeModel m_codeModel;
    QLanguageServer m_server;

    TextSynchronization m_textSynchronization;
    WorkspaceHandlers m_workspace;

    // note: the order in which server modules are initialized also defines the order in which
    // they are run! Most of them connect to m_codeModelManager::updatedSnapshot in
    // their constructors, see https://doc.qt.io/qt-6/signalsandslots.html#signals.
    // ==== modules that are directly triggered by the user ====
    QmlCompletionSupport m_completionSupport;
    QmlGoToTypeDefinitionSupport m_navigationSupport;
    QmlGoToDefinitionSupport m_definitionSupport;
    QQmlFindUsagesSupport m_referencesSupport;
    QQmlDocumentFormatting m_documentFormatting;
    QQmlRenameSymbolSupport m_renameSupport;
    QQmlRangeFormatting m_rangeFormatting;
    QQmlHover m_hover;

    // ==== Highlighting ====
    QQmlHighlightSupport m_highlightSupport;

    // ==== modules that are not triggered by the user ====
    QQmlDocumentSymbolSupport m_documentSymbolSupport;

    // ==== Linting should happen at the end as it potentially can take a longer time ====
    QmlLintSuggestions m_lint;
    int m_returnValue = 1;
};

} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QQMLLANGUAGESERVER_P_H
