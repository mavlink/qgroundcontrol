// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDATA_P_H
#define QQMLDATA_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qobject_p.h>
#include <private/qqmlpropertyindex_p.h>
#include <private/qv4value_p.h>
#include <private/qv4persistent_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qqmlpropertycache_p.h>
#include <qqmlprivate.h>
#include <qjsengine.h>
#include <qvector.h>

QT_BEGIN_NAMESPACE

template <class Key, class T> class QHash;
class QQmlEngine;
class QQmlGuardImpl;
class QQmlAbstractBinding;
class QQmlBoundSignal;
class QQmlContext;
class QQmlPropertyCache;
class QQmlContextData;
class QQmlNotifier;
class QQmlDataExtended;
class QQmlNotifierEndpoint;
class QQmlPropertyObserver;

namespace QV4 {
class ExecutableCompilationUnit;
namespace CompiledData {
struct Binding;
}
}

// This class is structured in such a way, that simply zero'ing it is the
// default state for elemental object allocations.  This is crucial in the
// workings of the QQmlInstruction::CreateSimpleObject instruction.
// Don't change anything here without first considering that case!
class Q_QML_EXPORT QQmlData : public QAbstractDeclarativeData
{
public:
    enum Ownership { DoesNotOwnMemory, OwnsMemory };

    QQmlData(Ownership ownership);
    ~QQmlData();

    static inline void init() {
        static bool initialized = false;
        if (!initialized) {
            initialized = true;
            QAbstractDeclarativeData::destroyed = destroyed;
            QAbstractDeclarativeData::signalEmitted = signalEmitted;
            QAbstractDeclarativeData::receivers = receivers;
            QAbstractDeclarativeData::isSignalConnected = isSignalConnected;
        }
    }

    static void destroyed(QAbstractDeclarativeData *, QObject *);
    static void signalEmitted(QAbstractDeclarativeData *, QObject *, int, void **);
    static int receivers(QAbstractDeclarativeData *, const QObject *, int);
    static bool isSignalConnected(QAbstractDeclarativeData *, const QObject *, int);

    void destroyed(QObject *);

    void setImplicitDestructible() {
        if (!explicitIndestructibleSet) indestructible = false;
    }

    // If ownMemomry is true, the QQmlData was normally allocated. Otherwise it was allocated
    // with placement new and QQmlData::destroyed is not allowed to free the memory
    quint32 ownMemory:1;
    // indestructible is set if and only if the object has CppOwnership
    // This can be explicitly set with QJSEngine::setObjectOwnership
    // Top level objects generally have CppOwnership (see QQmlcCmponentprivate::beginCreate),
    // unless created by special methods like the QML component.createObject() function
    quint32 indestructible:1;
    // indestructible was explicitly set  with setObjectOwnership
    // or the object is a top-level object
    quint32 explicitIndestructibleSet:1;
    // set when one QObject has been wrapped into QObjectWrapper in multiple engines
    // at the same time - a rather rare case
    quint32 hasTaintedV4Object:1;
    quint32 isQueuedForDeletion:1;
    /*
     * rootObjectInCreation should be true only when creating top level CPP and QML objects,
     * v4 GC will check this flag, only deletes the objects when rootObjectInCreation is false.
     */
    quint32 rootObjectInCreation:1;
    // set when at least one of the object's properties is intercepted
    quint32 hasInterceptorMetaObject:1;
    quint32 hasVMEMetaObject:1;
    /* If we have another wrapper for a const QObject * in the multiply wrapped QObjects,
     * in one of the engines. If multiple engines are used, we might not actually have
     * a const wrapper. However, if the flag is not set, we can avoid some additional
     * lookups in the map.
     */
    quint32 hasConstWrapper: 1;
    quint32 dummy:7;

    // When bindingBitsSize < sizeof(ptr), we store the binding bit flags inside
    // bindingBitsValue. When we need more than sizeof(ptr) bits, we allocated
    // sufficient space and use bindingBits to point to it.
    quint32 bindingBitsArraySize : 16;
    typedef quintptr BindingBitsType;
    enum {
        BitsPerType = sizeof(BindingBitsType) * 8,
        InlineBindingArraySize = 2
    };
    union {
        BindingBitsType *bindingBits;
        BindingBitsType bindingBitsValue[InlineBindingArraySize];
    };

    struct NotifyList {
        QAtomicInteger<quint64> connectionMask;
        QQmlNotifierEndpoint *todo = nullptr;
        QQmlNotifierEndpoint**notifies = nullptr;
        quint16 maximumTodoIndex = 0;
        quint16 notifiesSize = 0;
        void layout();
    private:
        void layout(QQmlNotifierEndpoint*);
    };
    QAtomicPointer<NotifyList> notifyList;

    inline QQmlNotifierEndpoint *notify(int index) const;
    void addNotify(int index, QQmlNotifierEndpoint *);
    int endpointCount(int index);
    bool signalHasEndpoint(int index) const;

    enum class DeleteNotifyList { Yes, No };
    void disconnectNotifiers(DeleteNotifyList doDelete);

