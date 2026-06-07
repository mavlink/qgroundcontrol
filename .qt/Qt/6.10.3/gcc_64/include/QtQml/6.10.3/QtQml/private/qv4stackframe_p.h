// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4STACKFRAME_H
#define QV4STACKFRAME_H

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

#include <private/qv4scopedvalue_p.h>
#include <private/qv4context_p.h>
#include <private/qv4enginebase_p.h>
#include <private/qv4calldata_p.h>
#include <private/qv4function_p.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct CppStackFrame;
struct Q_QML_EXPORT CppStackFrameBase
{
    enum class Kind : quint8 { JS, Meta };

    CppStackFrame *parent;
    Function *v4Function;
    int originalArgumentsCount;
    int instructionPointer;

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_MSVC(4201) // nonstandard extension used: nameless struct/union
    union {
        struct {
            Value *savedStackTop;
            CallData *jsFrame;
            const Value *originalArguments;
            const char *yield;
            const char *unwindHandler;
            const char *unwindLabel;
            int unwindLevel;
            bool yieldIsIterator;
            bool callerCanHandleTailCall;
            bool pendingTailCall;
            bool isTailCalling;
        };
        struct {
            ExecutionContext *context;
            QObject *thisObject;
            const QMetaType *metaTypes;
            const QQmlPrivate::AOTTrackedLocalsStorage *locals;
            void **returnAndArgs;
            bool returnValueIsUndefined;
        };
    };
    QT_WARNING_POP

    Kind kind;
};

struct Q_QML_EXPORT CppStackFrame : protected CppStackFrameBase
{
    // We want to have those public but we can't declare them as public without making the struct
    // non-standard layout. So we have this other struct with "using" in between.
    using CppStackFrameBase::instructionPointer;
    using CppStackFrameBase::locals;
    using CppStackFrameBase::v4Function;

    void init(Function *v4Function, int argc, Kind kind) {
        this->v4Function = v4Function;
        originalArgumentsCount = argc;
        instructionPointer = 0;
        locals = nullptr;
        this->kind = kind;
    }

    bool isJSTypesFrame() const { return kind == Kind::JS; }
    bool isMetaTypesFrame() const { return kind == Kind::Meta; }

    QString source() const;
    QString function() const;
    int lineNumber() const;
    int statementNumber() const;

    int missingLineNumber() const;

    CppStackFrame *parentFrame() const { return parent; }
    void setParentFrame(CppStackFrame *parentFrame) { parent = parentFrame; }

    int argc() const { return originalArgumentsCount; }

    inline ExecutionContext *context() const;

    Heap::CallContext *callContext() const { return callContext(context()->d()); }
    ReturnedValue thisObject() const;

protected:
    CppStackFrame() = default;

    void push(EngineBase *engine)
    {
        Q_ASSERT(kind == Kind::JS || kind == Kind::Meta);
        parent = engine->currentStackFrame;
        engine->currentStackFrame = this;
    }

    void pop(EngineBase *engine)
    {
        engine->currentStackFrame = parent;
    }

    Heap::CallContext *callContext(Heap::ExecutionContext *ctx) const
    {
        while (ctx->type != Heap::ExecutionContext::Type_CallContext)
            ctx = ctx->outer;
        return static_cast<Heap::CallContext *>(ctx);
    }
};

struct Q_QML_EXPORT MetaTypesStackFrame : public CppStackFrame
{
    using CppStackFrame::push;
    using CppStackFrame::pop;

    void init(Function *v4Function, QObject *thisObject, ExecutionContext *context,
              void **returnAndArgs, const QMetaType *metaTypes, int argc)
    {
        CppStackFrame::init(v4Function, argc, Kind::Meta);
        CppStackFrameBase::context = context;
        CppStackFrameBase::thisObject = thisObject;
        CppStackFrameBase::metaTypes = metaTypes;
        CppStackFrameBase::locals = nullptr;
        CppStackFrameBase::returnAndArgs = returnAndArgs;
        CppStackFrameBase::returnValueIsUndefined = false;
    }

