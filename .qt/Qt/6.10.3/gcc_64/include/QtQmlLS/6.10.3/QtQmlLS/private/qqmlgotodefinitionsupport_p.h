// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLGOTODEFINITIONSUPPORT_P_H
#define QQMLGOTODEFINITIONSUPPORT_P_H

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
#include "qqmlcodemodel_p.h"
#include "qqmlbasemodule_p.h"

QT_BEGIN_NAMESPACE

struct DefinitionRequest : public BaseRequest<QLspSpecification::DefinitionParams,
                                              QLspSpecification::Responses::DefinitionResponseType>
{
};

class QmlGoToDefinitionSupport : public QQmlBaseModule<DefinitionRequest>
{
    Q_OBJECT
public:
    QmlGoToDefinitionSupport(QmlLsp::QQmlCodeModel *codeModel);

    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;

    void process(RequestPointerArgument request) override;

    void typeDefinitionRequestHandler(const QByteArray &,
                                      const QLspSpecification::DefinitionParams &params,
                                      RequestPointerArgument response);
};

QT_END_NAMESPACE

#endif // QQMLGOTODEFINITIONSUPPORT_P_H
