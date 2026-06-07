// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTHREADPOOL_P_H
#define QTHREADPOOL_P_H

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

#include "QtCore/qmutex.h"
#include "QtCore/qthread.h"
#include "QtCore/qwaitcondition.h"
#include "QtCore/qthreadpool.h"
#include "QtCore/qset.h"
#include "QtCore/qqueue.h"
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

class QDeadlineTimer;

class QueuePage
{
public:
    enum {
        MaxPageSize = 256
    };

    QueuePage(QRunnable *runnable, int pri) : m_priority(pri) { push(runnable); }

    bool isFull() { return m_lastIndex >= MaxPageSize - 1; }

    bool isFinished() { return m_firstIndex > m_lastIndex; }

    void push(QRunnable *runnable)
    {
        Q_ASSERT(runnable != nullptr);
        Q_ASSERT(!isFull());
        m_lastIndex += 1;
        m_entries[m_lastIndex] = runnable;
    }

    void skipToNextOrEnd()
    {
        while (!isFinished() && m_entries[m_firstIndex] == nullptr) {
            m_firstIndex += 1;
        }
    }

    QRunnable *first()
    {
        Q_ASSERT(!isFinished());
        QRunnable *runnable = m_entries[m_firstIndex];
        Q_ASSERT(runnable);
        return runnable;
    }

    QRunnable *pop()
    {
        Q_ASSERT(!isFinished());
        QRunnable *runnable = first();
        Q_ASSERT(runnable);

        // clear the entry although this should not be necessary
        m_entries[m_firstIndex] = nullptr;
        m_firstIndex += 1;

        // make sure the next runnable returned by first() is not a nullptr
        skipToNextOrEnd();

        return runnable;
    }

    bool tryTake(QRunnable *runnable)
    {
        Q_ASSERT(!isFinished());
        for (int i = m_firstIndex; i <= m_lastIndex; i++) {
            if (m_entries[i] == runnable) {
                m_entries[i] = nullptr;
                if (i == m_firstIndex) {
                    // make sure first() does not return a nullptr
                    skipToNextOrEnd();
                }
                return true;
            }
        }
        return false;
    }

    int priority() const { return m_priority; }

private:
    int m_priority = 0;
    int m_firstIndex = 0;
    int m_lastIndex = -1;
    QRunnable *m_entries[MaxPageSize];
};

class QThreadPoolThread;
class Q_CORE_EXPORT QThreadPoolPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QThreadPool)
    friend class QThreadPoolThread;

public:
    QThreadPoolPrivate();

    bool tryStart(QRunnable *task);
    void enqueueTask(QRunnable *task, int priority = 0);
    int activeThreadCount() const;

    void tryToStartMoreThreads();
    bool areAllThreadsActive() const;
    bool tooManyThreadsActive() const;

    int maxThreadCount() const
    { return qMax(requestedMaxThreadCount, 1); }    // documentation says we start at least one
    void startThread(QRunnable *runnable = nullptr);
    void reset();
    bool waitForDone(const QDeadlineTimer &timer);
    void clear();
    void stealAndRunRunnable(QRunnable *runnable);
    void deletePageIfFinished(QueuePage *page);

    mutable QMutex mutex;
    QSet<QThreadPoolThread *> allThreads;
    QQueue<QThreadPoolThread *> waitingThreads;
    QQueue<QThreadPoolThread *> expiredThreads;
    QList<QueuePage *> queue;
    QWaitCondition noActiveThreads;
    QString objectName;

    std::chrono::duration<int, std::milli> expiryTimeout = std::chrono::seconds(30);
    int requestedMaxThreadCount = QThread::idealThreadCount();  // don't use this directly
    int reservedThreads = 0;
    int activeThreads = 0;
    uint stackSize = 0;
    QThread::Priority threadPriority = QThread::InheritPriority;
    QThread::QualityOfService serviceLevel = QThread::QualityOfService::Auto;
};

QT_END_NAMESPACE

#endif
