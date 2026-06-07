// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4VTABLE_P_H
#define QV4VTABLE_P_H

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

#include "qv4global_p.h"
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

class QObject;
namespace QV4 {

struct Lookup;

struct Q_QML_EXPORT OwnPropertyKeyIterator {
    virtual ~OwnPropertyKeyIterator() = 0;
    virtual PropertyKey next(const Object *o, Property *p = nullptr, PropertyAttributes *attrs = nullptr) = 0;
};

struct VTable
{
    typedef void (*Destroy)(Heap::Base *);
    typedef void (*MarkObjects)(Heap::Base *, MarkStack *markStack);
    typedef bool (*IsEqualTo)(Managed *m, Managed *other);

    typedef ReturnedValue (*Get)(const Managed *, PropertyKey id, const Value *receiver, bool *hasProperty);
    typedef bool (*Put)(Managed *, PropertyKey id, const Value &value, Value *receiver);
    typedef bool (*DeleteProperty)(Managed *m, PropertyKey id);
    typedef bool (*HasProperty)(const Managed *m, PropertyKey id);
    typedef PropertyAttributes (*GetOwnProperty)(const Managed *m, PropertyKey id, Property *p);
    typedef bool (*DefineOwnProperty)(Managed *m, PropertyKey id, const Property *p, PropertyAttributes attrs);
    typedef bool (*IsExtensible)(const Managed *);
    typedef bool (*PreventExtensions)(Managed *);
    typedef Heap::Object *(*GetPrototypeOf)(const Managed *);
    typedef bool (*SetPrototypeOf)(Managed *, const Object *);
    typedef qint64 (*GetLength)(const Managed *m);
    typedef OwnPropertyKeyIterator *(*OwnPropertyKeys)(const Object *m, Value *target);
    typedef ReturnedValue (*InstanceOf)(const Object *typeObject, const Value &var);

