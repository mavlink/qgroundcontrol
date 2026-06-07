// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTHREAD_P_H
#define QTHREAD_P_H

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
//

#include "QtCore/qthread.h"

#include "QtCore/qcoreapplication.h"
#include "private/qobject_p.h"
#include "QtCore/qmap.h"
#include "QtCore/qmutex.h"
#include "QtCore/qstack.h"

#if QT_CONFIG(thread)
#include "private/qthreadstorage_p.h"
#include "QtCore/qwaitcondition.h"
#endif

#include <atomic>

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QEventLoop;

class QPostEvent
{
public:
    QObject *receiver;
    QEvent *event;
    int priority;
    inline QPostEvent()
        : receiver(nullptr), event(nullptr), priority(0)
    { }
    inline QPostEvent(QObject *r, QEvent *e, int p)
        : receiver(r), event(e), priority(p)
    { }
};
Q_DECLARE_TYPEINFO(QPostEvent, Q_RELOCATABLE_TYPE);

inline bool operator<(const QPostEvent &first, const QPostEvent &second)
{
    return first.priority > second.priority;
}

// This class holds the list of posted events.
//  The list has to be kept sorted by priority
// ### Qt7 remove the next line
// It's used in a virtual in QCoreApplication, so ELFVERSION:ignore-next
class QPostEventList : public QList<QPostEvent>
{
public:
    // recursion == recursion count for sendPostedEvents()
    int recursion;

    // sendOffset == the current event to start sending
    qsizetype startOffset;
    // insertionOffset == set by sendPostedEvents to tell postEvent() where to start insertions
    qsizetype insertionOffset;

    QMutex mutex;

    inline QPostEventList() : QList<QPostEvent>(), recursion(0), startOffset(0), insertionOffset(0) { }

    void addEvent(const QPostEvent &ev);

private:
    //hides because they do not keep that list sorted. addEvent must be used
    using QList<QPostEvent>::append;
    using QList<QPostEvent>::insert;
};

namespace QtPrivate {

/* BindingStatusOrList is basically a QBiPointer (as found in declarative)
   with some helper methods to manipulate the list. BindingStatusOrList starts
   its life in a null state and supports the following transitions

                        0 state (initial)
                       /                \
                      /                  \
                     v                    v
             pending object list----------->binding status
    Note that binding status is the final state, and we never transition away
    from it
*/
class BindingStatusOrList
{
    Q_DISABLE_COPY_MOVE(BindingStatusOrList)
public:
    using List = std::vector<QObject *>;

    constexpr BindingStatusOrList() noexcept : data(0) {}
    explicit BindingStatusOrList(QBindingStatus *status) noexcept :
        data(encodeBindingStatus(status)) {}
    explicit BindingStatusOrList(List *list) noexcept : data(encodeList(list)) {}
    ~BindingStatusOrList()
    {
        auto status = bindingStatus();
        delete status;
    }

    // requires external synchronization:
    QBindingStatus *addObjectUnlessAlreadyStatus(QObject *object);
    void removeObject(QObject *object);
    void setStatusAndClearList(QBindingStatus *status) noexcept;


    static bool isBindingStatus(quintptr data) noexcept
    {
        return !isNull(data) && !isList(data);
    }
    static bool isList(quintptr data) noexcept { return data & 1; }
    static bool isNull(quintptr data) noexcept { return data == 0; }

    // thread-safe:
    QBindingStatus *bindingStatus() const noexcept
    {
        // synchronizes-with the store-release in setStatusAndClearList():
        const auto d = data.load(std::memory_order_acquire);
        if (isBindingStatus(d))
            return reinterpret_cast<QBindingStatus *>(d);
        else
            return nullptr;
    }

    // requires external synchronization:
    List *list() const noexcept
    {
        return decodeList(data.load(std::memory_order_relaxed));
    }

private:
    static List *decodeList(quintptr ptr) noexcept
    {
        if (isList(ptr))
            return reinterpret_cast<List *>(ptr & ~1);
        else
            return nullptr;
    }

