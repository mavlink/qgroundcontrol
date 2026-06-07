// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QORDEREDMUTEXLOCKER_P_H
#define QORDEREDMUTEXLOCKER_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qmutex.h>

#include <functional>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(thread)

/*
  Locks 2 mutexes in a defined order, avoiding a recursive lock if
  we're trying to lock the same mutex twice.
*/
class QOrderedMutexLocker
{
public:
    Q_NODISCARD_CTOR
    QOrderedMutexLocker(QBasicMutex *m1, QBasicMutex *m2)
        : mtx1((m1 == m2) ? m1 : (std::less<QBasicMutex *>()(m1, m2) ? m1 : m2)),
          mtx2((m1 == m2) ?  nullptr : (std::less<QBasicMutex *>()(m1, m2) ? m2 : m1)),
          locked(false)
    {
        relock();
    }

    Q_DISABLE_COPY(QOrderedMutexLocker)

    void swap(QOrderedMutexLocker &other) noexcept
    {
        qSwap(this->mtx1, other.mtx1);
        qSwap(this->mtx2, other.mtx2);
        qSwap(this->locked, other.locked);
    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QOrderedMutexLocker)

    Q_NODISCARD_CTOR
    QOrderedMutexLocker(QOrderedMutexLocker &&other) noexcept
        : mtx1(std::exchange(other.mtx1, nullptr))
        , mtx2(std::exchange(other.mtx2, nullptr))
        , locked(std::exchange(other.locked, false))
    {}

    ~QOrderedMutexLocker()
    {
        unlock();
    }

    void relock()
    {
        if (!locked) {
            if (mtx1) mtx1->lock();
            if (mtx2) mtx2->lock();
            locked = true;
        }
    }

    /*!
        \internal
        Can be called if the mutexes have been unlocked manually, and sets the
        state of the QOrderedMutexLocker to unlocked.
        The caller is expected to have unlocked both of them if they
        are not the same. Calling this method when the QOrderedMutexLocker is
        unlocked or when the provided mutexes have not actually been unlocked is
        UB.
     */
    void dismiss()
    {
        Q_ASSERT(locked);
        locked = false;
    }

    void unlock()
    {
        if (locked) {
            if (mtx2) mtx2->unlock();
            if (mtx1) mtx1->unlock();
            locked = false;
        }
    }

    static bool relock(QBasicMutex *mtx1, QBasicMutex *mtx2)
    {
        // mtx1 is already locked, mtx2 not... do we need to unlock and relock?
        if (mtx1 == mtx2)
            return false;
        if (std::less<QBasicMutex *>()(mtx1, mtx2)) {
            mtx2->lock();
            return true;
        }
        if (!mtx2->tryLock()) {
            mtx1->unlock();
            mtx2->lock();
            mtx1->lock();
        }
        return true;
    }

private:
    QBasicMutex *mtx1, *mtx2;
    bool locked;
};

#else

class QOrderedMutexLocker
{
public:
    Q_DISABLE_COPY(QOrderedMutexLocker)
    Q_NODISCARD_CTOR
    QOrderedMutexLocker(QBasicMutex *, QBasicMutex *) {}
    Q_NODISCARD_CTOR
    QOrderedMutexLocker(QOrderedMutexLocker &&) = default;
    QOrderedMutexLocker& operator=(QOrderedMutexLocker &&other) = default;
    ~QOrderedMutexLocker() {}

    void relock() {}
    void unlock() {}
    void dismiss() {}

    static bool relock(QBasicMutex *, QBasicMutex *) { return false; }
};

#endif


QT_END_NAMESPACE

#endif