    QMetaType returnType() const { return metaTypes[0]; }
    void *returnValue() const { return returnAndArgs[0]; }

    bool isReturnValueUndefined() const { return CppStackFrameBase::returnValueIsUndefined; }
    void setReturnValueUndefined() { CppStackFrameBase::returnValueIsUndefined = true; }

    const QMetaType *argTypes() const { return metaTypes + 1; }
    void **argv() const { return returnAndArgs + 1; }

    const QMetaType *returnAndArgTypes() const { return metaTypes; }
    void **returnAndArgValues() const { return returnAndArgs; }

    QObject *thisObject() const { return CppStackFrameBase::thisObject; }

    ExecutionContext *context() const { return CppStackFrameBase::context; }
    void setContext(ExecutionContext *context) { CppStackFrameBase::context = context; }

    const QQmlPrivate::AOTTrackedLocalsStorage *locals() const { return CppStackFrameBase::locals; }
    void setLocals(const QQmlPrivate::AOTTrackedLocalsStorage *locals)
    {
        CppStackFrameBase::locals = locals;
    }

    Heap::CallContext *callContext() const
    {
        return CppStackFrame::callContext(CppStackFrameBase::context->d());
    }
};

struct Q_QML_EXPORT JSTypesStackFrame : public CppStackFrame
{
    using CppStackFrame::jsFrame;

    // The JIT needs to poke directly into those using offsetof
    using CppStackFrame::unwindHandler;
    using CppStackFrame::unwindLabel;
    using CppStackFrame::unwindLevel;

    void init(Function *v4Function, const Value *argv, int argc,
              bool callerCanHandleTailCall = false)
    {
        CppStackFrame::init(v4Function, argc, Kind::JS);
        CppStackFrame::originalArguments = argv;
        CppStackFrame::yield = nullptr;
        CppStackFrame::unwindHandler = nullptr;
        CppStackFrame::yieldIsIterator = false;
        CppStackFrame::callerCanHandleTailCall = callerCanHandleTailCall;
        CppStackFrame::pendingTailCall = false;
        CppStackFrame::isTailCalling = false;
        CppStackFrame::unwindLabel = nullptr;
        CppStackFrame::unwindLevel = 0;
    }

    const Value *argv() const { return originalArguments; }

    static uint requiredJSStackFrameSize(uint nRegisters) {
        return CallData::HeaderSize() + nRegisters;
    }
    static uint requiredJSStackFrameSize(Function *v4Function) {
        return CallData::HeaderSize() + v4Function->compiledFunction->nRegisters;
    }
    uint requiredJSStackFrameSize() const {
        return requiredJSStackFrameSize(v4Function);
    }

    void setupJSFrame(Value *stackSpace, const Value &function, const Heap::ExecutionContext *scope,
                      const Value &thisObject, const Value &newTarget = Value::undefinedValue()) {
        setupJSFrame(stackSpace, function, scope, thisObject, newTarget,
                     v4Function->compiledFunction->nFormals,
                     v4Function->compiledFunction->nRegisters);
    }

