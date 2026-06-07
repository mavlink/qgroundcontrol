// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4COMPILERCONTROLFLOW_P_H
#define QV4COMPILERCONTROLFLOW_P_H

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
#include <private/qqmljsast_p.h>
#include <private/qv4bytecodegenerator_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Compiler {

struct ControlFlow {
    using Reference = Codegen::Reference;
    using BytecodeGenerator = Moth::BytecodeGenerator;
    using Instruction = Moth::Instruction;

    enum Type {
        Loop,
        With,
        Block,
        Finally,
        Catch
    };

    enum UnwindType {
        Break,
        Continue,
        Return
    };

    struct UnwindTarget {
        BytecodeGenerator::Label linkLabel;
        int unwindLevel;
    };

    Codegen *cg;
    ControlFlow *parent;
    Type type;

    ControlFlow(Codegen *cg, Type type)
        : cg(cg), parent(cg->controlFlow), type(type)
    {
        cg->controlFlow = this;
    }

    virtual ~ControlFlow() {
        cg->controlFlow = parent;
    }

    UnwindTarget unwindTarget(UnwindType type, const QString &label = QString())
    {
        Q_ASSERT(type == Break || type == Continue || type == Return);
        ControlFlow *flow = this;
        int level = 0;
        while (flow) {
            BytecodeGenerator::Label l = flow->getUnwindTarget(type, label);
            if (l.isValid())
                return UnwindTarget{l, level};
            if (flow->requiresUnwind())
                ++level;
            flow = flow->parent;
        }
        if (type == Return)
            return UnwindTarget{ cg->returnLabel(), level };
        return UnwindTarget();
    }

    virtual QString label() const { return QString(); }

    bool hasLoop() const {
        const ControlFlow *flow = this;
        while (flow) {
            if (flow->type == Loop)
                return true;
            flow = flow->parent;
        }
        return false;
    }

protected:
    virtual BytecodeGenerator::Label getUnwindTarget(UnwindType, const QString & = QString()) {
        return BytecodeGenerator::Label();
    }
    virtual bool requiresUnwind() {
        return false;
    }

public:
    BytecodeGenerator::ExceptionHandler *parentUnwindHandler() {
        return parent ? parent->unwindHandler() : nullptr;
    }

    virtual BytecodeGenerator::ExceptionHandler *unwindHandler() {
        return parentUnwindHandler();
    }


protected:
    QString loopLabel() const {
        QString label;
        if (cg->_labelledStatement) {
            label = cg->_labelledStatement->label.toString();
            cg->_labelledStatement = nullptr;
        }
        return label;
    }
    BytecodeGenerator *generator() const {
        return cg->bytecodeGenerator;
    }
};

struct ControlFlowUnwind : public ControlFlow
{
    BytecodeGenerator::ExceptionHandler unwindLabel;

    ControlFlowUnwind(Codegen *cg, Type type)
        : ControlFlow(cg, type)
    {
    }

    void setupUnwindHandler()
    {
        unwindLabel = generator()->newExceptionHandler();
    }

    void emitUnwindHandler()
    {
        Q_ASSERT(requiresUnwind());

        Instruction::UnwindDispatch dispatch;
        generator()->addInstruction(dispatch);
    }

    virtual BytecodeGenerator::ExceptionHandler *unwindHandler() override {
        return unwindLabel.isValid() ? &unwindLabel : parentUnwindHandler();
    }
};

struct ControlFlowUnwindCleanup : public ControlFlowUnwind
{
    std::function<void()> cleanup = nullptr;

    ControlFlowUnwindCleanup(Codegen *cg, std::function<void()> cleanup, Type type = Block)
        : ControlFlowUnwind(cg, type), cleanup(cleanup)
    {
        if (cleanup) {
            setupUnwindHandler();
            generator()->setUnwindHandler(&unwindLabel);
        }
    }

    ~ControlFlowUnwindCleanup() {
        if (cleanup) {
            unwindLabel.link();
            cleanup();
            generator()->setUnwindHandler(parentUnwindHandler());
            emitUnwindHandler();
        }
    }

    bool requiresUnwind() override {
        return cleanup != nullptr;
    }
};

struct ControlFlowLoop : public ControlFlowUnwindCleanup
{
    QString loopLabel;
    BytecodeGenerator::Label *breakLabel = nullptr;
    BytecodeGenerator::Label *continueLabel = nullptr;

    ControlFlowLoop(Codegen *cg, BytecodeGenerator::Label *breakLabel, BytecodeGenerator::Label *continueLabel = nullptr, std::function<void()> cleanup = nullptr)
        : ControlFlowUnwindCleanup(cg, cleanup, Loop), loopLabel(ControlFlow::loopLabel()), breakLabel(breakLabel), continueLabel(continueLabel)
    {
    }

    BytecodeGenerator::Label getUnwindTarget(UnwindType type, const QString &label) override {
        switch (type) {
        case Break:
            if (breakLabel && (label.isEmpty() || label == loopLabel))
                return *breakLabel;
            break;
        case Continue:
            if (continueLabel && (label.isEmpty() || label == loopLabel))
                return *continueLabel;
            break;
        default:
            break;
        }
        return BytecodeGenerator::Label();
    }

    QString label() const override { return loopLabel; }
};