    // The context that created the C++ object; not refcounted to prevent cycles
    QQmlContextData *context = nullptr;
    // The outermost context in which this object lives; not refcounted to prevent cycles
    QQmlContextData *outerContext = nullptr;
    QQmlRefPointer<QQmlContextData> ownContext;

    QQmlAbstractBinding *bindings = nullptr;
    QQmlBoundSignal *signalHandlers = nullptr;
    std::vector<QQmlPropertyObserver> propertyObservers;

    // Linked list for QQmlContext::contextObjects
    QQmlData *nextContextObject = nullptr;
    QQmlData**prevContextObject = nullptr;

    inline bool hasBindingBit(int) const;
    inline void setBindingBit(QObject *obj, int);
    inline void clearBindingBit(int);

    inline bool hasPendingBindingBit(int index) const;
    inline void setPendingBindingBit(QObject *obj, int);
    inline void clearPendingBindingBit(int);

    quint16 lineNumber = 0;
    quint16 columnNumber = 0;

    quint32 jsEngineId = 0; // id of the engine that created the jsWrapper

    struct DeferredData {
        DeferredData();
        ~DeferredData();
        unsigned int deferredIdx;
        QMultiHash<int, const QV4::CompiledData::Binding *> bindings;

        // Not always the same as the other compilation unit
        QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;

        // Could be either context or outerContext
        QQmlRefPointer<QQmlContextData> context;
        /* set if the deferred binding originates in an inline component,
           necessary to adjust the compilationUnit; null if there was no
           inline component */
        QString inlineComponentName;
        Q_DISABLE_COPY(DeferredData);
    };
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    QVector<DeferredData *> deferredData;

    void deferData(int objectIndex, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &,
                   const QQmlRefPointer<QQmlContextData> &, const QString &inlineComponentName);
    void releaseDeferredData();

    QV4::WeakValue jsWrapper;

    QQmlPropertyCache::ConstPtr propertyCache;

    QQmlGuardImpl *guards = nullptr;

    static QQmlData *get(QObjectPrivate *priv, bool create) {
        // If QObjectData::isDeletingChildren is set then access to QObjectPrivate::declarativeData has
        // to be avoided because QObjectPrivate::currentChildBeingDeleted is in use.
        if (priv->isDeletingChildren || priv->wasDeleted) {
            Q_ASSERT(!create);
            return nullptr;
        } else if (priv->declarativeData) {
            return static_cast<QQmlData *>(priv->declarativeData);
        } else if (create) {
            return createQQmlData(priv);
        } else {
            return nullptr;
        }
    }

    static QQmlData *get(const QObjectPrivate *priv) {
        // If QObjectData::isDeletingChildren is set then access to QObjectPrivate::declarativeData has
        // to be avoided because QObjectPrivate::currentChildBeingDeleted is in use.
        if (priv->isDeletingChildren || priv->wasDeleted)
            return nullptr;
        if (priv->declarativeData)
            return static_cast<QQmlData *>(priv->declarativeData);
        return nullptr;
    }

    static QQmlData *get(QObject *object, bool create) {
        return QQmlData::get(QObjectPrivate::get(object), create);
    }

    static QQmlData *get(const QObject *object) {
        return QQmlData::get(QObjectPrivate::get(object));

    }

    static bool keepAliveDuringGarbageCollection(const QObject *object) {
        QQmlData *ddata = get(object);
        if (!ddata || ddata->indestructible || ddata->rootObjectInCreation)
            return true;
        return false;
    }

    bool hasExtendedData() const { return extendedData != nullptr; }
    QHash<QQmlAttachedPropertiesFunc, QObject *> *attachedProperties() const;

    static inline bool wasDeleted(const QObject *);
    static inline bool wasDeleted(const QObjectPrivate *);

    static void markAsDeleted(QObject *);
    static void setQueuedForDeletion(QObject *);

    static inline void flushPendingBinding(QObject *object, int coreIndex);
    void flushPendingBinding(int coreIndex);

    static QQmlPropertyCache::ConstPtr ensurePropertyCache(QObject *object)
    {
        QQmlData *ddata = QQmlData::get(object, /*create*/true);
        if (Q_LIKELY(ddata->propertyCache))
            return ddata->propertyCache;
        return createPropertyCache(object);
    }

    Q_ALWAYS_INLINE static uint offsetForBit(int bit) { return static_cast<uint>(bit) / BitsPerType; }
    Q_ALWAYS_INLINE static BindingBitsType bitFlagForBit(int bit) { return BindingBitsType(1) << (static_cast<uint>(bit) & (BitsPerType - 1)); }

private:
    // For attachedProperties
    mutable QQmlDataExtended *extendedData = nullptr;

    Q_NEVER_INLINE static QQmlData *createQQmlData(QObjectPrivate *priv);
    Q_NEVER_INLINE static QQmlPropertyCache::ConstPtr createPropertyCache(QObject *object);

    Q_ALWAYS_INLINE bool hasBitSet(int bit) const
    {
        uint offset = offsetForBit(bit);
        if (bindingBitsArraySize <= offset)
            return false;

        const BindingBitsType *bits = (bindingBitsArraySize == InlineBindingArraySize) ? bindingBitsValue : bindingBits;
        return bits[offset] & bitFlagForBit(bit);
    }

