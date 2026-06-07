// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLHOVER_P_H
#define QQMLHOVER_P_H

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

struct HoverRequest
    : public BaseRequest<QLspSpecification::HoverParams,
                         QLspSpecification::Responses::HoverResponseType>
{
};
class HelpManager;
class QQmlHover : public QQmlBaseModule<HoverRequest>
{
    Q_OBJECT
public:
    QQmlHover(QmlLsp::QQmlCodeModel *codeModel);
    ~QQmlHover() override;
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;
    void process(RequestPointerArgument req) override;

private:
    std::unique_ptr<HelpManager> m_helpManager;
};

QT_END_NAMESPACE

#endif // QQMLHOVER_P_H
