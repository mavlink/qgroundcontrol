// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSBASICBLOCKS_P_H
#define QQMLJSBASICBLOCKS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.


#include <private/qflatmap_p.h>
#include <private/qqmljscompilepass_p.h>
#include <private/qqmljscompiler_p.h>

QT_BEGIN_NAMESPACE

class Q_QMLCOMPILER_EXPORT QQmlJSBasicBlocks : public QQmlJSCompilePass
{
public:
    QQmlJSBasicBlocks(const QV4::Compiler::Context *context,
                      const QV4::Compiler::JSUnitGenerator *unitGenerator,
                      const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger)
        : QQmlJSCompilePass(unitGenerator, typeResolver, logger), m_context{ context }
    {
    }

    ~QQmlJSBasicBlocks() = default;

    QQmlJSCompilePass::BlocksAndAnnotations run(const Function *function,
                                                QQmlJSAotCompiler::Flags compileFlags,
                                                bool &basicBlocksValidationFailed);

    struct BasicBlocksValidationResult { bool success = true; QString errorMessage; };
    BasicBlocksValidationResult basicBlocksValidation();

    static BasicBlocks::iterator
    basicBlockForInstruction(QFlatMap<int, BasicBlock> &container, int instructionOffset);
    static BasicBlocks::const_iterator
    basicBlockForInstruction(const QFlatMap<int, BasicBlock> &container, int instructionOffset);

    QList<ObjectOrArrayDefinition> objectAndArrayDefinitions() const;

private:
    QV4::Moth::ByteCodeHandler::Verdict startInstruction(QV4::Moth::Instr::Type type) override;
    void endInstruction(QV4::Moth::Instr::Type type) override;

    void generate_Jump(int offset) override;
    void generate_JumpTrue(int offset) override;
    void generate_JumpFalse(int offset) override;
    void generate_JumpNoException(int offset) override;
    void generate_JumpNotUndefined(int offset) override;
    void generate_IteratorNext(int value, int offset) override;
    void generate_GetOptionalLookup(int index, int offset) override;

    void generate_Ret() override;
    void generate_ThrowException() override;

    void generate_DefineArray(int argc, int argv) override;
    void generate_DefineObjectLiteral(int internalClassId, int argc, int args) override;
    void generate_Construct(int func, int argc, int argv) override;

    enum JumpMode { Unconditional, Conditional };
    void processJump(int offset, JumpMode mode);

    void dumpBasicBlocks();
    void dumpDOTGraph();

    const QV4::Compiler::Context *m_context;
    QList<ObjectOrArrayDefinition> m_objectAndArrayDefinitions;
    bool m_skipUntilNextLabel = false;
    bool m_hadBackJumps = false;
};

QT_END_NAMESPACE

#endif // QQMLJSBASICBLOCKS_P_H
