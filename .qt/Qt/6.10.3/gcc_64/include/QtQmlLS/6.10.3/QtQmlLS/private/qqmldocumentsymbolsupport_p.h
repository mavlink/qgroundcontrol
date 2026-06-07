// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOCUMENTSYMBOLSUPPORT_P_H
#define QQMLDOCUMENTSYMBOLSUPPORT_P_H

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

#include <QtQmlLS/private/qlanguageserver_p.h>
#include <QtQmlLS/private/qqmlbasemodule_p.h>
#include <QtQmlLS/private/qqmlcodemodel_p.h>

QT_BEGIN_NAMESPACE

struct DocumentSymbolsRequest
    : public BaseRequest<QLspSpecification::DocumentSymbolParams,
                         QLspSpecification::Responses::DocumentSymbolResponseType>
{
};

class QQmlDocumentSymbolSupport : public QQmlBaseModule<DocumentSymbolsRequest>
{
public:
    QQmlDocumentSymbolSupport(QmlLsp::QQmlCodeModel *codeModel);
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;
    void process(RequestPointerArgument req) override;
};

QT_END_NAMESPACE

#endif // QQMLDOCUMENTSYMBOLSUPPORT_P_H
