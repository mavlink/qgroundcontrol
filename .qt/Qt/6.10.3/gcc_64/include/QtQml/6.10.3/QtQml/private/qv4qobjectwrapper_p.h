// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4QOBJECTWRAPPER_P_H
#define QV4QOBJECTWRAPPER_P_H

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

#include <private/qbipointer_p.h>
#include <private/qintrusivelist_p.h>
#include <private/qqmldata_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4value_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qhash.h>

#include <utility>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcBuiltinsBindingRemoval)

class QObject;
class QQmlData;
class QQmlPropertyCache;
class QQmlPropertyData;
class QQmlObjectOrGadget;

namespace QV4 {
struct QObjectSlotDispatcher;

namespace Heap {

struct QQmlValueTypeWrapper;

struct Q_QML_EXPORT QObjectWrapper : Object {
    void init(QObject *object)
    {
        Object::init();
        qObj.init(object);
    }

    void destroy() {
        qObj.destroy();
        Object::destroy();
    }

    QObject *object() const { return qObj.data(); }
    static void markObjects(Heap::Base *that, MarkStack *markStack);

private:
    QV4QPointer<QObject> qObj;
};

#define QObjectMethodMembers(class, Member) \
    Member(class, Pointer, Object *, wrapper) \

DECLARE_EXPORTED_HEAP_OBJECT(QObjectMethod, FunctionObject) {
    DECLARE_MARKOBJECTS(QObjectMethod)

    QQmlPropertyData *methods;
    alignas(alignof(QQmlPropertyData)) std::byte _singleMethod[sizeof(QQmlPropertyData)];
    int methodCount;
    int index;

    void init(QV4::ExecutionEngine *engine, Object *wrapper, int index);
    void destroy()
    {
        if (methods != reinterpret_cast<const QQmlPropertyData *>(&_singleMethod))
            delete[] methods;
        FunctionObject::destroy();
    }

    void ensureMethodsCache(const QMetaObject *thisMeta);
    QString name() const;

    const QMetaObject *metaObject() const;
    QObject *object() const;

    bool isDetached() const;
    bool isAttachedTo(QObject *o) const;

    enum ThisObjectMode {
        Invalid,
        Included,
        Explicit,
    };

    QV4::Heap::QObjectMethod::ThisObjectMode checkThisObject(const QMetaObject *thisMeta) const;
};

struct QmlSignalHandler : Object {
    void init(QObject *object, int signalIndex);
    void destroy() {
        qObj.destroy();
        Object::destroy();
    }
    int signalIndex;

    QObject *object() const { return qObj.data(); }
    void setObject(QObject *o) { qObj = o; }

private:
    QV4QPointer<QObject> qObj;
};

}

struct Q_QML_EXPORT QObjectWrapper : public Object
{
    V4_OBJECT2(QObjectWrapper, Object)
    Q_MANAGED_TYPE(V4QObjectWrapper)
    V4_NEEDS_DESTROY

    enum Flag {
        NoFlag         = 0x0,
        CheckRevision  = 0x1,
        AttachMethods   = 0x2,
        AllowOverride  = 0x4,
        IncludeImports = 0x8,
    };

    Q_DECLARE_FLAGS(Flags, Flag);

    static void initializeBindings(ExecutionEngine *engine);

    const QMetaObject *metaObject() const
    {
        if (QObject *o = object())
            return o->metaObject();
        return nullptr;
    }

    QObject *object() const { return d()->object(); }

    ReturnedValue getQmlProperty(
            const QQmlRefPointer<QQmlContextData> &qmlContext, String *name,
            Flags flags, bool *hasProperty = nullptr) const;

    static ReturnedValue getQmlProperty(
            ExecutionEngine *engine, const QQmlRefPointer<QQmlContextData> &qmlContext,
            Heap::Object *wrapper, QObject *object, String *name, Flags flags,
            bool *hasProperty = nullptr, const QQmlPropertyData **property = nullptr);

    static bool setQmlProperty(
            ExecutionEngine *engine, const QQmlRefPointer<QQmlContextData> &qmlContext,
            QObject *object, String *name, Flags flags, const Value &value);

