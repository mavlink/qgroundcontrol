// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QREADWRITELOCK_P_H
#define QREADWRITELOCK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the implementation.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qlocking_p.h>
#include <QtCore/private/qwaitcondition_p.h>
#include <QtCore/qreadwritelock.h>
#include <QtCore/qvarlengtharray.h>

QT_REQUIRE_CONFIG(thread);

QT_BEGIN_NAMESPACE

namespace QReadWriteLockStates {
enum {
    StateMask = 0x3,
    StateLockedForRead = 0x1,
    StateLockedForWrite = 0x2,
};
enum StateForWaitCondition {
    LockedForRead,
    LockedForWrite,
    Unlocked,
    RecursivelyLocked
};
}

class QReadWriteLockPrivate
{
public:
    explicit QReadWriteLockPrivate(bool isRecursive = false)
        : recursive(isRecursive) {}

    alignas(QtPrivate::IdealMutexAlignment) std::condition_variable writerCond;
    std::condition_variable readerCond;

    alignas(QtPrivate::IdealMutexAlignment) std::mutex mutex;
    int readerCount = 0;
    int writerCount = 0;
    int waitingReaders = 0;
    int waitingWriters = 0;
    const bool recursive;

    //Called with the mutex locked
    bool lockForWrite(std::unique_lock<std::mutex> &lock, QDeadlineTimer timeout);
    bool lockForRead(std::unique_lock<std::mutex> &lock, QDeadlineTimer timeout);
    void unlock();

    //memory management
    int id = 0;
    void release();
    static QReadWriteLockPrivate *allocate();

    // Recursive mutex handling
    Qt::HANDLE currentWriter = {};

    struct Reader {
        Qt::HANDLE handle;
        int recursionLevel;
    };

    QVarLengthArray<Reader, 16> currentReaders;

    // called with the mutex unlocked
    bool recursiveLockForWrite(QDeadlineTimer timeout);
    bool recursiveLockForRead(QDeadlineTimer timeout);
    void recursiveUnlock();

    static QReadWriteLockStates::StateForWaitCondition
    stateForWaitCondition(const QReadWriteLock *lock);
};
Q_DECLARE_TYPEINFO(QReadWriteLockPrivate::Reader, Q_PRIMITIVE_TYPE);\

/*! \internal  Helper for QWaitCondition::wait */
inline QReadWriteLockStates::StateForWaitCondition
QReadWriteLockPrivate::stateForWaitCondition(const QReadWriteLock *q)
{
    using namespace QReadWriteLockStates;
    QReadWriteLockPrivate *d = q->d_ptr.loadAcquire();
    switch (quintptr(d) & StateMask) {
    case StateLockedForRead: return LockedForRead;
    case StateLockedForWrite: return LockedForWrite;
    }

    if (!d)
        return Unlocked;
    const auto lock = qt_scoped_lock(d->mutex);
    if (d->writerCount > 1)
        return RecursivelyLocked;
    else if (d->writerCount == 1)
        return LockedForWrite;
    return LockedForRead;

}

QT_END_NAMESPACE

#endif // QREADWRITELOCK_P_H
