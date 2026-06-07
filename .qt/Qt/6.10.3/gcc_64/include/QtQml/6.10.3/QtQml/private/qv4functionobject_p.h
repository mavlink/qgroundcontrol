// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4FUNCTIONOBJECT_H
#define QV4FUNCTIONOBJECT_H

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

#include "qv4object_p.h"
#include "qv4function_p.h"
#include "qv4context_p.h"
#include <private/qv4mm_p.h>

QT_BEGIN_NAMESPACE

struct QQmlSourceLocation;

namespace QV4 {

struct IndexedBuiltinFunction;
struct JSCallData;

// A FunctionObject is generally something that can be called, either with a JavaScript
// signature (QV4::Value etc) or with a C++ signature (QMetaType etc). For this, it has
// the Call and CallWithMetaTypes VTable entries.
// Some FunctionObjects need to select the actual implementation of the call at run time.
// This comese in two flavors:
// 1. The implementation is a JavaScript function. For these we have
//    JavaScriptFunctionObject that holds a QV4::Function member to defer the call to.
// 2. The implementation is a C++ function. For these we have DynamicFunctionObject that
//    holds another Call member in the heap object to defer the call to.
// In addition, a FunctionObject may want to be called as constructor. For this we have
// another VTable entry and a flag in the heap object.

namespace Heap {

#define FunctionObjectMembers(class, Member)
DECLARE_HEAP_OBJECT(FunctionObject, Object) {
    enum {
        Index_ProtoConstructor = 0,
        Index_Prototype = 0,
        Index_HasInstance = 1,
    };

    Q_QML_EXPORT void init(QV4::ExecutionEngine *engine, QV4::String *name = nullptr);
    Q_QML_EXPORT void init(QV4::ExecutionEngine *engine, const QString &name);
    Q_QML_EXPORT void init();
};

#define JavaScriptFunctionObjectMembers(class, Member) \
    Member(class, Pointer, ExecutionContext *, scope) \
    Member(class, NoMark, Function *, function)

DECLARE_HEAP_OBJECT(JavaScriptFunctionObject, FunctionObject) {
    DECLARE_MARKOBJECTS(JavaScriptFunctionObject)

    void init(QV4::ExecutionContext *scope, QV4::Function *function, QV4::String *n = nullptr);
    Q_QML_EXPORT void destroy();

    void setFunction(Function *f);

    unsigned int formalParameterCount() { return function ? function->nFormals : 0; }
    unsigned int varCount() { return function ? function->compiledFunction->nLocals : 0; }
};

#define DynamicFunctionObjectMembers(class, Member) \
    Member(class, NoMark, VTable::Call, jsCall)

DECLARE_HEAP_OBJECT(DynamicFunctionObject, FunctionObject) {
    // NB: We might add a CallWithMetaTypes member to this struct and implement our
    //     builtins with metatypes, to be called from C++ code. This would make them
    //     available to qmlcachegen's C++ code generation.
    void init(ExecutionEngine *engine, QV4::String *name, VTable::Call call);
};

struct FunctionCtor : FunctionObject {
    void init(QV4::ExecutionEngine *engine);
};

struct FunctionPrototype : FunctionObject {
    void init();
};

// A function object with an additional index into a list.
// Used by Models to refer to property roles.
struct IndexedBuiltinFunction : DynamicFunctionObject {
    inline void init(QV4::ExecutionEngine *engine, qsizetype index, VTable::Call call);
    qsizetype index;
};

struct ArrowFunction : JavaScriptFunctionObject {
    enum {
        Index_Name = Index_HasInstance + 1,
        Index_Length
    };
    void init(QV4::ExecutionContext *scope, Function *function, QV4::String *name = nullptr);
};

#define ScriptFunctionMembers(class, Member) \
    Member(class, Pointer, InternalClass *, cachedClassForConstructor)

DECLARE_HEAP_OBJECT(ScriptFunction, ArrowFunction) {
    DECLARE_MARKOBJECTS(ScriptFunction)
    void init(QV4::ExecutionContext *scope, Function *function);
};

#define MemberFunctionMembers(class, Member) \
    Member(class, Pointer, Object *, homeObject)

DECLARE_HEAP_OBJECT(MemberFunction, ArrowFunction) {
    DECLARE_MARKOBJECTS(MemberFunction)

    void init(QV4::ExecutionContext *scope, Function *function, QV4::String *name = nullptr) {
        ArrowFunction::init(scope, function, name);
    }
};

#define ConstructorFunctionMembers(class, Member) \
    Member(class, Pointer, Object *, homeObject)

DECLARE_HEAP_OBJECT(ConstructorFunction, ScriptFunction) {
    DECLARE_MARKOBJECTS(ConstructorFunction)
    bool isDerivedConstructor;
};

#define DefaultClassConstructorFunctionMembers(class, Member) \
    Member(class, Pointer, ExecutionContext *, scope)

DECLARE_HEAP_OBJECT(DefaultClassConstructorFunction, FunctionObject) {
    DECLARE_MARKOBJECTS(DefaultClassConstructorFunction)

    bool isDerivedConstructor;

    void init(QV4::ExecutionContext *scope);
};

#define BoundFunctionMembers(class, Member) \
    Member(class, Pointer, FunctionObject *, target) \
    Member(class, HeapValue, HeapValue, boundThis) \
    Member(class, Pointer, MemberData *, boundArgs)

DECLARE_HEAP_OBJECT(BoundFunction, JavaScriptFunctionObject) {
    DECLARE_MARKOBJECTS(BoundFunction)

    void init(QV4::FunctionObject *target, const Value &boundThis, QV4::MemberData *boundArgs);
};

struct BoundConstructor : BoundFunction {};

}

struct Q_QML_EXPORT FunctionObject: Object {
    V4_OBJECT2(FunctionObject, Object)
    Q_MANAGED_TYPE(FunctionObject)
    V4_INTERNALCLASS(FunctionObject)
    V4_PROTOTYPE(functionPrototype)
    enum { NInlineProperties = 1 };

