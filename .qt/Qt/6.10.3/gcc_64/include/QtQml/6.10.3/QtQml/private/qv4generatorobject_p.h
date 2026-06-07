// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4GENERATOROBJECT_P_H
#define QV4GENERATOROBJECT_P_H

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

#include "qv4functionobject_p.h"
#include "qv4stackframe_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

enum class GeneratorState {
    Undefined,
    SuspendedStart,
    SuspendedYield,
    Executing,
    Completed
};

namespace Heap {

struct GeneratorFunctionCtor : FunctionObject {
    void init(ExecutionEngine *engine);
};

struct GeneratorFunction : ArrowFunction {
    void init(QV4::ExecutionContext *scope, Function *function, QV4::String *name = nullptr) {
        ArrowFunction::init(scope, function, name);
    }
};

struct MemberGeneratorFunction : MemberFunction {
};

struct GeneratorPrototype : FunctionObject {
    void init();
};

#define GeneratorObjectMembers(class, Member) \
    Member(class, Pointer, ExecutionContext *, context) \
    Member(class, NoMark, GeneratorState, state) \
    Member(class, NoMark, JSTypesStackFrame, cppFrame) \
    Member(class, Pointer, ArrayObject *, values) \
    Member(class, Pointer, ArrayObject *, jsFrame)

DECLARE_HEAP_OBJECT(GeneratorObject, Object) {
    DECLARE_MARKOBJECTS(GeneratorObject)
};

}

struct GeneratorFunctionCtor : FunctionCtor
{
    V4_OBJECT2(GeneratorFunctionCtor, FunctionCtor)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct GeneratorFunction : ArrowFunction
{
    V4_OBJECT2(GeneratorFunction, ArrowFunction)
    V4_INTERNALCLASS(GeneratorFunction)

    static inline constexpr quint8 IsTailCallable = false;

    static Heap::FunctionObject *create(ExecutionContext *scope, Function *function);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct MemberGeneratorFunction : MemberFunction
{
    V4_OBJECT2(MemberGeneratorFunction, MemberFunction)
    V4_INTERNALCLASS(MemberGeneratorFunction)

    static inline constexpr quint8 IsTailCallable = false;

    static Heap::FunctionObject *create(ExecutionContext *scope, Function *function, Object *homeObject, String *name);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct GeneratorPrototype : Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_next(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_return(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_throw(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


struct GeneratorObject : Object {
    V4_OBJECT2(GeneratorObject, Object)
    Q_MANAGED_TYPE(GeneratorObject)
    V4_INTERNALCLASS(GeneratorObject)
    V4_PROTOTYPE(generatorPrototype)

    ReturnedValue resume(ExecutionEngine *engine, const Value &arg, std::optional<Value>) const;
};

}

QT_END_NAMESPACE

#endif // QV4GENERATORFUNCTION_P_H

