// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4LOOKUP_H
#define QV4LOOKUP_H

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

#include "qv4engine_p.h"
#include "qv4object_p.h"
#include "qv4internalclass_p.h"
#include "qv4qmlcontext_p.h"
#include <private/qqmltypewrapper_p.h>
#include <private/qv4mm_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {
    struct QObjectMethod;
}

template <typename T, int PhantomTag>
using HeapObjectWrapper = WriteBarrier::HeapObjectWrapper<T, PhantomTag>;

// Note: We cannot hide the copy ctor and assignment operator of this class because it needs to
//       be trivially copyable. But you should never ever copy it. There are refcounted members
//       in there.
struct Q_QML_EXPORT Lookup {
    enum class Call: quint16 {
        ContextGetterContextObjectMethod,
        ContextGetterContextObjectProperty,
        ContextGetterGeneric,
        ContextGetterIdObject,
        ContextGetterIdObjectInParentContext,
        ContextGetterInGlobalObject,
        ContextGetterInParentContextHierarchy,
        ContextGetterScopeObjectMethod,
        ContextGetterScopeObjectProperty,
        ContextGetterScopeObjectPropertyFallback,
        ContextGetterScript,
        ContextGetterSingleton,
        ContextGetterType,
        ContextGetterValueSingleton,

        GlobalGetterGeneric,
        GlobalGetterProto,
        GlobalGetterProtoAccessor,

        Getter0Inline,
        Getter0InlineGetter0Inline,
        Getter0InlineGetter0MemberData,
        Getter0MemberData,
        Getter0MemberDataGetter0MemberData,
        GetterAccessor,
        GetterAccessorPrimitive,
        GetterEnum,
        GetterEnumValue,
        GetterGeneric,
        GetterIndexed,
        GetterProto,
        GetterProtoAccessor,
        GetterProtoAccessorTwoClasses,
        GetterProtoPrimitive,
        GetterProtoTwoClasses,
        GetterQObjectAttached,
        GetterQObjectMethod,
        GetterQObjectMethodFallback,
        GetterQObjectProperty,
        GetterQObjectPropertyFallback,
        GetterSingletonMethod,
        GetterSingletonProperty,
        GetterStringLength,
        GetterValueTypeProperty,

        Setter0Inline,
        Setter0MemberData,
        Setter0Setter0,
        SetterArrayLength,
        SetterGeneric,
        SetterInsert,
        SetterQObjectProperty,
        SetterQObjectPropertyFallback,
        SetterValueTypeProperty,
    };

    // NOTE: gc assumes the first two entries in the struct are pointers to heap objects or null
    //       or that the least significant bit is 1 (see the Lookup::markObjects function)
    union {
        struct {
            Heap::Base *h1;
            Heap::Base *h2;
            quintptr unused;
            quintptr unused2;
        } markDef;
        struct {
            HeapObjectWrapper<Heap::InternalClass, 0> ic;
            quintptr unused;
            uint index;
            uint offset;
        } objectLookup;
        struct {
            quintptr protoId;
            quintptr _unused;
            const Value *data;
            const QtPrivate::QMetaTypeInterface *metaType;
        } protoLookup;
        struct {
            HeapObjectWrapper<Heap::InternalClass, 1> ic;
            HeapObjectWrapper<Heap::InternalClass, 2> ic2;
            uint offset;
            uint offset2;
        } objectLookupTwoClasses;
        struct {
            quintptr protoId;
            quintptr protoId2;
            const Value *data;
            const Value *data2;
        } protoLookupTwoClasses;
        struct {
            // Make sure the next two values are in sync with protoLookup
            quintptr protoId;
            HeapObjectWrapper<Heap::Object, 3> proto;
            const Value *data;
            quintptr type;
        } primitiveLookup;
        struct {
            HeapObjectWrapper<Heap::InternalClass, 4> newClass;
            quintptr protoId;
            uint offset;
            uint unused;
        } insertionLookup;
        struct {
            quintptr _unused;
            quintptr _unused2;
            uint index;
            uint unused;
        } indexedLookup;
        struct {
            HeapObjectWrapper<Heap::InternalClass, 5> ic;
            HeapObjectWrapper<Heap::InternalClass, 6> qmlTypeIc; // only used when lookup goes through QQmlTypeWrapper
            const QQmlPropertyCache *propertyCache;
            const QQmlPropertyData *propertyData;
        } qobjectLookup;
        struct {
            HeapObjectWrapper<Heap::InternalClass, 7> ic;
            HeapObjectWrapper<Heap::QObjectMethod, 8> method;
            const QQmlPropertyCache *propertyCache;
            const QQmlPropertyData *propertyData;
        } qobjectMethodLookup;
        struct {
            // NB: None of this is actually cache-able. The metaobject may change at any time.
            //     We invalidate this data every time the lookup is invoked and thereby force a
            //     re-initialization next time.

