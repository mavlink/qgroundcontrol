// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TASKING_CONCURRENTCALL_H
#define TASKING_CONCURRENTCALL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "tasktree.h"

#include <QtConcurrent/QtConcurrent>

QT_BEGIN_NAMESPACE

namespace Tasking {

// This class introduces the dependency to Qt::Concurrent, otherwise Tasking namespace
// is independent on Qt::Concurrent.
// Possibly, it could be placed inside Qt::Concurrent library, as a wrapper around
// QtConcurrent::run() call.

template <typename ResultType>
class ConcurrentCall
{
    Q_DISABLE_COPY_MOVE(ConcurrentCall)

public:
    ConcurrentCall() = default;
    template <typename Function, typename ...Args>
    void setConcurrentCallData(Function &&function, Args &&...args)
    {
        wrapConcurrent(std::forward<Function>(function), std::forward<Args>(args)...);
    }
    void setThreadPool(QThreadPool *pool) { m_threadPool = pool; }
    ResultType result() const
    {
        return m_future.resultCount() ? m_future.result() : ResultType();
    }
    QList<ResultType> results() const
    {
        return m_future.results();
    }
    QFuture<ResultType> future() const { return m_future; }

private:
    template <typename Function, typename ...Args>
    void wrapConcurrent(Function &&function, Args &&...args)
    {
        m_startHandler = [this, function = std::forward<Function>(function), args...] {
            QThreadPool *threadPool = m_threadPool ? m_threadPool : QThreadPool::globalInstance();
            return QtConcurrent::run(threadPool, function, args...);
        };
    }

    template <typename Function, typename ...Args>
    void wrapConcurrent(std::reference_wrapper<const Function> &&wrapper, Args &&...args)
    {
        m_startHandler = [this, wrapper = std::forward<std::reference_wrapper<const Function>>(wrapper), args...] {
            QThreadPool *threadPool = m_threadPool ? m_threadPool : QThreadPool::globalInstance();
            return QtConcurrent::run(threadPool, std::forward<const Function>(wrapper.get()),
                                     args...);
        };
    }

    template <typename T>
    friend class ConcurrentCallTaskAdapter;

    std::function<QFuture<ResultType>()> m_startHandler;
    QThreadPool *m_threadPool = nullptr;
    QFuture<ResultType> m_future;
};

template <typename ResultType>
class ConcurrentCallTaskAdapter : public TaskAdapter<ConcurrentCall<ResultType>>
{
public:
    ~ConcurrentCallTaskAdapter() {
        if (m_watcher) {
            m_watcher->cancel();
            m_watcher->waitForFinished();
        }
    }

    void start() final {
        if (!this->task()->m_startHandler) {
            emit this->done(DoneResult::Error); // TODO: Add runtime assert
            return;
        }
        m_watcher.reset(new QFutureWatcher<ResultType>);
        this->connect(m_watcher.get(), &QFutureWatcherBase::finished, this, [this] {
            emit this->done(toDoneResult(!m_watcher->isCanceled()));
            m_watcher.release()->deleteLater();
        });
        this->task()->m_future = this->task()->m_startHandler();
        m_watcher->setFuture(this->task()->m_future);
    }

private:
    std::unique_ptr<QFutureWatcher<ResultType>> m_watcher;
};

template <typename T>
using ConcurrentCallTask = CustomTask<ConcurrentCallTaskAdapter<T>>;

} // namespace Tasking

QT_END_NAMESPACE

#endif // TASKING_CONCURRENTCALL_H