    bool canBeTailCalled() const { return vtable()->isTailCallable; }

    ReturnedValue name() const;

    void setName(String *name) {
        defineReadonlyConfigurableProperty(engine()->id_name(), *name);
    }
    void createDefaultPrototypeProperty(uint protoConstructorSlot);

    ReturnedValue callAsConstructor(
            const Value *argv, int argc, const Value *newTarget = nullptr) const
    {
        if (const auto callAsConstructor = vtable()->callAsConstructor)
            return callAsConstructor(this, argv, argc, newTarget ? newTarget : this);
        return failCallAsConstructor();
    }

    ReturnedValue call(const Value *thisObject, const Value *argv, int argc) const
    {
        if (const auto call = vtable()->call)
            return call(this, thisObject, argv, argc);
        return failCall();
    }

    void call(QObject *thisObject, void **argv, const QMetaType *types, int argc) const
    {
        if (const auto callWithMetaTypes = vtable()->callWithMetaTypes)
            callWithMetaTypes(this, thisObject, argv, types, argc);
        else
            failCall();
    }

    inline ReturnedValue callAsConstructor(const JSCallData &data) const;
    inline ReturnedValue call(const JSCallData &data) const;

    ReturnedValue failCall() const;
    ReturnedValue failCallAsConstructor() const;
    static void virtualConvertAndCall(
            const FunctionObject *f, QObject *thisObject,
            void **argv, const QMetaType *types, int argc);

    static Heap::FunctionObject *createScriptFunction(ExecutionContext *scope, Function *function);
    static Heap::FunctionObject *createConstructorFunction(ExecutionContext *scope, Function *function, Object *homeObject, bool isDerivedConstructor);
    static Heap::FunctionObject *createMemberFunction(ExecutionContext *scope, Function *function, Object *homeObject, String *name);
    static Heap::FunctionObject *createBuiltinFunction(ExecutionEngine *engine, StringOrSymbol *nameOrSymbol, VTable::Call code, int argumentCount);

    bool isBinding() const;
    bool isBoundFunction() const;
    bool isConstructor() const { return vtable()->callAsConstructor; }

    ReturnedValue getHomeObject() const;

    ReturnedValue protoProperty() const {
        return getValueByIndex(Heap::FunctionObject::Index_Prototype);
    }
    bool hasHasInstanceProperty() const {
        return !internalClass()->propertyData.at(Heap::FunctionObject::Index_HasInstance).isEmpty();
    }
};

template<>
inline const FunctionObject *Value::as() const {
    if (!isManaged())
        return nullptr;

    const VTable *vtable = m()->internalClass->vtable;
    return (vtable->call || vtable->callAsConstructor)
            ? reinterpret_cast<const FunctionObject *>(this)
            : nullptr;
}

struct Q_QML_EXPORT JavaScriptFunctionObject: FunctionObject
{
    V4_OBJECT2(JavaScriptFunctionObject, FunctionObject)
    V4_NEEDS_DESTROY