    Q_NODISCARD_X("Use ensureWrapper if you don't need the return value")
    static ReturnedValue wrap(ExecutionEngine *engine, QObject *object);
    Q_NODISCARD_X("Throwing the const wrapper away can cause it to be garbage collected")
    static ReturnedValue wrapConst(ExecutionEngine *engine, QObject *object);
    static void ensureWrapper(ExecutionEngine *engine, QObject *object);
    static void markWrapper(QObject *object, MarkStack *markStack);

    using Object::get;

    static void setProperty(ExecutionEngine *engine, QObject *object, int propertyIndex, const Value &value);
    void setProperty(ExecutionEngine *engine, int propertyIndex, const Value &value);
    static void setProperty(
            ExecutionEngine *engine, QObject *object,
            const QQmlPropertyData *property, const Value &value);

    void destroyObject(bool lastCall);

    static ReturnedValue getProperty(
            ExecutionEngine *engine, Heap::Object *wrapper, QObject *object,
            const QQmlPropertyData *property, Flags flags);

    static ReturnedValue virtualResolveLookupGetter(const Object *object, ExecutionEngine *engine, Lookup *lookup);

    template <typename ReversalFunctor> static ReturnedValue lookupPropertyGetterImpl(
            Lookup *l, ExecutionEngine *engine, const Value &object,
            Flags flags, ReversalFunctor revert);
    template <typename ReversalFunctor> static ReturnedValue lookupMethodGetterImpl(
            Lookup *l, ExecutionEngine *engine, const Value &object,
            Flags flags, ReversalFunctor revert);
    static bool virtualResolveLookupSetter(
            Object *object, ExecutionEngine *engine, Lookup *lookup, const Value &value);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);

    static int virtualMetacall(Object *object, QMetaObject::Call call, int index, void **a);

    static QString objectToString(
            ExecutionEngine *engine, const QMetaObject *metaObject, QObject *object);

protected:
    static bool virtualIsEqualTo(Managed *that, Managed *o);
    static ReturnedValue create(ExecutionEngine *engine, QObject *object);

    static const QQmlPropertyData *findProperty(
            QObject *o, const QQmlRefPointer<QQmlContextData> &qmlContext,
            String *name, Flags flags, QQmlPropertyData *local);

    const QQmlPropertyData *findProperty(
            const QQmlRefPointer<QQmlContextData> &qmlContext,
            String *name, Flags flags, QQmlPropertyData *local) const;

    static ReturnedValue virtualGet(
            const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);