    Q_ALWAYS_INLINE void clearBit(int bit)
    {
        uint offset = QQmlData::offsetForBit(bit);
        if (bindingBitsArraySize > offset) {
            BindingBitsType *bits = (bindingBitsArraySize == InlineBindingArraySize) ? bindingBitsValue : bindingBits;
            bits[offset] &= ~QQmlData::bitFlagForBit(bit);
        }
    }

    Q_ALWAYS_INLINE void setBit(QObject *obj, int bit)
    {
        uint offset = QQmlData::offsetForBit(bit);
        BindingBitsType *bits = (bindingBitsArraySize == InlineBindingArraySize) ? bindingBitsValue : bindingBits;
        if (Q_UNLIKELY(bindingBitsArraySize <= offset))
            bits = growBits(obj, bit);
        bits[offset] |= QQmlData::bitFlagForBit(bit);
    }

    Q_NEVER_INLINE BindingBitsType *growBits(QObject *obj, int bit);

    Q_DISABLE_COPY_MOVE(QQmlData);
};

bool QQmlData::wasDeleted(const QObjectPrivate *priv)
{
    if (!priv || priv->wasDeleted || priv->isDeletingChildren)
        return true;

    const QQmlData *ddata = QQmlData::get(priv);
    return ddata && ddata->isQueuedForDeletion;
}

bool QQmlData::wasDeleted(const QObject *object)
{
    if (!object)
        return true;

    const QObjectPrivate *priv = QObjectPrivate::get(object);
    return QQmlData::wasDeleted(priv);
}

inline bool isIndexInConnectionMask(quint64 connectionMask, int index)
{
    return connectionMask & (1ULL << quint64(index % 64));
}

QQmlNotifierEndpoint *QQmlData::notify(int index) const
{
    // Can only happen on "home" thread. We apply relaxed semantics when loading the atomics.

    Q_ASSERT(index <= 0xFFFF);

    NotifyList *list = notifyList.loadRelaxed();
    if (!list || !isIndexInConnectionMask(list->connectionMask.loadRelaxed(), index))
        return nullptr;

    if (index < list->notifiesSize)
        return list->notifies[index];

    if (index <= list->maximumTodoIndex) {
        list->layout();
        if (index < list->notifiesSize)
            return list->notifies[index];
    }

    return nullptr;
}

/*
    The index MUST be in the range returned by QObjectPrivate::signalIndex()
    This is different than the index returned by QMetaMethod::methodIndex()
*/
inline bool QQmlData::signalHasEndpoint(int index) const
{
    // This can be called from any thread.
    // We still use relaxed semantics. If we're on a thread different from the "home" thread
    // of the QQmlData, two interesting things might happen:
    //
    // 1. The list might go away while we hold it. In that case we are dealing with an object whose
    //    QObject dtor is being executed concurrently. This is UB already without the notify lists.
    //    Therefore, we don't need to consider it.
    // 2. The connectionMask may be amended or zeroed while we are looking at it. In that case
    //    we "misreport" the endpoint. Since ordering of events across threads is inherently
    //    nondeterministic, either result is correct in that case. We can accept it.

    NotifyList *list = notifyList.loadRelaxed();
    return list && isIndexInConnectionMask(list->connectionMask.loadRelaxed(), index);
}

bool QQmlData::hasBindingBit(int coreIndex) const
{
    Q_ASSERT(coreIndex >= 0);
    Q_ASSERT(coreIndex <= 0xffff);

    return hasBitSet(coreIndex * 2);
}

void QQmlData::setBindingBit(QObject *obj, int coreIndex)
{
    Q_ASSERT(coreIndex >= 0);
    Q_ASSERT(coreIndex <= 0xffff);
    setBit(obj, coreIndex * 2);
}

void QQmlData::clearBindingBit(int coreIndex)
{
    Q_ASSERT(coreIndex >= 0);
    Q_ASSERT(coreIndex <= 0xffff);
    clearBit(coreIndex * 2);
}

bool QQmlData::hasPendingBindingBit(int coreIndex) const
{
    Q_ASSERT(coreIndex >= 0);
    Q_ASSERT(coreIndex <= 0xffff);

    return hasBitSet(coreIndex * 2 + 1);
}

void QQmlData::setPendingBindingBit(QObject *obj, int coreIndex)
{
    Q_ASSERT(coreIndex >= 0);
    Q_ASSERT(coreIndex <= 0xffff);
    setBit(obj, coreIndex * 2 + 1);
}

void QQmlData::clearPendingBindingBit(int coreIndex)
{
    Q_ASSERT(coreIndex >= 0);
    Q_ASSERT(coreIndex <= 0xffff);
    clearBit(coreIndex * 2 + 1);
}

void QQmlData::flushPendingBinding(QObject *object, int coreIndex)
{
    QQmlData *data = QQmlData::get(object, false);
    if (data && data->hasPendingBindingBit(coreIndex))
        data->flushPendingBinding(coreIndex);
}

QT_END_NAMESPACE

#endif // QQMLDATA_P_H