    Heap::ExecutionContext *scope() const { return d()->scope; }

    Function *function() const { return d()->function; }
    unsigned int formalParameterCount() const { return d()->formalParameterCount(); }
    unsigned int varCount() const { return d()->varCount(); }
    bool strictMode() const { return d()->function ? d()->function->isStrict() : false; }
    QQmlSourceLocation sourceLocation() const;
};

struct Q_QML_EXPORT DynamicFunctionObject: FunctionObject
{
    V4_OBJECT2(DynamicFunctionObject, FunctionObject)

    static ReturnedValue virtualCall(
            const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct FunctionCtor: FunctionObject
{
    V4_OBJECT2(FunctionCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
protected:
    enum Type {
        Type_Function,
        Type_Generator
    };
    static QQmlRefPointer<ExecutableCompilationUnit> parse(ExecutionEngine *engine, const Value *argv, int argc, Type t = Type_Function);
};

struct FunctionPrototype: FunctionObject
{
    V4_OBJECT2(FunctionPrototype, FunctionObject)

    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue virtualCall(
            const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_apply(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_call(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_bind(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_hasInstance(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct Q_QML_EXPORT IndexedBuiltinFunction : DynamicFunctionObject
{
    V4_OBJECT2(IndexedBuiltinFunction, DynamicFunctionObject)
};

void Heap::IndexedBuiltinFunction::init(
        QV4::ExecutionEngine *engine, qsizetype index, VTable::Call call)
{
    Heap::FunctionObject::init(engine);
    this->jsCall = call;
    this->index = index;
}

struct ArrowFunction : JavaScriptFunctionObject {
    V4_OBJECT2(ArrowFunction, JavaScriptFunctionObject)
    V4_INTERNALCLASS(ArrowFunction)
    enum {
        NInlineProperties = 3,
        IsTailCallable = true,
    };

    static void virtualCallWithMetaTypes(const FunctionObject *f, QObject *thisObject,
                                         void **a, const QMetaType *types, int argc);
    static ReturnedValue virtualCall(const QV4::FunctionObject *f, const QV4::Value *thisObject,
                                     const QV4::Value *argv, int argc);
};

struct ScriptFunction : ArrowFunction {
    V4_OBJECT2(ScriptFunction, ArrowFunction)
    V4_INTERNALCLASS(ScriptFunction)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *);

    Heap::InternalClass *classForConstructor() const;
};

struct MemberFunction : ArrowFunction {
    V4_OBJECT2(MemberFunction, ArrowFunction)
    V4_INTERNALCLASS(MemberFunction)
};

struct ConstructorFunction : ScriptFunction {
    V4_OBJECT2(ConstructorFunction, ScriptFunction)
    V4_INTERNALCLASS(ConstructorFunction)
    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct DefaultClassConstructorFunction : FunctionObject {
    V4_OBJECT2(DefaultClassConstructorFunction, FunctionObject)

    Heap::ExecutionContext *scope() const { return d()->scope; }
    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct BoundFunction: JavaScriptFunctionObject {
    V4_OBJECT2(BoundFunction, JavaScriptFunctionObject)

    Heap::FunctionObject *target() const { return d()->target; }
    Value boundThis() const { return d()->boundThis; }
    Heap::MemberData *boundArgs() const { return d()->boundArgs; }

    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct BoundConstructor: BoundFunction {
    V4_OBJECT2(BoundConstructor, BoundFunction)

    static ReturnedValue virtualCallAsConstructor(
            const FunctionObject *f, const Value *argv, int argc, const Value *);
};

inline bool FunctionObject::isBoundFunction() const
{
    const VTable *vtable = d()->vtable();
    return vtable == BoundFunction::staticVTable() || vtable == BoundConstructor::staticVTable();
}

inline ReturnedValue checkedResult(QV4::ExecutionEngine *v4, ReturnedValue result)
{
    return v4->hasException ? QV4::Encode::undefined() : result;
}

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
