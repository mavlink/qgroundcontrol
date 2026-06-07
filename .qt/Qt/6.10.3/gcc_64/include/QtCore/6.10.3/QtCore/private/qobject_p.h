// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOBJECT_P_H
#define QOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qcoreevent.h"
#include <QtCore/qfunctionaltools_impl.h>
#include "QtCore/qlist.h"
#include "QtCore/qobject.h"
#include "QtCore/qpointer.h"
#include "QtCore/qvariant.h"
#include "QtCore/qproperty.h"
#include <QtCore/qshareddata.h>
#include "QtCore/private/qproperty_p.h"

#include <string>

QT_BEGIN_NAMESPACE

#ifdef Q_MOC_RUN
#define QT_ANONYMOUS_PROPERTY(text) QT_ANONYMOUS_PROPERTY(text)
#define QT_ANONYMOUS_PRIVATE_PROPERTY(d, text) QT_ANONYMOUS_PRIVATE_PROPERTY(d, text)
#elif !defined QT_NO_META_MACROS
#define QT_ANONYMOUS_PROPERTY(...) QT_ANNOTATE_CLASS(qt_anonymous_property, __VA_ARGS__)
#define QT_ANONYMOUS_PRIVATE_PROPERTY(d, text) QT_ANNOTATE_CLASS2(qt_anonymous_private_property, d, text)
#endif

#define QT_CONCAT(B, M, m, u)   QT_CONCAT2(B, M, m, u)
#define QT_CONCAT2(B, M, m, u)  B ## M ## _ ## m ## _ ## u
#if defined(QT_BUILD_INTERNAL) && !QT_CONFIG(elf_private_full_version)
// Don't check the version parameter in internal builds.
// This allows incompatible versions to be loaded, possibly for testing.
enum QObjectPrivateVersionEnum
#else
enum QT_CONCAT(QtPrivate_, QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH)
#endif
{ QObjectPrivateVersion = QT_VERSION };
#undef QT_CONCAT
#undef QT_CONCAT2

class QVariant;
class QThreadData;
class QObjectConnectionListVector;
namespace QtSharedPointer { struct ExternalRefCountData; }

/* for Qt Test */
struct QSignalSpyCallbackSet
{
    typedef void (*BeginCallback)(QObject *caller, int signal_or_method_index, void **argv);
    typedef void (*EndCallback)(QObject *caller, int signal_or_method_index);
    BeginCallback signal_begin_callback,
                    slot_begin_callback;
    EndCallback signal_end_callback,
                slot_end_callback;
};
void Q_CORE_EXPORT qt_register_signal_spy_callbacks(QSignalSpyCallbackSet *callback_set);

extern Q_CORE_EXPORT QBasicAtomicPointer<QSignalSpyCallbackSet> qt_signal_spy_callback_set;

class Q_CORE_EXPORT QAbstractDeclarativeData
{
public:
    static void (*destroyed)(QAbstractDeclarativeData *, QObject *);
    static void (*signalEmitted)(QAbstractDeclarativeData *, QObject *, int, void **);
    static int  (*receivers)(QAbstractDeclarativeData *, const QObject *, int);
    static bool (*isSignalConnected)(QAbstractDeclarativeData *, const QObject *, int);
    static void (*setWidgetParent)(QObject *, QObject *); // Used by the QML engine to specify parents for widgets. Set by QtWidgets.
};

class Q_CORE_EXPORT QObjectPrivate : public QObjectData
{
public:
    Q_DECLARE_PUBLIC(QObject)

    struct ExtraData
    {
        ExtraData(QObjectPrivate *ptr) : parent(ptr) { }

        inline void setObjectNameForwarder(const QString &name)
        {
            parent->q_func()->setObjectName(name);
        }

        inline void nameChangedForwarder(const QString &name)
        {
            Q_EMIT parent->q_func()->objectNameChanged(name, QObject::QPrivateSignal());
        }

        QList<QByteArray> propertyNames;
        QList<QVariant> propertyValues;
        QList<Qt::TimerId> runningTimers;
        QList<QPointer<QObject>> eventFilters;
        Q_OBJECT_COMPAT_PROPERTY(QObjectPrivate::ExtraData, QString, objectName,
                                 &QObjectPrivate::ExtraData::setObjectNameForwarder,
                                 &QObjectPrivate::ExtraData::nameChangedForwarder)
        QObjectPrivate *parent;
    };

    void ensureExtraData()
    {
        if (!extraData)
            extraData = new ExtraData(this);
    }
    void setObjectNameWithoutBindings(const QString &name);

    typedef void (*StaticMetaCallFunction)(QObject *, QMetaObject::Call, int, void **);
    struct Connection;
    struct ConnectionData;
    struct ConnectionList;
    struct ConnectionOrSignalVector;
    struct SignalVector;
    struct Sender;
    struct TaggedSignalVector;

