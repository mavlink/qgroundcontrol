// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLCOMPLETIONSUPPORT_P_H
#define QQMLCOMPLETIONSUPPORT_P_H

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

#include "qlanguageserver_p.h"
#include "qqmlbasemodule_p.h"
#include "qqmlcodemodel_p.h"

#include <QtCore/qmutex.h>
#include <QtCore/qhash.h>
#include <QtQmlLS/private/qqmllscompletion_p.h>

QT_BEGIN_NAMESPACE
struct CompletionRequest
    : BaseRequest<QLspSpecification::CompletionParams,
                  QLspSpecification::LSPPartialResponse<
                          std::variant<QList<QLspSpecification::CompletionItem>,
                                       QLspSpecification::CompletionList, std::nullptr_t>,
                          std::variant<QLspSpecification::CompletionList,
                                       QList<QLspSpecification::CompletionItem>>>>
{
    QString code;

    bool fillFrom(QmlLsp::OpenDocument doc, const Parameters &params, Response &&response);
    void sendCompletions(const QList<QLspSpecification::CompletionItem> &completions);
    QString urlAndPos() const;
    QList<QLspSpecification::CompletionItem>
    completions(QmlLsp::OpenDocumentSnapshot &doc, const QQmlLSCompletion &completionEngine) const;
    QQmlJS::Dom::DomItem patchInvalidFileForParser(const QQmlJS::Dom::DomItem &file,
                                                   qsizetype position) const;
};

class QmlCompletionSupport : public QQmlBaseModule<CompletionRequest>
{
    Q_OBJECT
public:
    QmlCompletionSupport(QmlLsp::QQmlCodeModel *codeModel);
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;
    void process(RequestPointerArgument req) override;

    QQmlLSCompletion m_completionEngine;
};
QT_END_NAMESPACE

#endif // QMLCOMPLETIONSUPPORT_P_H
