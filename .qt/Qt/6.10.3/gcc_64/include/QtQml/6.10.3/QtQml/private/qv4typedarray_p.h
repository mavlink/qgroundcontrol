// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4TYPEDARRAY_H
#define QV4TYPEDARRAY_H

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
#include "qv4functionobject_p.h"
#include "qv4arraybuffer_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct ArrayBuffer;

enum TypedArrayType {
    Int8Array,
    UInt8Array,
    Int16Array,
    UInt16Array,
    Int32Array,
    UInt32Array,
    UInt8ClampedArray,
    Float32Array,
    Float64Array,
    NTypedArrayTypes
};

enum AtomicModifyOps {
    AtomicAdd,
    AtomicAnd,
    AtomicExchange,
    AtomicOr,
    AtomicSub,
    AtomicXor,
    NAtomicModifyOps
};

struct TypedArrayOperations {
    typedef ReturnedValue (*Read)(const char *data);
    typedef void (*Write)(char *data, Value value);
    typedef ReturnedValue (*AtomicModify)(char *data, Value value);
    typedef ReturnedValue (*AtomicCompareExchange)(char *data, Value expected, Value v);
    typedef ReturnedValue (*AtomicLoad)(char *data);
    typedef ReturnedValue (*AtomicStore)(char *data, Value value);

    template<typename T>
    static constexpr TypedArrayOperations create(const char *name);
    template<typename T>
    static constexpr TypedArrayOperations createWithAtomics(const char *name);

    int bytesPerElement;
    const char *name;
    Read read;
    Write write;
    AtomicModify atomicModifyOps[AtomicModifyOps::NAtomicModifyOps];
    AtomicCompareExchange atomicCompareExchange;
    AtomicLoad atomicLoad;
    AtomicStore atomicStore;
};

namespace Heap {

#define TypedArrayMembers(class, Member) \
    Member(class, Pointer, ArrayBuffer *, buffer) \
    Member(class, NoMark, const TypedArrayOperations *, type) \
    Member(class, NoMark, uint, byteLength) \
    Member(class, NoMark, uint, byteOffset) \
    Member(class, NoMark, uint, arrayType)

DECLARE_HEAP_OBJECT(TypedArray, Object) {
    DECLARE_MARKOBJECTS(TypedArray)
    using Type = TypedArrayType;

    void init(Type t);
};

struct IntrinsicTypedArrayCtor : FunctionObject {
};

struct TypedArrayCtor : FunctionObject {
    void init(ExecutionEngine *engine, TypedArray::Type t);

    TypedArray::Type type;
};

struct IntrinsicTypedArrayPrototype : Object {
};

struct TypedArrayPrototype : Object {
    inline void init(TypedArray::Type t);
    TypedArray::Type type;
};


}

struct Q_QML_EXPORT TypedArray : Object
{
    V4_OBJECT2(TypedArray, Object)

    static Heap::TypedArray *create(QV4::ExecutionEngine *e, Heap::TypedArray::Type t);

    uint byteOffset() const noexcept { return d()->byteOffset; }
    uint byteLength() const noexcept { return d()->byteLength; }
    int bytesPerElement() const noexcept { return d()->type->bytesPerElement; }
    uint length() const noexcept  { return d()->byteLength / d()->type->bytesPerElement; }

    char *arrayData() noexcept { return d()->buffer->arrayData(); }
    const char *constArrayData() const noexcept { return d()->buffer->constArrayData(); }
    bool hasDetachedArrayData() const noexcept { return d()->buffer->hasDetachedArrayData(); }
    uint arrayDataLength() const noexcept { return d()->buffer->arrayDataLength(); }

    Heap::TypedArray::Type arrayType() const noexcept
    {
        return static_cast<Heap::TypedArray::Type>(d()->arrayType);
    }
    using Object::get;

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualHasProperty(const Managed *m, PropertyKey id);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static bool virtualDefineOwnProperty(Managed *m, PropertyKey id, const Property *p, PropertyAttributes attrs);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);

};

struct IntrinsicTypedArrayCtor: FunctionObject
{
    V4_OBJECT2(IntrinsicTypedArrayCtor, FunctionObject)

    static ReturnedValue method_of(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_from(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct TypedArrayCtor: FunctionObject
{
    V4_OBJECT2(TypedArrayCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct IntrinsicTypedArrayPrototype : Object
{
    V4_OBJECT2(IntrinsicTypedArrayPrototype, Object)
    V4_PROTOTYPE(objectPrototype)

    void init(ExecutionEngine *engine, IntrinsicTypedArrayCtor *ctor);

    static ReturnedValue method_get_buffer(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get_byteLength(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get_byteOffset(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get_length(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_copyWithin(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_entries(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_every(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_fill(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_filter(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_find(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_findIndex(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_forEach(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_includes(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_indexOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_join(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_keys(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_lastIndexOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_map(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_reduce(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_reduceRight(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_reverse(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_some(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_values(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_set(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_slice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_subarray(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toLocaleString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_get_toStringTag(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

};

struct TypedArrayPrototype : Object
{
    V4_OBJECT2(TypedArrayPrototype, Object)
    V4_PROTOTYPE(objectPrototype)

    void init(ExecutionEngine *engine, TypedArrayCtor *ctor);
};

inline void
Heap::TypedArrayPrototype::init(TypedArray::Type t)
{
    Object::init();
    type = t;
}

} // namespace QV4

QT_END_NAMESPACE

#endif