    typedef ReturnedValue (*Call)(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    typedef void (*CallWithMetaTypes)(const FunctionObject *, QObject *, void **, const QMetaType *, int);
    typedef ReturnedValue (*CallAsConstructor)(const FunctionObject *, const Value *argv, int argc, const Value *newTarget);

    typedef ReturnedValue (*ResolveLookupGetter)(const Object *, ExecutionEngine *, Lookup *);
    typedef bool (*ResolveLookupSetter)(Object *, ExecutionEngine *, Lookup *, const Value &);

    typedef int (*Metacall)(Object *, QMetaObject::Call, int, void **);

    const VTable * const parent;
    quint16 inlinePropertyOffset;
    quint16 nInlineProperties;
    quint8 isExecutionContext;
    quint8 isString;
    quint8 isObject;
    quint8 isTailCallable;
    quint8 isErrorObject;
    quint8 isArrayData;
    quint8 isStringOrSymbol;
    quint8 type;
    quint8 unused[4];
    const char *className;

    Destroy destroy;
    MarkObjects markObjects;
    IsEqualTo isEqualTo;

    Get get;
    Put put;
    DeleteProperty deleteProperty;
    HasProperty hasProperty;
    GetOwnProperty getOwnProperty;
    DefineOwnProperty defineOwnProperty;
    IsExtensible isExtensible;
    PreventExtensions preventExtensions;
    GetPrototypeOf getPrototypeOf;
    SetPrototypeOf setPrototypeOf;
    GetLength getLength;
    OwnPropertyKeys ownPropertyKeys;
    InstanceOf instanceOf;

    Call call;
    CallAsConstructor callAsConstructor;
    CallWithMetaTypes callWithMetaTypes;

    ResolveLookupGetter resolveLookupGetter;
    ResolveLookupSetter resolveLookupSetter;

    Metacall metacall;
};

template<VTable::CallWithMetaTypes call>
struct VTableCallWithMetaTypesWrapper { constexpr static VTable::CallWithMetaTypes c = call; };

template<VTable::Call call>
struct VTableCallWrapper { constexpr static VTable::Call c = call; };

template<class Class>
constexpr VTable::CallWithMetaTypes vtableMetaTypesCallEntry()
{
    // If Class overrides virtualCallWithMetaTypes, return that.
    // Otherwise, if it overrides virtualCall, return convertAndCall.
    // Otherwise, just return whatever the base class had.

    // A simple == on methods is not considered constexpr, so we have to jump through some hoops.

    static_assert(
            std::is_same_v<
                    VTableCallWithMetaTypesWrapper<Class::virtualCallWithMetaTypes>,
                    VTableCallWithMetaTypesWrapper<Class::SuperClass::virtualCallWithMetaTypes>>
                || !std::is_same_v<
                    VTableCallWithMetaTypesWrapper<Class::virtualCallWithMetaTypes>,
                    VTableCallWithMetaTypesWrapper<nullptr>>,
            "You mustn't override virtualCallWithMetaTypes with nullptr");

    static_assert(
            std::is_same_v<
                    VTableCallWithMetaTypesWrapper<Class::virtualConvertAndCall>,
                    VTableCallWithMetaTypesWrapper<Class::SuperClass::virtualConvertAndCall>>
                || !std::is_same_v<
                    VTableCallWithMetaTypesWrapper<Class::virtualConvertAndCall>,
                    VTableCallWithMetaTypesWrapper<nullptr>>,
            "You mustn't override virtualConvertAndCall with nullptr");

    if constexpr (
            std::is_same_v<
                    VTableCallWithMetaTypesWrapper<Class::virtualCallWithMetaTypes>,
                    VTableCallWithMetaTypesWrapper<Class::SuperClass::virtualCallWithMetaTypes>>
            && !std::is_same_v<
                    VTableCallWrapper<Class::virtualCall>,
                    VTableCallWrapper<Class::SuperClass::virtualCall>>) {
        // Converting from metatypes to JS signature is easy.
        return Class::virtualConvertAndCall;
    }

    return Class::virtualCallWithMetaTypes;
}

template<class Class>
constexpr VTable::Call vtableJsTypesCallEntry()
{
    // If Class overrides virtualCall, return that.
    // Otherwise, if it overrides virtualCallWithMetaTypes, fail.
    // (We cannot determine the target types to call virtualCallWithMetaTypes in that case)
    // Otherwise, just return whatever the base class had.

    // A simple == on methods is not considered constexpr, so we have to jump through some hoops.

    static_assert(
            !std::is_same_v<
                    VTableCallWrapper<Class::virtualCall>,
                    VTableCallWrapper<Class::SuperClass::virtualCall>>
                || std::is_same_v<
                    VTableCallWithMetaTypesWrapper<Class::virtualCallWithMetaTypes>,
                    VTableCallWithMetaTypesWrapper<Class::SuperClass::virtualCallWithMetaTypes>>,
            "If you override virtualCallWithMetaTypes, override virtualCall, too");

    static_assert(
            std::is_same_v<
                    VTableCallWrapper<Class::virtualCall>,
                    VTableCallWrapper<Class::SuperClass::virtualCall>>
                || VTableCallWrapper<Class::virtualCall>::c != nullptr,
            "You mustn't override virtualCall with nullptr");

    return Class::virtualCall;
}

struct VTableBase {
protected:
    static constexpr VTable::Destroy virtualDestroy = nullptr;
    static constexpr VTable::IsEqualTo virtualIsEqualTo = nullptr;

    static constexpr VTable::Get virtualGet = nullptr;
    static constexpr VTable::Put virtualPut = nullptr;
    static constexpr VTable::DeleteProperty virtualDeleteProperty = nullptr;
    static constexpr VTable::HasProperty virtualHasProperty = nullptr;
    static constexpr VTable::GetOwnProperty virtualGetOwnProperty = nullptr;
    static constexpr VTable::DefineOwnProperty virtualDefineOwnProperty = nullptr;
    static constexpr VTable::IsExtensible virtualIsExtensible = nullptr;
    static constexpr VTable::PreventExtensions virtualPreventExtensions = nullptr;
    static constexpr VTable::GetPrototypeOf virtualGetPrototypeOf = nullptr;
    static constexpr VTable::SetPrototypeOf virtualSetPrototypeOf = nullptr;
    static constexpr VTable::GetLength virtualGetLength = nullptr;
    static constexpr VTable::OwnPropertyKeys virtualOwnPropertyKeys = nullptr;
    static constexpr VTable::InstanceOf virtualInstanceOf = nullptr;

    static constexpr VTable::Call virtualCall = nullptr;
    static constexpr VTable::CallAsConstructor virtualCallAsConstructor = nullptr;
    static constexpr VTable::CallWithMetaTypes virtualCallWithMetaTypes = nullptr;
    static constexpr VTable::CallWithMetaTypes virtualConvertAndCall = nullptr;

