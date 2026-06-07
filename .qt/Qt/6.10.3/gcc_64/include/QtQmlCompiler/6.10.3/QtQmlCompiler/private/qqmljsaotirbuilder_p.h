// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSAOTIRBUILDER_P_H
#define QQMLJSAOTIRBUILDER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <private/qqmlirbuilder_p.h>

QT_BEGIN_NAMESPACE

struct Q_QMLCOMPILER_EXPORT QQmlJSAOTIRBuilder : public QmlIR::IRBuilder
{
    using QmlIR::IRBuilder::visit;

    bool visit(QQmlJS::AST::FunctionExpression *ast) override;
    bool visit(QQmlJS::AST::FunctionDeclaration *ast) override;

    bool visit(QQmlJS::AST::UiSourceElement *node) override;

    void registerFunctionExpr(QQmlJS::AST::FunctionExpression *fexp, IsQmlFunction) override;

    void setBindingValue(QV4::CompiledData::Binding *binding, QQmlJS::AST::Statement *statement,
                         QQmlJS::AST::Node *parentNode) override;

    QSet<std::pair<QmlIR::Object *, QQmlJS::AST::FunctionExpression *>> qmlFuncDecls;
};

QT_END_NAMESPACE

#endif // QQMLJSAOTIRBUILDER_P_H
