// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLRANGEFORMATTING_P_H
#define QQMLRANGEFORMATTING_P_H

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

QT_BEGIN_NAMESPACE

struct RangeFormattingRequest
    : public BaseRequest<QLspSpecification::DocumentRangeFormattingParams,
                         QLspSpecification::Responses::DocumentRangeFormattingResponseType>
{
};

class QQmlRangeFormatting : public QQmlBaseModule<RangeFormattingRequest>
{
    Q_OBJECT
public:
    QQmlRangeFormatting(QmlLsp::QQmlCodeModel *codeModel);
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;
    void process(RequestPointerArgument req) override;
};

QT_END_NAMESPACE

#endif // QQMLRANGEFORMATTING_P_H