    static quintptr encodeBindingStatus(QBindingStatus *status) noexcept
    {
        return quintptr(status);
    }

    static quintptr encodeList(List *list) noexcept
    {
        return quintptr(list) | 1;
    }

    std::atomic<quintptr> data;
};

} // namespace QtPrivate

#if QT_CONFIG(thread)

class Q_CORE_EXPORT QDaemonThread : public QThread
{
public:
    QDaemonThread(QObject *parent = nullptr);
    ~QDaemonThread();
};

class Q_AUTOTEST_EXPORT QThreadPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QThread)

public:
    QThreadPrivate(QThreadData *d = nullptr);
    ~QThreadPrivate();

    void setPriority(QThread::Priority prio);
    Qt::HANDLE threadId() const noexcept;

    mutable QMutex mutex;
    QAtomicInt quitLockRef;

    enum State : quint8 {
        // All state changes are imprecise
        NotStarted = 0,     // before start() or if failed to start
        Running = 1,        // in run()
        Finishing = 2,      // in QThreadPrivate::finish()
        Finished = 3,       // QThreadPrivate::finish() or cleanup() is done
                            //    or, if using pthread_join, joining is done
    };

    State threadState = NotStarted;
    bool exited = false;
    std::atomic<bool> interruptionRequested = false;
#ifdef Q_OS_UNIX
    bool terminated = false; // when (the first) terminate has been called
#endif

    int waiters = 0;
    int returnCode = -1;

    uint stackSize = 0;
    std::underlying_type_t<QThread::Priority> priority = QThread::InheritPriority;

    bool wait(QMutexLocker<QMutex> &locker, QDeadlineTimer deadline);

    QThread::QualityOfService serviceLevel = QThread::QualityOfService::Auto;
    void setQualityOfServiceLevel(QThread::QualityOfService qosLevel);
#ifdef Q_OS_DARWIN
    qos_class_t nativeQualityOfServiceClass() const;
#endif

#ifdef Q_OS_UNIX
    QWaitCondition thread_done;

    void wakeAll();
    static void *start(void *arg);
    void finish();          // happens early (before thread-local dtors)
    void cleanup();         // happens late (as a thread-local dtor, if possible)
#endif // Q_OS_UNIX

#ifdef Q_OS_WIN
    static unsigned int __stdcall start(void *) noexcept;
    void finish(bool lockAnyway = true) noexcept;

    Qt::HANDLE handle;
    bool terminationEnabled, terminatePending;
#endif // Q_OS_WIN
#ifdef Q_OS_WASM
    static int idealThreadCount;
#endif
    QThreadData *data;

    static QAbstractEventDispatcher *createEventDispatcher(QThreadData *data);

    void ref()
    {
        quitLockRef.ref();
    }

    void deref()
    {
        if (!quitLockRef.deref() && threadState == Running) {
            QCoreApplication::instance()->postEvent(q_ptr, new QEvent(QEvent::Quit));
        }
    }

    QBindingStatus *bindingStatus();

    /* Returns nullptr if the object has been added, or the binding status
       if that one has been set in the meantime
    */
    QBindingStatus *addObjectWithPendingBindingStatusChange(QObject *obj);
    void removeObjectWithPendingBindingStatusChange(QObject *obj);

private:
#ifdef Q_OS_INTEGRITY
    // On INTEGRITY we set the thread name before starting it, so just fake a string
    struct FakeString {
        bool isEmpty() const { return true; }
        const char *toLocal8Bit() const { return nullptr; }
    } objectName;
#else
    // Used in QThread(Private)::start to avoid racy access to QObject::objectName,
    // unset afterwards.
    QString objectName;
#endif
};

#else // QT_CONFIG(thread)

class QThreadPrivate : public QObjectPrivate
{
public:
    QThreadPrivate(QThreadData *d = nullptr);
    ~QThreadPrivate();

    mutable QMutex mutex;
    QThreadData *data;
    QBindingStatus* m_bindingStatus;
    bool running = false;

