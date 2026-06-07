// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4ENGINEBASE_P_H
#define QV4ENGINEBASE_P_H

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

#include <private/qv4global_p.h>
#include <private/qv4runtimeapi_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct CppStackFrame;

// Base class for the execution engine
struct Q_QML_EXPORT EngineBase {

    CppStackFrame *currentStackFrame = nullptr;

    Value *jsStackTop = nullptr;

    // The JIT expects hasException and isInterrupted to be in the same 32bit word in memory.
    quint8 hasException = false;
    // isInterrupted is expected to be set from a different thread
#if defined(Q_ATOMIC_INT8_IS_SUPPORTED)
    QAtomicInteger<quint8> isInterrupted = false;
    quint16 unused = 0;
#elif defined(Q_ATOMIC_INT16_IS_SUPPORTED)
    quint8 unused = 0;
    QAtomicInteger<quint16> isInterrupted = false;
#else
#   error V4 needs either 8bit or 16bit atomics.
#endif

    quint8 isExecutingInRegExpJIT = false;
    quint8 isInitialized = false;
    quint8 inShutdown = false;
    quint8 isGCOngoing = false; // incremental gc is ongoing (but mutator might be running)
    MemoryManager *memoryManager = nullptr;

    union {
        const void *cppStackBase = nullptr;
        struct {
            qint32 callDepth;
#if QT_POINTER_SIZE == 8
            quint32 padding2;
#endif
        };
    };
    const void *cppStackLimit = nullptr;

    Object *globalObject = nullptr;
    Value *jsStackLimit = nullptr;
    Value *jsStackBase = nullptr;

    IdentifierTable *identifierTable = nullptr;

    // Exception handling
    Value *exceptionValue = nullptr;

    enum InternalClassType {
        Class_Empty,
        Class_String,
        Class_MemberData,
        Class_SimpleArrayData,
        Class_SparseArrayData,
        Class_ExecutionContext,
        Class_CallContext,
        Class_QmlContext,
        Class_Object,
        Class_ArrayObject,
        Class_FunctionObject,
        Class_ArrowFunction,
        Class_GeneratorFunction,
        Class_GeneratorObject,
        Class_StringObject,
        Class_SymbolObject,
        Class_ScriptFunction,
        Class_ConstructorFunction,
        Class_MemberFunction,
        Class_MemberGeneratorFunction,
        Class_ObjectProto,
        Class_RegExp,
        Class_RegExpObject,
        Class_RegExpExecArray,
        Class_ArgumentsObject,
        Class_StrictArgumentsObject,
        Class_ErrorObject,
        Class_ErrorObjectWithMessage,
        Class_ErrorProto,
        Class_QmlContextWrapper,
        Class_ProxyObject,
        Class_ProxyFunctionObject,
        Class_Symbol,
        NClasses
    };
    Heap::InternalClass *classes[NClasses];
    Heap::InternalClass *internalClasses(InternalClassType icType) { return classes[icType]; }
};

Q_STATIC_ASSERT(std::is_standard_layout<EngineBase>::value);
Q_STATIC_ASSERT(offsetof(EngineBase, currentStackFrame) == 0);
Q_STATIC_ASSERT(offsetof(EngineBase, jsStackTop) == offsetof(EngineBase, currentStackFrame) + QT_POINTER_SIZE);
Q_STATIC_ASSERT(offsetof(EngineBase, hasException) == offsetof(EngineBase, jsStackTop) + QT_POINTER_SIZE);
Q_STATIC_ASSERT(offsetof(EngineBase, memoryManager) == offsetof(EngineBase, hasException) + 8);
Q_STATIC_ASSERT(offsetof(EngineBase, isInterrupted) + sizeof(EngineBase::isInterrupted) <= offsetof(EngineBase, hasException) + 4);
Q_STATIC_ASSERT(offsetof(EngineBase, globalObject) % QT_POINTER_SIZE == 0);

}

QT_END_NAMESPACE

#endif