    /*
        This contains the all connections from and to an object.

        The signalVector contains the lists of connections for a given signal. The index in the vector correspond
        to the signal index. The signal index is the one returned by QObjectPrivate::signalIndex (not
        QMetaObject::indexOfSignal). allsignals contains a list of special connections that will get invoked on
        any signal emission. This is done by connecting to signal index -1.

        This vector is protected by the object mutex (signalSlotLock())

        Each Connection is also part of a 'senders' linked list. This one contains all connections connected
        to a slot in this object. The mutex of the receiver must be locked when touching the pointers of this
        linked list.
    */

    QObjectPrivate(decltype(QObjectPrivateVersion) version = QObjectPrivateVersion);
    virtual ~QObjectPrivate();
    void deleteChildren();
    // used to clear binding storage early in ~QObject
    void clearBindingStorage();

    void setParent_helper(QObject *);
    void moveToThread_helper();
    void setThreadData_helper(QThreadData *currentData, QThreadData *targetData, QBindingStatus *status);

    QObjectList receiverList(const char *signal) const;

    inline void ensureConnectionData();
    inline void addConnection(int signal, Connection *c);
    static inline bool removeConnection(Connection *c);

    static QObjectPrivate *get(QObject *o) { return o->d_func(); }
    static const QObjectPrivate *get(const QObject *o) { return o->d_func(); }

    int signalIndex(const char *signalName, const QMetaObject **meta = nullptr) const;
    bool isSignalConnected(uint signalIdx, bool checkDeclarative = true) const;
    bool maybeSignalConnected(uint signalIndex) const;
    inline bool isDeclarativeSignalConnected(uint signalIdx) const;

    // To allow abitrary objects to call connectNotify()/disconnectNotify() without making
    // the API public in QObject. This is used by QQmlNotifierEndpoint.
    inline void connectNotify(const QMetaMethod &signal);
    inline void disconnectNotify(const QMetaMethod &signal);

    void reinitBindingStorageAfterThreadMove();

    template <typename Func1, typename Func2>
    static inline QMetaObject::Connection connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                                  const typename QtPrivate::FunctionPointer<Func2>::Object *receiverPrivate, Func2 slot,
                                                  Qt::ConnectionType type = Qt::AutoConnection);

    template <typename Func1, typename Func2>
    static inline bool disconnect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                  const typename QtPrivate::FunctionPointer<Func2>::Object *receiverPrivate, Func2 slot);

    static QMetaObject::Connection connectImpl(const QObject *sender, int signal_index,
                                               const QObject *receiver, void **slot,
                                               QtPrivate::QSlotObjectBase *slotObj, int type,
                                               const int *types, const QMetaObject *senderMetaObject);
    static QMetaObject::Connection connect(const QObject *sender, int signal_index, QtPrivate::QSlotObjectBase *slotObj, Qt::ConnectionType type);
    static QMetaObject::Connection connect(const QObject *sender, int signal_index,
                                           const QObject *receiver,
                                           QtPrivate::QSlotObjectBase *slotObj,
                                           Qt::ConnectionType type);
    static bool disconnect(const QObject *sender, int signal_index, void **slot);
    static bool disconnect(const QObject *sender, int signal_index, const QObject *receiver,
                           void **slot);

    virtual std::string flagsForDumping() const;

#ifndef QT_NO_DEBUG_STREAM
    virtual void writeToDebugStream(QDebug &) const;
#endif

    QtPrivate::QPropertyAdaptorSlotObject *
    getPropertyAdaptorSlotObject(const QMetaProperty &property);

public:
    mutable ExtraData *extraData; // extra data set by the user
    // This atomic requires acquire/release semantics in a few places,
    // e.g. QObject::moveToThread must synchronize with QCoreApplication::postEvent,
    // because postEvent is thread-safe.
    // However, most of the code paths involving QObject are only reentrant and
    // not thread-safe, so synchronization should not be necessary there.
    QAtomicPointer<QThreadData> threadData; // id of the thread that owns the object

    using ConnectionDataPointer = QExplicitlySharedDataPointer<ConnectionData>;
    QAtomicPointer<ConnectionData> connections;

    union {
        QObject *currentChildBeingDeleted; // should only be used when QObjectData::isDeletingChildren is set
        QAbstractDeclarativeData *declarativeData; //extra data used by the declarative module
    };

    // these objects are all used to indicate that a QObject was deleted
    // plus QPointer, which keeps a separate list
    QAtomicPointer<QtSharedPointer::ExternalRefCountData> sharedRefcount;
};

