// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMLFINDUSAGESUPPORT_P_H
#define QMLFINDUSAGESUPPORT_P_H

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
struct ReferencesRequest : public BaseRequest<QLspSpecification::ReferenceParams,
                                              QLspSpecification::Responses::ReferenceResponseType>
{
};

class QQmlFindUsagesSupport : public QQmlBaseModule<ReferencesRequest>
{
    Q_OBJECT
public:
    QQmlFindUsagesSupport(QmlLsp::QQmlCodeModel *codeModel);

    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;

    void process(RequestPointerArgument request) override;

    void typeDefinitionRequestHandler(const QByteArray &,
                                      const QLspSpecification::TypeDefinitionParams &params,
                                      ReferencesRequest::Response &&response);
};
QT_END_NAMESPACE

#endif //  QMLFINDUSAGESUPPORT_P_H