            quintptr isConstant; // This is a bool, encoded as 0 or 1. Both values are ignored by gc
            quintptr metaObject; // a (const QMetaObject* & 1) or nullptr
            int coreIndex;
            int notifyIndex;
        } qobjectFallbackLookup;
        struct {
            HeapObjectWrapper<Heap::InternalClass, 9> ic;
            quintptr metaObject; // a (const QMetaObject* & 1) or nullptr
            const QtPrivate::QMetaTypeInterface *metaType; // cannot use QMetaType; class must be trivial
            quint16 coreIndex;
            bool isFunction;
            bool isEnum;
        } qgadgetLookup;
        struct {
            quintptr unused1;
            quintptr unused2;
            int scriptIndex;
        } qmlContextScriptLookup;
        struct {
            HeapObjectWrapper<Heap::Base, 10> singletonObject;
            quintptr unused2;
            QV4::ReturnedValue singletonValue;
        } qmlContextSingletonLookup;
        struct {
            quintptr unused1;
            quintptr unused2;
            int objectId;
        } qmlContextIdObjectLookup;
        struct {
            // Same as protoLookup, as used for global lookups
            quintptr reserved1;
            quintptr reserved2;
            quintptr reserved3;
            Call getterTrampoline;
        } qmlContextGlobalLookup;
        struct {
            HeapObjectWrapper<Heap::Base, 11> qmlTypeWrapper;
            quintptr unused2;
        } qmlTypeLookup;
        struct {
            HeapObjectWrapper<Heap::InternalClass, 12> ic;
            quintptr unused;
            ReturnedValue encodedEnumValue;
            const QtPrivate::QMetaTypeInterface *metaType;
        } qmlEnumValueLookup;
        struct {
            HeapObjectWrapper<Heap::InternalClass, 13> ic;
            HeapObjectWrapper<Heap::Object, 14> qmlEnumWrapper;
        } qmlEnumWrapperLookup;
    };

    Call call;
    quint16 padding;

    uint nameIndex: 28; // Same number of bits we store in the compilation unit for name indices
    uint forCall: 1;    // Whether we are looking up a value in order to call it right away
    uint asVariant: 1;  // Whether all types are to be converted from/to QVariant
    uint reserved: 2;

    ReturnedValue resolveGetter(ExecutionEngine *engine, const Object *object);
    ReturnedValue resolvePrimitiveGetter(ExecutionEngine *engine, const Value &object);
    ReturnedValue resolveGlobalGetter(ExecutionEngine *engine);
    void resolveProtoGetter(PropertyKey name, const Heap::Object *proto);