    static constexpr VTable::ResolveLookupGetter virtualResolveLookupGetter = nullptr;
    static constexpr VTable::ResolveLookupSetter virtualResolveLookupSetter = nullptr;

    static constexpr VTable::Metacall virtualMetacall = nullptr;

    template<class Class>
    friend constexpr VTable::CallWithMetaTypes vtableMetaTypesCallEntry();

    template<class Class>
    friend constexpr VTable::Call vtableJsTypesCallEntry();
};

#define DEFINE_MANAGED_VTABLE_INT(classname, parentVTable) \
{     \
    parentVTable, \
    (sizeof(classname::Data) + sizeof(QV4::Value) - 1)/sizeof(QV4::Value), \
    (sizeof(classname::Data) + (classname::NInlineProperties*sizeof(QV4::Value)) + QV4::Chunk::SlotSize - 1)/QV4::Chunk::SlotSize*QV4::Chunk::SlotSize/sizeof(QV4::Value) \
        - (sizeof(classname::Data) + sizeof(QV4::Value) - 1)/sizeof(QV4::Value), \
    classname::IsExecutionContext,          \
    classname::IsString,                    \
    classname::IsObject,                    \
    classname::IsTailCallable,              \
    classname::IsErrorObject,               \
    classname::IsArrayData,                 \
    classname::IsStringOrSymbol,            \
    classname::MyType,                      \
    { 0, 0, 0, 0 },                         \
    #classname, \
    \
    classname::virtualDestroy,              \
    classname::Data::markObjects,           \
    classname::virtualIsEqualTo,            \
    \
    classname::virtualGet,                  \
    classname::virtualPut,                  \
    classname::virtualDeleteProperty,       \
    classname::virtualHasProperty,          \
    classname::virtualGetOwnProperty,       \
    classname::virtualDefineOwnProperty,    \
    classname::virtualIsExtensible,         \
    classname::virtualPreventExtensions,    \
    classname::virtualGetPrototypeOf,       \
    classname::virtualSetPrototypeOf,       \
    classname::virtualGetLength,            \
    classname::virtualOwnPropertyKeys,      \
    classname::virtualInstanceOf,           \
    \
    QV4::vtableJsTypesCallEntry<classname>(),   \
    classname::virtualCallAsConstructor,        \
    QV4::vtableMetaTypesCallEntry<classname>(), \
    \
    classname::virtualResolveLookupGetter,  \
    classname::virtualResolveLookupSetter,  \
    classname::virtualMetacall              \
}

#define DEFINE_MANAGED_VTABLE(classname) \
const QV4::VTable classname::static_vtbl = DEFINE_MANAGED_VTABLE_INT(classname, 0)

#define V4_OBJECT2(DataClass, superClass) \
    private: \
        DataClass() = delete; \
        Q_DISABLE_COPY(DataClass) \
    public: \
        Q_MANAGED_CHECK \
        typedef QV4::Heap::DataClass Data; \
        typedef superClass SuperClass; \
        static const QV4::VTable static_vtbl; \
        static inline const QV4::VTable *staticVTable() { return &static_vtbl; } \
        V4_MANAGED_SIZE_TEST \
        QV4::Heap::DataClass *d_unchecked() const { return static_cast<QV4::Heap::DataClass *>(m()); } \
        QV4::Heap::DataClass *d() const { \
            QV4::Heap::DataClass *dptr = d_unchecked(); \
            dptr->_checkIsInitialized(); \
            return dptr; \
        } \
        static_assert(std::is_trivially_copyable_v<QV4::Heap::DataClass>); \
        static_assert(std::is_trivially_default_constructible_v<QV4::Heap::DataClass>);

#define V4_PROTOTYPE(p) \
    static QV4::Object *defaultPrototype(QV4::ExecutionEngine *e) \
    { return e->p(); }


#define DEFINE_OBJECT_VTABLE_BASE(classname) \
    const QV4::VTable classname::static_vtbl = DEFINE_MANAGED_VTABLE_INT(classname, (std::is_same<classname::SuperClass, Object>::value) ? nullptr : &classname::SuperClass::static_vtbl)

#define DEFINE_OBJECT_VTABLE(classname) \
DEFINE_OBJECT_VTABLE_BASE(classname)

#define DEFINE_OBJECT_TEMPLATE_VTABLE(classname) \
template<> DEFINE_OBJECT_VTABLE_BASE(classname)

}

QT_END_NAMESPACE

#endif
