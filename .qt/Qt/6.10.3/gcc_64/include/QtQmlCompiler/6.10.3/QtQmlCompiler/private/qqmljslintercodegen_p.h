// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSLINTERCODEGEN_P_H
#define QQMLJSLINTERCODEGEN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QString>
#include <QFile>
#include <QList>

#include <variant>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljscompiler_p.h>

#include <QtQmlCompiler/private/qqmljstyperesolver_p.h>
#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQmlCompiler/private/qqmljscompilepass_p.h>
#include <QtQmlCompiler/private/qqmljscontextproperties_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlSA {
class PassManager;
};

class QQmlJSLinterCodegen : public QQmlJSAotCompiler
{
public:
    QQmlJSLinterCodegen(QQmlJSImporter *importer, const QString &fileName,
                        const QStringList &qmldirFiles, QQmlJSLogger *logger,
                        const QQmlJS::ContextProperties &knownContextProperties);

    void setDocument(const QmlIR::JSCodeGen *codegen, const QmlIR::Document *document) override;
    std::variant<QQmlJSAotFunction, QList<QQmlJS::DiagnosticMessage>>
    compileBinding(const QV4::Compiler::Context *context, const QmlIR::Binding &irBinding,
                   QQmlJS::AST::Node *astNode) override;
    std::variant<QQmlJSAotFunction, QList<QQmlJS::DiagnosticMessage>>
    compileFunction(const QV4::Compiler::Context *context, const QString &name,
                    QQmlJS::AST::Node *astNode) override;

    void setTypeResolver(QQmlJSTypeResolver typeResolver)
    {
        m_typeResolver = std::move(typeResolver);
    }

    QQmlJSTypeResolver *typeResolver() { return &m_typeResolver; }

    void setPassManager(QQmlSA::PassManager *passManager);

    QQmlSA::PassManager *passManager() { return m_passManager; }

private:
    QQmlSA::PassManager *m_passManager = nullptr;

    void analyzeFunction(const QV4::Compiler::Context *context,
                         QQmlJSCompilePass::Function *function);
    QQmlJS::ContextProperties m_knownContextProperties;
};

QT_END_NAMESPACE

#endif