    QBindingStatus* bindingStatus() { return m_bindingStatus; }
    QBindingStatus *addObjectWithPendingBindingStatusChange(QObject *) { return nullptr; }
    void removeObjectWithPendingBindingStatusChange(QObject *) {}

    static void setCurrentThread(QThread *) { }
    static QAbstractEventDispatcher *createEventDispatcher(QThreadData *data);

    void ref() {}
    void deref() {}

    Q_DECLARE_PUBLIC(QThread)
};

#endif // QT_CONFIG(thread)

class QThreadData
{
public:
    QThreadData(int initialRefCount = 1)
        : _ref(initialRefCount)
    {
        // fprintf(stderr, "QThreadData %p created\n", this);
    }
    ~QThreadData();

    static QThreadData *current()
    {
        if (QThreadData *data = currentThreadData()) Q_LIKELY_BRANCH
            return data;
        return createCurrentThreadData();
    }
    static void clearCurrentThreadData();
    static QThreadData *get2(QThread *thread)
    { Q_ASSERT_X(thread != nullptr, "QThread", "internal error"); return thread->d_func()->data; }

#if QT_CONFIG(thread)
    void ref()
    {
        (void) _ref.ref();
        Q_ASSERT(_ref.loadRelaxed() != 0);
    }
    void deref()
    {
        if (!_ref.deref())
            delete this;
    }
#else
    void ref() {}
    void deref() {}
#endif
    inline bool hasEventDispatcher() const
    { return eventDispatcher.loadRelaxed() != nullptr; }
    QAbstractEventDispatcher *createEventDispatcher();
    QAbstractEventDispatcher *ensureEventDispatcher()
    {
        QAbstractEventDispatcher *ed = eventDispatcher.loadRelaxed();
        if (Q_LIKELY(ed))
            return ed;
        return createEventDispatcher();
    }

    bool canWaitLocked()
    {
        QMutexLocker locker(&postEventList.mutex);
        return canWait;
    }

    void clearEvents();

    void reuseBindingStatusForNewNativeThread()
    {
        auto status = m_statusOrPendingObjects.bindingStatus();
        if (status)
            QtPrivate::setBindingStatus(status, {});
    }

    QStack<QEventLoop *> eventLoops;
    QPostEventList postEventList;
    QAtomicPointer<QThread> thread;
    QAtomicPointer<void> threadId;
    QAtomicPointer<QAbstractEventDispatcher> eventDispatcher;
    QList<void *> tls;
    // manipulating m_statusOrPendingObjects requires QTreadPrivate's mutex to be locked
    QtPrivate::BindingStatusOrList m_statusOrPendingObjects = {};

private:
    QAtomicInt _ref;

public:
    int loopLevel = 0;
    int scopeLevel = 0;

    bool quitNow = false;
    bool canWait = true;
    bool isAdopted = false;
    bool requiresCoreApplication = true;

private:
    friend class QAbstractEventDispatcher;
    friend class QBasicTimer;
    static Q_AUTOTEST_EXPORT QThreadData *currentThreadData() noexcept Q_DECL_PURE_FUNCTION;
    static Q_AUTOTEST_EXPORT QThreadData *createCurrentThreadData();
};

class QScopedScopeLevelCounter
{
    QThreadData *threadData;
public:
    inline QScopedScopeLevelCounter(QThreadData *threadData)
        : threadData(threadData)
    { ++threadData->scopeLevel; }
    inline ~QScopedScopeLevelCounter()
    { --threadData->scopeLevel; }
};

// thread wrapper for the main() thread
class QAdoptedThread : public QThread
{
    Q_DECLARE_PRIVATE(QThread)

public:
    QAdoptedThread(QThreadData *data);
    ~QAdoptedThread();
    void init();

private:
#if QT_CONFIG(thread)
    void run() override;
#endif
};

#if QT_CONFIG(thread)
inline QBindingStatus *QThreadPrivate::bindingStatus()
{
    return data->m_statusOrPendingObjects.bindingStatus();
}
#endif

QT_END_NAMESPACE

#endif // QTHREAD_P_H
