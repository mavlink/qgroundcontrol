// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWORKSPACE_P_H
#define QWORKSPACE_P_H

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

#include "qqmlcodemodel_p.h"
#include "qlanguageserver_p.h"

QT_BEGIN_NAMESPACE

class WorkspaceHandlers : public QLanguageServerModule
{
    Q_OBJECT
public:
    WorkspaceHandlers(QmlLsp::QQmlCodeModel *codeModel) : m_codeModel(codeModel) { }
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;
public Q_SLOTS:
    void clientInitialized(QLanguageServer *);

private:
    QmlLsp::QQmlCodeModel *m_codeModel = nullptr;
};

QT_END_NAMESPACE

#endif // QWORKSPACE_P_H