    void setupJSFrame(
            Value *stackSpace, const Value &function, const Heap::ExecutionContext *scope,
            const Value &thisObject, const Value &newTarget, uint nFormals, uint nRegisters)
    {
        jsFrame = reinterpret_cast<CallData *>(stackSpace);
        jsFrame->function = function;
        jsFrame->context = scope->asReturnedValue();
        jsFrame->accumulator = Encode::undefined();
        jsFrame->thisObject = thisObject;
        jsFrame->newTarget = newTarget;

        uint argc = uint(originalArgumentsCount);
        if (argc > nFormals)
            argc = nFormals;
        jsFrame->setArgc(argc);

        // memcpy requires non-null ptr, even if  argc * sizeof(Value) == 0
        if (originalArguments)
            memcpy(jsFrame->args, originalArguments, argc * sizeof(Value));
        Q_STATIC_ASSERT(Encode::undefined() == 0);
        memset(jsFrame->args + argc, 0, (nRegisters - argc) * sizeof(Value));

        if (v4Function && v4Function->compiledFunction) {
            const int firstDeadZoneRegister
                    = v4Function->compiledFunction->firstTemporalDeadZoneRegister;
            const int registerDeadZoneSize
                    = v4Function->compiledFunction->sizeOfRegisterTemporalDeadZone;

            const Value * tdzEnd = stackSpace + firstDeadZoneRegister + registerDeadZoneSize;
            for (Value *v = stackSpace + firstDeadZoneRegister; v < tdzEnd; ++v)
                *v = Value::emptyValue().asReturnedValue();
        }
    }

    ExecutionContext *context() const
    {
        return static_cast<ExecutionContext *>(&jsFrame->context);
    }

    void setContext(ExecutionContext *context)
    {
        jsFrame->context = context;
    }

    Heap::CallContext *callContext() const
    {
        return CppStackFrame::callContext(static_cast<ExecutionContext &>(jsFrame->context).d());
    }

    bool isTailCalling() const { return CppStackFrame::isTailCalling; }
    void setTailCalling(bool tailCalling) { CppStackFrame::isTailCalling = tailCalling; }

    bool pendingTailCall() const { return CppStackFrame::pendingTailCall; }
    void setPendingTailCall(bool pending) { CppStackFrame::pendingTailCall = pending; }

    const char *yield() const { return CppStackFrame::yield; }
    void setYield(const char *yield) { CppStackFrame::yield = yield; }

    bool yieldIsIterator() const { return CppStackFrame::yieldIsIterator; }
    void setYieldIsIterator(bool isIter) { CppStackFrame::yieldIsIterator = isIter; }

    bool callerCanHandleTailCall() const { return CppStackFrame::callerCanHandleTailCall; }

    ReturnedValue thisObject() const
    {
        return jsFrame->thisObject.asReturnedValue();
    }

    Value *framePointer() const { return savedStackTop; }

    void push(EngineBase *engine) {
        CppStackFrame::push(engine);
        savedStackTop = engine->jsStackTop;
    }

    void pop(EngineBase *engine) {
        CppStackFrame::pop(engine);
        engine->jsStackTop = savedStackTop;
    }
};

inline ExecutionContext *CppStackFrame::context() const
{
    if (isJSTypesFrame())
        return static_cast<const JSTypesStackFrame *>(this)->context();

    Q_ASSERT(isMetaTypesFrame());
    return static_cast<const MetaTypesStackFrame *>(this)->context();
}

struct ScopedStackFrame
{
    ScopedStackFrame(const Scope &scope, ExecutionContext *context)
        : engine(scope.engine)
    {
        if (auto currentFrame = engine->currentStackFrame) {
            frame.init(currentFrame->v4Function, nullptr, context, nullptr, nullptr, 0);
            frame.instructionPointer = currentFrame->instructionPointer;
        } else {
            frame.init(nullptr, nullptr, context, nullptr, nullptr, 0);
        }
        frame.push(engine);
    }

    ~ScopedStackFrame()
    {
        frame.pop(engine);
    }

private:
    ExecutionEngine *engine = nullptr;
    MetaTypesStackFrame frame;
};

Q_STATIC_ASSERT(sizeof(CppStackFrame) == sizeof(JSTypesStackFrame));
Q_STATIC_ASSERT(sizeof(CppStackFrame) == sizeof(MetaTypesStackFrame));
Q_STATIC_ASSERT(std::is_standard_layout_v<CppStackFrame>);
Q_STATIC_ASSERT(std::is_standard_layout_v<JSTypesStackFrame>);
Q_STATIC_ASSERT(std::is_standard_layout_v<MetaTypesStackFrame>);

}

QT_END_NAMESPACE

#endif