struct ControlFlowWith : public ControlFlowUnwind
{
    ControlFlowWith(Codegen *cg)
        : ControlFlowUnwind(cg, With)
    {
        setupUnwindHandler();

        // assumes the with object is in the accumulator
        Instruction::PushWithContext pushScope;
        generator()->addInstruction(pushScope);
        generator()->setUnwindHandler(&unwindLabel);
    }

    ~ControlFlowWith() {
        // emit code for unwinding
        unwindLabel.link();

        generator()->setUnwindHandler(parentUnwindHandler());
        Instruction::PopContext pop;
        generator()->addInstruction(pop);

        emitUnwindHandler();
    }

    bool requiresUnwind() override {
        return true;
    }


};

struct ControlFlowBlock : public ControlFlowUnwind
{
    ControlFlowBlock(Codegen *cg, QQmlJS::AST::Node *ast)
        : ControlFlowUnwind(cg, Block)
    {
        block = cg->enterBlock(ast);
        block->emitBlockHeader(cg);

        if (block->requiresExecutionContext) {
            setupUnwindHandler();
            generator()->setUnwindHandler(&unwindLabel);
        }
    }

    virtual ~ControlFlowBlock() {
        // emit code for unwinding
        if (block->requiresExecutionContext) {
            unwindLabel.link();
            generator()->setUnwindHandler(parentUnwindHandler());
        }

        block->emitBlockFooter(cg);

        if (block->requiresExecutionContext )
            emitUnwindHandler();
        cg->leaveBlock();
    }

    virtual bool requiresUnwind() override {
        return block->requiresExecutionContext;
    }

    Context *block;
};

struct ControlFlowCatch : public ControlFlowUnwind
{
    QQmlJS::AST::Catch *catchExpression;
    bool insideCatch = false;
    BytecodeGenerator::ExceptionHandler exceptionLabel;

    ControlFlowCatch(Codegen *cg, QQmlJS::AST::Catch *catchExpression)
        : ControlFlowUnwind(cg, Catch), catchExpression(catchExpression),
          exceptionLabel(generator()->newExceptionHandler())
    {
        generator()->setUnwindHandler(&exceptionLabel);
    }

    virtual bool requiresUnwind() override {
        return true;
    }

    BytecodeGenerator::ExceptionHandler *unwindHandler() override {
        return insideCatch ? &unwindLabel : &exceptionLabel;
    }

    ~ControlFlowCatch() {
        // emit code for unwinding
        insideCatch = true;
        setupUnwindHandler();

        Codegen::RegisterScope scope(cg);

        // exceptions inside the try block go here
        exceptionLabel.link();
        BytecodeGenerator::Jump noException = generator()->jumpNoException();

        Context *block = cg->enterBlock(catchExpression);

        block->emitBlockHeader(cg);

        generator()->setUnwindHandler(&unwindLabel);

        if (catchExpression->patternElement->bindingIdentifier.isEmpty())
            // destructuring pattern
            cg->initializeAndDestructureBindingElement(catchExpression->patternElement, Reference::fromName(cg, QStringLiteral("@caught")));
        // skip the additional block
        cg->statementList(catchExpression->statement->statements);

        // exceptions inside catch and break/return statements go here
        unwindLabel.link();
        block->emitBlockFooter(cg);

        cg->leaveBlock();

        noException.link();
        generator()->setUnwindHandler(parentUnwindHandler());

        emitUnwindHandler();
        insideCatch = false;
    }
};

struct ControlFlowFinally : public ControlFlowUnwind
{
    QQmlJS::AST::Finally *finally;
    bool insideFinally = false;

    ControlFlowFinally(Codegen *cg, QQmlJS::AST::Finally *finally, bool hasCatchBlock)
        : ControlFlowUnwind(cg, Finally), finally(finally)
    {
        Q_ASSERT(finally != nullptr);
        setupUnwindHandler();

        // No need to set the handler for the finally now if there is a catch block.
        // In that case, a handler for the latter will be set immediately after this.
        if (!hasCatchBlock) {
            generator()->setUnwindHandler(&unwindLabel);
        }
    }

    virtual bool requiresUnwind() override {
        return !insideFinally;
    }

    BytecodeGenerator::ExceptionHandler *unwindHandler() override {
        return insideFinally ? parentUnwindHandler() : ControlFlowUnwind::unwindHandler();
    }

    ~ControlFlowFinally() {
        // emit code for unwinding
        unwindLabel.link();

        Codegen::RegisterScope scope(cg);

        insideFinally = true;
        int returnValueTemp = -1;
        if (cg->requiresReturnValue) {
            returnValueTemp = generator()->newRegister();
            Instruction::MoveReg move;
            move.srcReg = cg->_returnAddress;
            move.destReg = returnValueTemp;
            generator()->addInstruction(move);
        }
        int exceptionTemp = generator()->newRegister();
        Instruction::GetException instr;
        generator()->addInstruction(instr);
        Reference::fromStackSlot(cg, exceptionTemp).storeConsumeAccumulator();

        generator()->setUnwindHandler(parentUnwindHandler());
        cg->statement(finally->statement);
        insideFinally = false;

        if (cg->requiresReturnValue) {
            Instruction::MoveReg move;
            move.srcReg = returnValueTemp;
            move.destReg = cg->_returnAddress;
            generator()->addInstruction(move);
        }
        Reference::fromStackSlot(cg, exceptionTemp).loadInAccumulator();
        Instruction::SetException setException;
        generator()->addInstruction(setException);

        emitUnwindHandler();
    }
};

} } // QV4::Compiler namespace

QT_END_NAMESPACE

#endif
