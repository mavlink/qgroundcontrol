// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4RUNTIMECODEGEN_P_H
#define QV4RUNTIMECODEGEN_P_H

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

#include <private/qv4codegen_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

class RuntimeCodegen : public Compiler::Codegen
{
public:
    RuntimeCodegen(ExecutionEngine *engine, Compiler::JSUnitGenerator *jsUnitGenerator, bool strict)
        : Codegen(jsUnitGenerator, strict)
        , engine(engine)
    {}

    void generateFromFunctionExpression(
            const QString &sourceCode, QQmlJS::AST::FunctionExpression *ast,
            Compiler::Module *module);

    void throwSyntaxError(const QQmlJS::SourceLocation &loc, const QString &detail) override;
    void throwReferenceError(const QQmlJS::SourceLocation &loc, const QString &detail) override;

private:
    ExecutionEngine *engine;
};

}

QT_END_NAMESPACE

#endif // QV4CODEGEN_P_H