    static ReturnedValue getterGeneric(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterTwoClasses(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterFallback(Lookup *lookup, ExecutionEngine *engine, const Value &object);

    static ReturnedValue getter0MemberData(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getter0Inline(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterProto(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getter0Inlinegetter0Inline(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getter0Inlinegetter0MemberData(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getter0MemberDatagetter0MemberData(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterProtoTwoClasses(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterAccessor(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterProtoAccessor(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterProtoAccessorTwoClasses(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterIndexed(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterQObject(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterQObjectMethod(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterFallbackMethod(Lookup *lookup, ExecutionEngine *engine, const Value &object);

    static ReturnedValue getterValueType(Lookup *lookup, ExecutionEngine *engine, const Value &object);

    static ReturnedValue primitiveGetterProto(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue primitiveGetterAccessor(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static ReturnedValue stringLengthGetter(Lookup *lookup, ExecutionEngine *engine, const Value &object);

    static ReturnedValue globalGetterGeneric(Lookup *lookup, ExecutionEngine *engine);
    static ReturnedValue globalGetterProto(Lookup *lookup, ExecutionEngine *engine);
    static ReturnedValue globalGetterProtoAccessor(Lookup *lookup, ExecutionEngine *engine);

    bool resolveSetter(ExecutionEngine *engine, Object *object, const Value &value);
    static bool setterGeneric(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value);
    Q_NEVER_INLINE static bool setterTwoClasses(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterFallback(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setter0MemberData(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setter0Inline(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setter0setter0(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterInsert(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterQObject(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value);
    static bool arrayLengthSetter(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value);

    void markObjects(MarkStack *stack) {
        if (markDef.h1 && !(reinterpret_cast<quintptr>(markDef.h1) & 1))
            markDef.h1->mark(stack);
        if (markDef.h2 && !(reinterpret_cast<quintptr>(markDef.h2) & 1))
            markDef.h2->mark(stack);
    }

    ReturnedValue contextGetter(ExecutionEngine *engine, Value *base)
    {
        switch (call) {
        case Call::ContextGetterContextObjectMethod:
            return QQmlContextWrapper::lookupContextObjectMethod(this, engine, base);
        case Call::ContextGetterContextObjectProperty:
            return QQmlContextWrapper::lookupContextObjectProperty(this, engine, base);
        case Call::ContextGetterGeneric:
            return QQmlContextWrapper::resolveQmlContextPropertyLookupGetter(this, engine, base);
        case Call::ContextGetterIdObject:
            return QQmlContextWrapper::lookupIdObject(this, engine, base);
        case Call::ContextGetterIdObjectInParentContext:
            return QQmlContextWrapper::lookupIdObjectInParentContext(this, engine, base);
        case Call::ContextGetterInGlobalObject:
            return QQmlContextWrapper::lookupInGlobalObject(this, engine, base);
        case Call::ContextGetterInParentContextHierarchy:
            return QQmlContextWrapper::lookupInParentContextHierarchy(this, engine, base);
        case Call::ContextGetterScopeObjectMethod:
            return QQmlContextWrapper::lookupScopeObjectMethod(this, engine, base);
        case Call::ContextGetterScopeObjectProperty:
            return QQmlContextWrapper::lookupScopeObjectProperty(this, engine, base);
        case Call::ContextGetterSingleton:
            return QQmlContextWrapper::lookupSingleton(this, engine, base);
        case Call::ContextGetterScript:
            return QQmlContextWrapper::lookupScript(this, engine, base);
        case Call::ContextGetterType:
            return QQmlContextWrapper::lookupType(this, engine, base);
        case Call::ContextGetterValueSingleton:
            return QQmlContextWrapper::lookupValueSingleton(this, engine, base);
        default:
            break;
        }

        Q_UNREACHABLE_RETURN(Encode::undefined());
    }

    static ReturnedValue doCallGlobal(Call call, Lookup *lookup, ExecutionEngine *engine)
    {
        switch (call) {
        case Call::GlobalGetterGeneric:
            return globalGetterGeneric(lookup, engine);
        case Call::GlobalGetterProto:
            return globalGetterProto(lookup, engine);
        case Call::GlobalGetterProtoAccessor:
            return globalGetterProtoAccessor(lookup, engine);
        default:
            break;
        }

        Q_UNREACHABLE_RETURN(Encode::undefined());
    }

    ReturnedValue globalGetter(ExecutionEngine *engine)
    {
        return doCallGlobal(call, this, engine);
    }

    ReturnedValue getter(ExecutionEngine *engine, const Value &object)
    {
        switch (call) {
        case Call::Getter0Inline:
            return getter0Inline(this, engine, object);
        case Call::Getter0InlineGetter0Inline:
            return getter0Inlinegetter0Inline(this, engine, object);
        case Call::Getter0InlineGetter0MemberData:
            return getter0Inlinegetter0MemberData(this, engine, object);
        case Call::Getter0MemberData:
            return getter0MemberData(this, engine, object);
        case Call::Getter0MemberDataGetter0MemberData:
            return getter0MemberDatagetter0MemberData(this, engine, object);
        case Call::GetterAccessor:
            return getterAccessor(this, engine, object);
        case Call::GetterAccessorPrimitive:
            return primitiveGetterAccessor(this, engine, object);
        case Call::GetterEnumValue:
            return QQmlTypeWrapper::lookupEnumValue(this, engine, object);
        case Call::GetterQObjectPropertyFallback:
            return getterFallback(this, engine, object);
        case Call::GetterQObjectMethodFallback:
            return getterFallbackMethod(this, engine, object);
        case Call::GetterGeneric:
            return getterGeneric(this, engine, object);
        case Call::GetterIndexed:
            return getterIndexed(this, engine, object);
        case Call::GetterProto:
            return getterProto(this, engine, object);
        case Call::GetterProtoAccessor:
            return getterProtoAccessor(this, engine, object);
        case Call::GetterProtoAccessorTwoClasses:
            return getterProtoAccessorTwoClasses(this, engine, object);
        case Call::GetterProtoPrimitive:
            return primitiveGetterProto(this, engine, object);
        case Call::GetterProtoTwoClasses:
            return getterProtoTwoClasses(this, engine, object);
        case Call::GetterQObjectProperty:
            return getterQObject(this, engine, object);
        case Call::GetterQObjectAttached:
            // TODO: more specific implementation for interpreter / JIT
            return getterGeneric(this, engine, object);
        case Call::GetterQObjectMethod:
            return getterQObjectMethod(this, engine, object);
        case Call::GetterSingletonMethod:
            return QQmlTypeWrapper::lookupSingletonMethod(this, engine, object);
        case Call::GetterSingletonProperty:
            return QQmlTypeWrapper::lookupSingletonProperty(this, engine, object);
        case Call::GetterStringLength:
            return stringLengthGetter(this, engine, object);
        case Call::GetterValueTypeProperty:
            return getterValueType(this, engine, object);
        case Call::GetterEnum:
            return QQmlTypeWrapper::lookupEnum(this, engine, object);
        default:
            break;
        }

        Q_UNREACHABLE_RETURN(Encode::undefined());
    }

    bool setter(ExecutionEngine *engine, Value &object, const Value &value)
    {
        switch (call) {
        case Call::Setter0Inline:
            return setter0Inline(this, engine, object, value);
        case Call::Setter0MemberData:
            return setter0MemberData(this, engine, object, value);
        case Call::Setter0Setter0:
            return setter0setter0(this, engine, object, value);
        case Call::SetterArrayLength:
            return arrayLengthSetter(this, engine, object, value);
        case Call::SetterQObjectPropertyFallback:
            return setterFallback(this, engine, object, value);
        case Call::SetterGeneric:
            return setterGeneric(this, engine, object, value);
        case Call::SetterInsert:
            return setterInsert(this, engine, object, value);
        case Call::SetterQObjectProperty:
            return setterQObject(this, engine, object, value);
        case Call::SetterValueTypeProperty:
            // TODO: more specific implementation for interpreter / JIT
            return setterFallback(this, engine, object, value);
        default:
            break;
        }

        Q_UNREACHABLE_RETURN(Encode::undefined());
    }

    void releasePropertyCache()
    {
        switch (call) {
        case Call::ContextGetterContextObjectProperty:
        case Call::ContextGetterScopeObjectProperty:
        case Call::GetterQObjectProperty:
        case Call::GetterSingletonProperty:
        case Call::SetterQObjectProperty:
            if (const QQmlPropertyCache *pc = qobjectLookup.propertyCache)
                pc->release();
            break;
        case Call::ContextGetterContextObjectMethod:
        case Call::ContextGetterScopeObjectMethod:
        case Call::GetterQObjectMethod:
        case Call::GetterSingletonMethod:
            if (const QQmlPropertyCache *pc = qobjectMethodLookup.propertyCache)
                pc->release();
            break;
        default:
            break;
        }
    }
};

Q_STATIC_ASSERT(std::is_standard_layout<Lookup>::value);

inline void setupQObjectLookup(
        Lookup *lookup, const QQmlData *ddata, const QQmlPropertyData *propertyData)
{
    lookup->releasePropertyCache();
    Q_ASSERT(!ddata->propertyCache.isNull());
    lookup->qobjectLookup.propertyCache = ddata->propertyCache.data();
    lookup->qobjectLookup.propertyCache->addref();
    lookup->qobjectLookup.propertyData = propertyData;
}

inline void setupQObjectLookup(
        Lookup *lookup, const QQmlData *ddata, const QQmlPropertyData *propertyData,
        const Object *self)
{
    setupQObjectLookup(lookup, ddata, propertyData);
    lookup->qobjectLookup.ic.set(self->engine(), self->internalClass());
}


inline void setupQObjectLookup(
        Lookup *lookup, const QQmlData *ddata, const QQmlPropertyData *propertyData,
        const Object *self, const Object *qmlType)
{
    setupQObjectLookup(lookup, ddata, propertyData, self);
    lookup->qobjectLookup.qmlTypeIc.set(self->engine(), qmlType->internalClass());
}

// template parameter is an ugly trick to avoid pulling in the QObjectMethod header here
template<typename QObjectMethod = Heap::QObjectMethod>
inline void setupQObjectMethodLookup(
        Lookup *lookup, const QQmlPropertyCache::ConstPtr &propertyCache,
        const QQmlPropertyData *propertyData, const Object *self, QObjectMethod *method)
{
    lookup->releasePropertyCache();
    Q_ASSERT(!propertyCache.isNull());
    auto engine = self->engine();
    lookup->qobjectMethodLookup.method.set(engine, method);
    lookup->qobjectMethodLookup.ic.set(engine, self->internalClass());
    lookup->qobjectMethodLookup.propertyCache = propertyCache.data();
    lookup->qobjectMethodLookup.propertyCache->addref();
    lookup->qobjectMethodLookup.propertyData = propertyData;
}

inline bool qualifiesForMethodLookup(const QQmlPropertyData *propertyData)
{
    return propertyData->isFunction()
            && !propertyData->isSignalHandler() // TODO: Optimize SignalHandler, too
            && !propertyData->isVMEFunction() // Handled by QObjectLookup
            && !propertyData->isVarProperty();
}

}

QT_END_NAMESPACE

#endif