inline bool QObjectPrivate::isDeclarativeSignalConnected(uint signal_index) const
{
    return !isDeletingChildren && declarativeData && QAbstractDeclarativeData::isSignalConnected
            && QAbstractDeclarativeData::isSignalConnected(declarativeData, q_func(), signal_index);
}

inline void QObjectPrivate::connectNotify(const QMetaMethod &signal)
{
    q_ptr->connectNotify(signal);
}

inline void QObjectPrivate::disconnectNotify(const QMetaMethod &signal)
{
    q_ptr->disconnectNotify(signal);
}

namespace QtPrivate {
inline const QObject *getQObject(const QObjectPrivate *d) { return d->q_func(); }

template <typename Func>
using FunctionStorage = QtPrivate::CompactStorage<Func>;

template <typename ObjPrivate> inline void assertObjectType(QObjectPrivate *d)
{
    using Obj = std::remove_pointer_t<decltype(std::declval<ObjPrivate *>()->q_func())>;
    assertObjectType<Obj>(d->q_ptr);
}

template<typename Func, typename Args, typename R>
class QPrivateSlotObject : public QSlotObjectBase, private FunctionStorage<Func>
{
    typedef QtPrivate::FunctionPointer<Func> FuncType;
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    static void impl(int which, QSlotObjectBase *this_, QObject *r, void **a, bool *ret)
#else
    static void impl(QSlotObjectBase *this_, QObject *r, void **a, int which, bool *ret)
#endif
    {
        const auto that = static_cast<QPrivateSlotObject*>(this_);
        switch (which) {
            case Destroy:
                delete that;
                break;
            case Call:
                FuncType::template call<Args, R>(that->object(),
                                                 static_cast<typename FuncType::Object *>(QObjectPrivate::get(r)), a);
                break;
            case Compare:
                *ret = *reinterpret_cast<Func *>(a) == that->object();
                break;
            case NumOperations: ;
        }
    }
public:
    explicit QPrivateSlotObject(Func f) : QSlotObjectBase(&impl), FunctionStorage<Func>{std::move(f)} {}
};
} //namespace QtPrivate

