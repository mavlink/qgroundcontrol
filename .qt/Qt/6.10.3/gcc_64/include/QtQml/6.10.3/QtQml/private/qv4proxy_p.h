// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4PROXY_P_H
#define QV4PROXY_P_H

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

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

#define ProxyObjectMembers(class, Member) \
    Member(class, Pointer, Object *, target) \
    Member(class, Pointer, Object *, handler)

DECLARE_HEAP_OBJECT(ProxyObject, FunctionObject) {
    DECLARE_MARKOBJECTS(ProxyObject)

    void init(const QV4::Object *target, const QV4::Object *handler);
};

struct ProxyFunctionObject : ProxyObject {
    void init(const QV4::FunctionObject *target, const QV4::Object *handler);
};

struct ProxyConstructorObject : ProxyFunctionObject {};

#define ProxyMembers(class, Member) \
    Member(class, Pointer, Symbol *, revokableProxySymbol) \

DECLARE_HEAP_OBJECT(Proxy, FunctionObject) {
    DECLARE_MARKOBJECTS(Proxy)

    void init(ExecutionEngine *engine);
};

}

/*
 * The inheritance from FunctionObject is a hack. Regular proxy objects are no function objects.
 * But this helps implement the proxy for function objects, where we need this and thus gives us
 * all the virtual methods from ProxyObject without having to duplicate them.
 *
 * But it does require a few hacks to make sure we don't recognize regular proxy objects as function
 * objects in the runtime.
 */
struct ProxyObject : FunctionObject {
    V4_OBJECT2(ProxyObject, Object)
    Q_MANAGED_TYPE(ProxyObject)
    V4_INTERNALCLASS(ProxyObject)

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static bool virtualDeleteProperty(Managed *m, PropertyKey id);
    static bool virtualHasProperty(const Managed *m, PropertyKey id);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);
    static bool virtualDefineOwnProperty(Managed *m, PropertyKey id, const Property *p, PropertyAttributes attrs);
    static bool virtualIsExtensible(const Managed *m);
    static bool virtualPreventExtensions(Managed *);
    static Heap::Object *virtualGetPrototypeOf(const Managed *);
    static bool virtualSetPrototypeOf(Managed *, const Object *);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *iteratorTarget);
};

struct ProxyFunctionObject : ProxyObject {
    V4_OBJECT2(ProxyFunctionObject, FunctionObject)
    Q_MANAGED_TYPE(ProxyObject)
    V4_INTERNALCLASS(ProxyFunctionObject)

    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct ProxyConstructorObject : ProxyFunctionObject {
    V4_OBJECT2(ProxyConstructorObject, ProxyFunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
};

struct Proxy : FunctionObject
{
    V4_OBJECT2(Proxy, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_revocable(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_revoke(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