    static ReturnedValue method_connect(
            const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_disconnect(
            const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

private:
    Q_NEVER_INLINE static ReturnedValue wrap_slowPath(ExecutionEngine *engine, QObject *object);
    Q_NEVER_INLINE static ReturnedValue wrapConst_slowPath(ExecutionEngine *engine, QObject *object);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QObjectWrapper::Flags)

// We generally musn't pass ReturnedValue as arguments to other functions.
// In this case, we do it solely for marking purposes so it's fine.
inline void markIfPastMarkWeakValues(ExecutionEngine *engine, ReturnedValue rv)
{
    const auto gcState = engine->memoryManager->gcStateMachine->state;
    if (gcState != GCStateMachine::Invalid && gcState >= GCState::MarkWeakValues) {
        QV4::WriteBarrier::markCustom(engine, [rv](QV4::MarkStack *ms) {
            auto *m = StaticValue::fromReturnedValue(rv).m();
            m->mark(ms);
        });
    }
}

inline ReturnedValue QObjectWrapper::wrap(ExecutionEngine *engine, QObject *object)
{
    if (Q_UNLIKELY(QQmlData::wasDeleted(object)))
        return QV4::Encode::null();

    auto ddata = QQmlData::get(object);
    if (Q_LIKELY(ddata && ddata->jsEngineId == engine->m_engineId && !ddata->jsWrapper.isUndefined())) {
        // We own the JS object
        return ddata->jsWrapper.value();
    }

    const auto rv = wrap_slowPath(engine, object);
    markIfPastMarkWeakValues(engine, rv);
    return rv;
}

// Unfortunately we still need a non-const QObject* here because QQmlData needs to register itself in QObjectPrivate.
inline ReturnedValue QObjectWrapper::wrapConst(ExecutionEngine *engine, QObject *object)
{
    if (Q_UNLIKELY(QQmlData::wasDeleted(object)))
        return QV4::Encode::null();

    const auto rv = wrapConst_slowPath(engine, object);
    markIfPastMarkWeakValues(engine, rv);
    return rv;
}

inline bool canConvert(const QQmlPropertyCache *fromMo, const QQmlPropertyCache *toMo)
{
    while (fromMo) {
        if (fromMo == toMo)
            return true;
        fromMo = fromMo->parent().data();
    }
    return false;
}

template <typename ReversalFunctor>
inline ReturnedValue QObjectWrapper::lookupPropertyGetterImpl(
        Lookup *lookup, ExecutionEngine *engine, const Value &object,
        QObjectWrapper::Flags flags, ReversalFunctor revertLookup)
{
    // we can safely cast to a QV4::Object here. If object is something else,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (!o || o->internalClass != lookup->qobjectLookup.ic)
        return revertLookup();

    Heap::QObjectWrapper *This = static_cast<Heap::QObjectWrapper *>(o);
    QObject *qobj = This->object();
    if (QQmlData::wasDeleted(qobj))
        return QV4::Encode::undefined();

    QQmlData *ddata = QQmlData::get(qobj, /*create*/false);
    if (!ddata)
        return revertLookup();

    const QQmlPropertyData *property = lookup->qobjectLookup.propertyData;
    if (ddata->propertyCache.data() != lookup->qobjectLookup.propertyCache) {
        // If the property is overridden and the lookup allows overrides to be considered,
        // we have to revert here and redo the lookup from scratch.
        if (property->isOverridden()
                && ((flags & AllowOverride)
                    || property->isFunction()
                    || property->isSignalHandler())) {
            return revertLookup();
        }

        if (!canConvert(ddata->propertyCache.data(), lookup->qobjectLookup.propertyCache))
            return revertLookup();
    }

    return getProperty(engine, This, qobj, property, flags);
}

template <typename ReversalFunctor>
inline ReturnedValue QObjectWrapper::lookupMethodGetterImpl(
        Lookup *lookup, ExecutionEngine *engine, const Value &object,
        QObjectWrapper::Flags flags, ReversalFunctor revertLookup)
{
    // we can safely cast to a QV4::Object here. If object is something else,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (!o || o->internalClass != lookup->qobjectMethodLookup.ic)
        return revertLookup();

    Heap::QObjectWrapper *This = static_cast<Heap::QObjectWrapper *>(o);
    QObject *qobj = This->object();
    if (QQmlData::wasDeleted(qobj))
        return QV4::Encode::undefined();

    QQmlData *ddata = QQmlData::get(qobj, /*create*/false);
    if (!ddata)
        return revertLookup();

    const QQmlPropertyData *property = lookup->qobjectMethodLookup.propertyData;
    if (ddata->propertyCache.data() != lookup->qobjectMethodLookup.propertyCache) {
        if (property && property->isOverridden())
            return revertLookup();

        if (!canConvert(ddata->propertyCache.data(), lookup->qobjectMethodLookup.propertyCache))
            return revertLookup();
    }

    if (Heap::QObjectMethod *method = lookup->qobjectMethodLookup.method) {
        if (method->isDetached())
            return method->asReturnedValue();
    }

    if (!property) // was toString() or destroy()
        return revertLookup();

    QV4::Scope scope(engine);
    QV4::ScopedValue v(scope, getProperty(engine, This, qobj, property, flags));
    if (!v->as<QObjectMethod>())
        return revertLookup();

    lookup->qobjectMethodLookup.method.set(engine, static_cast<Heap::QObjectMethod *>(v->heapObject()));
    return v->asReturnedValue();
}

struct QQmlValueTypeWrapper;

struct Q_QML_EXPORT QObjectMethod : public QV4::FunctionObject
{
    V4_OBJECT2(QObjectMethod, QV4::FunctionObject)
    V4_NEEDS_DESTROY

    enum { DestroyMethod = -1, ToStringMethod = -2 };

    static ReturnedValue create(ExecutionEngine *engine, Heap::Object *wrapper, int index);
    static ReturnedValue create(
            ExecutionEngine *engine, Heap::QQmlValueTypeWrapper *valueType, int index);
    static ReturnedValue create(
            ExecutionEngine *engine, Heap::QObjectMethod *cloneFrom,
            Heap::Object *wrapper, Heap::Object *object);

    int methodIndex() const { return d()->index; }
    QObject *object() const { return d()->object(); }

    QV4::ReturnedValue method_toString(QV4::ExecutionEngine *engine, QObject *o) const;
    QV4::ReturnedValue method_destroy(
            QV4::ExecutionEngine *ctx, QObject *o, const Value *args, int argc) const;
    bool method_destroy(
            QV4::ExecutionEngine *engine, QObject *o, int delay) const;

    static ReturnedValue virtualCall(
            const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static void virtualCallWithMetaTypes(
            const FunctionObject *m, QObject *thisObject,
            void **argv, const QMetaType *types, int argc);

    ReturnedValue callInternal(
            const Value *thisObject, const Value *argv, int argc) const;
    void callInternalWithMetaTypes(
            QObject *thisObject, void **argv, const QMetaType *types, int argc) const;

    static std::pair<QObject *, int> extractQtMethod(const QV4::FunctionObject *function);

    static bool isExactMatch(
            const QMetaMethod &method, void **argv, int argc, const QMetaType *types);

private:
    friend struct QMetaObjectWrapper;

    static const QQmlPropertyData *resolveOverloaded(
            const QQmlObjectOrGadget &object, const QQmlPropertyData *methods, int methodCount,
            ExecutionEngine *engine, CallData *callArgs);

    static const QQmlPropertyData *resolveOverloaded(
            const QQmlPropertyData *methods, int methodCount,
            void **argv, int argc, const QMetaType *types);

    static ReturnedValue callPrecise(
            const QQmlObjectOrGadget &object, const QQmlPropertyData &data,
            ExecutionEngine *engine, CallData *callArgs,
            QMetaObject::Call callType = QMetaObject::InvokeMetaMethod);
};

struct Q_QML_EXPORT QmlSignalHandler : public QV4::Object
{
    V4_OBJECT2(QmlSignalHandler, QV4::Object)
    V4_PROTOTYPE(signalHandlerPrototype)
    V4_NEEDS_DESTROY

    int signalIndex() const { return d()->signalIndex; }
    QObject *object() const { return d()->object(); }

    ReturnedValue call(const Value *thisObject, const Value *argv, int argc) const;

    static void initProto(ExecutionEngine *v4);
};

using QObjectBiPointer = QBiPointer<QObject, const QObject>;

class MultiplyWrappedQObjectMap : public QObject,
                                  private QHash<QObjectBiPointer, QV4::WeakValue>
{
    Q_OBJECT
public:
    typedef QHash<QObjectBiPointer, QV4::WeakValue>::ConstIterator ConstIterator;
    typedef QHash<QObjectBiPointer, QV4::WeakValue>::Iterator Iterator;

    using value_type = QHash<QObjectBiPointer, QV4::WeakValue>::value_type;

    ConstIterator begin() const { return QHash<QObjectBiPointer, QV4::WeakValue>::constBegin(); }
    Iterator begin() { return QHash<QObjectBiPointer, QV4::WeakValue>::begin(); }
    ConstIterator end() const { return QHash<QObjectBiPointer, QV4::WeakValue>::constEnd(); }
    Iterator end() { return QHash<QObjectBiPointer, QV4::WeakValue>::end(); }

    template<typename Pointer>
    void insert(Pointer key, Heap::Object *value)
    {
        QHash<QObjectBiPointer, WeakValue>::operator[](key).set(value->internalClass->engine, value);
        connect(key, SIGNAL(destroyed(QObject*)), this, SLOT(removeDestroyedObject(QObject*)));
    }

    template<typename Pointer>
    ReturnedValue value(Pointer key) const
    {
        ConstIterator it = find(key);
        return it == end()
                ? QV4::WeakValue().value()
                : it->value();
    }

    Iterator erase(Iterator it);

    template<typename Pointer>
    void remove(Pointer key)
    {
        Iterator it = find(key);
        if (it == end())
            return;
        erase(it);
    }

    template<typename Pointer>
    void mark(Pointer key, MarkStack *markStack)
    {
        Iterator it = find(key);
        if (it == end())
            return;
        it->markOnce(markStack);
    }

private Q_SLOTS:
    void removeDestroyedObject(QObject*);
};

}

QT_END_NAMESPACE

#endif // QV4QOBJECTWRAPPER_P_H