template <typename Func1, typename Func2>
inline QMetaObject::Connection QObjectPrivate::connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                                       const typename QtPrivate::FunctionPointer<Func2>::Object *receiverPrivate, Func2 slot,
                                                       Qt::ConnectionType type)
{
    typedef QtPrivate::FunctionPointer<Func1> SignalType;
    typedef QtPrivate::FunctionPointer<Func2> SlotType;
    static_assert(QtPrivate::HasQ_OBJECT_Macro<typename SignalType::Object>::Value,
                      "No Q_OBJECT in the class with the signal");

    //compilation error if the arguments does not match.
    static_assert(int(SignalType::ArgumentCount) >= int(SlotType::ArgumentCount),
                      "The slot requires more arguments than the signal provides.");
    static_assert((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                      "Signal and slot arguments are not compatible.");
    static_assert((QtPrivate::AreArgumentsCompatible<typename SlotType::ReturnType, typename SignalType::ReturnType>::value),
                      "Return type of the slot is not compatible with the return type of the signal.");

    const int *types = nullptr;
    if (type == Qt::QueuedConnection || type == Qt::BlockingQueuedConnection)
        types = QtPrivate::ConnectionTypes<typename SignalType::Arguments>::types();

    return QObject::connectImpl(sender, reinterpret_cast<void **>(&signal),
        QtPrivate::getQObject(receiverPrivate), reinterpret_cast<void **>(&slot),
        new QtPrivate::QPrivateSlotObject<Func2, typename QtPrivate::List_Left<typename SignalType::Arguments, SlotType::ArgumentCount>::Value,
                                        typename SignalType::ReturnType>(slot),
        type, types, &SignalType::Object::staticMetaObject);
}

template <typename Func1, typename Func2>
bool QObjectPrivate::disconnect(const typename QtPrivate::FunctionPointer< Func1 >::Object* sender, Func1 signal,
                                const typename QtPrivate::FunctionPointer< Func2 >::Object* receiverPrivate, Func2 slot)
{
    typedef QtPrivate::FunctionPointer<Func1> SignalType;
    typedef QtPrivate::FunctionPointer<Func2> SlotType;
    static_assert(QtPrivate::HasQ_OBJECT_Macro<typename SignalType::Object>::Value,
                      "No Q_OBJECT in the class with the signal");
    //compilation error if the arguments does not match.
    static_assert((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                      "Signal and slot arguments are not compatible.");
    return QObject::disconnectImpl(sender, reinterpret_cast<void **>(&signal),
                          receiverPrivate->q_ptr, reinterpret_cast<void **>(&slot),
                          &SignalType::Object::staticMetaObject);
}

class QSemaphore;
class Q_CORE_EXPORT QAbstractMetaCallEvent : public QEvent
{
public:
    QAbstractMetaCallEvent(const QObject *sender, int signalId, QSemaphore *semaphore = nullptr)
        : QEvent(MetaCall), signalId_(signalId), sender_(sender)
#if QT_CONFIG(thread)
        , semaphore_(semaphore)
#endif
    { Q_UNUSED(semaphore); }
    ~QAbstractMetaCallEvent();

    virtual void placeMetaCall(QObject *object) = 0;

    inline const QObject *sender() const { return sender_; }
    inline int signalId() const { return signalId_; }

private:
    int signalId_;
    const QObject *sender_;
#if QT_CONFIG(thread)
    QSemaphore *semaphore_;
#endif
};

class Q_CORE_EXPORT QMetaCallEvent : public QAbstractMetaCallEvent
{
public:
    // blocking queued with semaphore - args always owned by caller
    QMetaCallEvent(ushort method_offset, ushort method_relative,
                   QObjectPrivate::StaticMetaCallFunction callFunction,
                   const QObject *sender, int signalId,
                   void **args, QSemaphore *semaphore);
    QMetaCallEvent(QtPrivate::QSlotObjectBase *slotObj,
                   const QObject *sender, int signalId,
                   void **args, QSemaphore *semaphore);
    QMetaCallEvent(QtPrivate::SlotObjUniquePtr slotObj,
                   const QObject *sender, int signalId,
                   void **args, QSemaphore *semaphore);

    // queued - args allocated by event, copied by caller
    QMetaCallEvent(ushort method_offset, ushort method_relative,
                   QObjectPrivate::StaticMetaCallFunction callFunction,
                   const QObject *sender, int signalId,
                   int nargs);
    QMetaCallEvent(QtPrivate::QSlotObjectBase *slotObj,
                   const QObject *sender, int signalId,
                   int nargs);
    QMetaCallEvent(QtPrivate::SlotObjUniquePtr slotObj,
                   const QObject *sender, int signalId,
                   int nargs);

    ~QMetaCallEvent() override;

    inline int id() const { return d.method_offset_ + d.method_relative_; }
    inline const void * const* args() const { return d.args_; }
    inline void ** args() { return d.args_; }
    inline const QMetaType *types() const { return reinterpret_cast<QMetaType *>(d.args_ + d.nargs_); }
    inline QMetaType *types() { return reinterpret_cast<QMetaType *>(d.args_ + d.nargs_); }

    virtual void placeMetaCall(QObject *object) override;

private:
    inline void allocArgs();

    struct Data {
        QtPrivate::SlotObjUniquePtr slotObj_;
        void **args_;
        QObjectPrivate::StaticMetaCallFunction callFunction_;
        int nargs_;
        ushort method_offset_;
        ushort method_relative_;
    } d;
    // preallocate enough space for three arguments
    alignas(void *) char prealloc_[3 * sizeof(void *) + 3 * sizeof(QMetaType)];
};

struct QAbstractDynamicMetaObject;
struct Q_CORE_EXPORT QDynamicMetaObjectData
{
    virtual ~QDynamicMetaObjectData();
    virtual void objectDestroyed(QObject *) { delete this; }

#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
    virtual const QMetaObject *toDynamicMetaObject(QObject *) const = 0;
#else
    virtual QMetaObject *toDynamicMetaObject(QObject *) = 0;
#endif
    virtual int metaCall(QObject *, QMetaObject::Call, int _id, void **) = 0;
};

struct Q_CORE_EXPORT QAbstractDynamicMetaObject : public QDynamicMetaObjectData, public QMetaObject
{
    ~QAbstractDynamicMetaObject();

#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
    const QMetaObject *toDynamicMetaObject(QObject *) const override { return this; }
#else
    QMetaObject *toDynamicMetaObject(QObject *) override { return this; }
#endif
    virtual int createProperty(const char *, const char *) { return -1; }
    int metaCall(QObject *, QMetaObject::Call c, int _id, void **a) override
    { return metaCall(c, _id, a); }
    virtual int metaCall(QMetaObject::Call, int _id, void **) { return _id; } // Compat overload
};

inline const QBindingStorage *qGetBindingStorage(const QObjectPrivate *o)
{
    return &o->bindingStorage;
}
inline QBindingStorage *qGetBindingStorage(QObjectPrivate *o)
{
    return &o->bindingStorage;
}
inline const QBindingStorage *qGetBindingStorage(const QObjectPrivate::ExtraData *ed)
{
    return &ed->parent->bindingStorage;
}
inline QBindingStorage *qGetBindingStorage(QObjectPrivate::ExtraData *ed)
{
    return &ed->parent->bindingStorage;
}

QT_END_NAMESPACE

#endif // QOBJECT_P_H
