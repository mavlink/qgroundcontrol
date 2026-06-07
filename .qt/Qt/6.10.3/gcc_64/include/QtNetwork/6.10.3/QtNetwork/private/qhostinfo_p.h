// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHOSTINFO_P_H
#define QHOSTINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QHostInfo class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtCore/qcoreapplication.h"
#include "private/qcoreapplication_p.h"
#include "private/qmetaobject_p.h"
#include "QtNetwork/qhostinfo.h"
#include "QtCore/qmutex.h"
#include "QtCore/qwaitcondition.h"
#include "QtCore/qobject.h"
#include "QtCore/qpointer.h"
#include "QtCore/qthread.h"
#if QT_CONFIG(thread)
#include "QtCore/qthreadpool.h"
#endif
#include "QtCore/qrunnable.h"
#include "QtCore/qlist.h"
#include "QtCore/qqueue.h"
#include <QElapsedTimer>
#include <QCache>

#include <atomic>

QT_BEGIN_NAMESPACE


class QHostInfoResult : public QObject
{
    Q_OBJECT
public:
    explicit QHostInfoResult(const QObject *receiver, QtPrivate::SlotObjUniquePtr slot);
    ~QHostInfoResult() override;

    void postResultsReady(const QHostInfo &info);

Q_SIGNALS:
    void resultsReady(const QHostInfo &info);

private Q_SLOTS:
    void finalizePostResultsReady(const QHostInfo &info);

private:
    QHostInfoResult(QHostInfoResult *other)
        : receiver(other->receiver.get() != other ? other->receiver.get() : this),
          slotObj{std::move(other->slotObj)}
    {
        // cleanup if the application terminates before results are delivered
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                this, &QObject::deleteLater);
        // maintain thread affinity
        moveToThread(other->thread());
    }

    // receiver is either a QObject provided by the user,
    // or it's set to `this` (to emulate the behavior of the contextless connect())
    QPointer<const QObject> receiver = nullptr;
    QtPrivate::SlotObjUniquePtr slotObj;
};

class QHostInfoAgent
{
public:
    static QHostInfo fromName(const QString &hostName);
    static QHostInfo lookup(const QString &hostName);
    static QHostInfo reverseLookup(const QHostAddress &address);
};

class QHostInfoPrivate
{
public:
    inline QHostInfoPrivate()
        : err(QHostInfo::NoError),
          errorStr(QLatin1StringView(QT_TRANSLATE_NOOP("QHostInfo", "Unknown error"))),
          lookupId(0)
    {
    }
    static int lookupHostImpl(const QString &name,
                              const QObject *receiver,
                              QtPrivate::QSlotObjectBase *slotObj,
                              const char *member);

    QHostInfo::HostInfoError err;
    QString errorStr;
    QList<QHostAddress> addrs;
    QString hostName;
    int lookupId;
};

// These functions are outside of the QHostInfo class and strictly internal.
// Do NOT use them outside of QAbstractSocket.
QHostInfo Q_NETWORK_EXPORT qt_qhostinfo_lookup(const QString &name, QObject *receiver, const char *member, bool *valid, int *id);
void Q_AUTOTEST_EXPORT qt_qhostinfo_clear_cache();
void Q_AUTOTEST_EXPORT qt_qhostinfo_enable_cache(bool e);
void Q_AUTOTEST_EXPORT qt_qhostinfo_cache_inject(const QString &hostname, const QHostInfo &resolution);

class QHostInfoCache
{
public:
    QHostInfoCache();
    const int max_age; // seconds

    QHostInfo get(const QString &name, bool *valid);
    void put(const QString &name, const QHostInfo &info);
    void clear();

    bool isEnabled() { return enabled.load(std::memory_order_relaxed); }
    // this function is currently only used for the auto tests
    // and not usable by public API
    void setEnabled(bool e) { enabled.store(e, std::memory_order_relaxed); }
private:
    std::atomic<bool> enabled;
    struct QHostInfoCacheElement {
        QHostInfo info;
        QElapsedTimer age;
    };
    QCache<QString,QHostInfoCacheElement> cache;
    QMutex mutex;
};

// the following classes are used for the (normal) case: We use multiple threads to lookup DNS

class QHostInfoRunnable : public QRunnable
{
public:
    explicit QHostInfoRunnable(const QString &hn, int i, const QObject *receiver,
                               QtPrivate::SlotObjUniquePtr slotObj);
    ~QHostInfoRunnable() override;

    void run() override;

    QString toBeLookedUp;
    int id;
    QHostInfoResult resultEmitter;
};


class QHostInfoLookupManager
{
public:
    QHostInfoLookupManager();
    ~QHostInfoLookupManager();

    void clear();

    // called from QHostInfo
    void scheduleLookup(QHostInfoRunnable *r);
    void abortLookup(int id);

    // called from QHostInfoRunnable
    void lookupFinished(QHostInfoRunnable *r);
    bool wasAborted(int id);

    QHostInfoCache cache;

    friend class QHostInfoRunnable;
protected:
#if QT_CONFIG(thread)
    QList<QHostInfoRunnable*> currentLookups; // in progress
    QList<QHostInfoRunnable*> postponedLookups; // postponed because in progress for same host
#endif
    QQueue<QHostInfoRunnable*> scheduledLookups; // not yet started
    QList<QHostInfoRunnable*> finishedLookups; // recently finished
    QList<int> abortedLookups; // ids of aborted lookups

#if QT_CONFIG(thread)
    QThreadPool threadPool;
#endif
    QMutex mutex;

    bool wasDeleted;

private:
    void rescheduleWithMutexHeld();
};

QT_END_NAMESPACE

#endif // QHOSTINFO_P_H
