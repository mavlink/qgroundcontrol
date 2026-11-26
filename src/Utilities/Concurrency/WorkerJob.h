#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>

namespace QGC {

template <typename Result>
class WorkerJob;

template <typename Work>
using WorkerResult = std::invoke_result_t<std::decay_t<Work>&>;

/// Runs @a work on a thread of QThreadPool::globalInstance() and calls @a done with its result on the thread of
/// @a context, from the event loop. Call it on that thread.
///
/// The job deletes itself before it calls @a done, and @a done gets the only instance of the result: it may delete
/// @a context or start the next job. Deleting the job, directly or with @a context, before then drops the result:
/// @a done is not called, and @a dropped, if given, gets the result instead, on the thread that ends the work, or
/// at once if it has ended. The work cannot be stopped and always runs to its end, so it must hold copies of what it
/// uses, never a pointer to an object of the caller. It must not throw.
/// @return The job, a child of @a context, already started. Hold it in a QPointer to drop the result
template <typename Work>
WorkerJob<WorkerResult<Work>>* runOnWorker(QObject* context, Work&& work, std::function<void(WorkerResult<Work>)> done,
                                           std::function<void(WorkerResult<Work>)> dropped = {});

/// The job that runOnWorker() returns
template <typename Result>
class WorkerJob : public QObject
{
public:
    using Handler = std::function<void(Result)>;

    ~WorkerJob() override
    {
        if (_dropped) {
            (void) _watcher->future().then(
                [dropped = std::move(_dropped)](QFuture<Result> ended) { dropped(ended.takeResult()); });
        }
    }

    /// Ends when the work has ended, also once the job is gone. A dropped handler uses the one continuation that the
    /// future of the work has: do not add one to this future of a job that has a dropped handler.
    QFuture<void> ended() const { return _watcher->future(); }

private:
    template <typename Work>
    WorkerJob(QObject* context, Work&& work, Handler done, Handler dropped)
        : QObject(context)
        , _watcher(new QFutureWatcher<Result>(this))
        , _done(std::move(done))
        , _dropped(std::move(dropped))
    {
        // Connected before the future is set, as Qt asks
        (void) connect(_watcher, &QFutureWatcherBase::finished, this, [this]() { _finish(); });
        _watcher->setFuture(QtConcurrent::run(std::forward<Work>(work)));
    }

    void _finish()
    {
        Result result = _watcher->future().takeResult();
        const Handler done = std::exchange(_done, nullptr);
        _dropped = nullptr;
        delete this;
        done(std::move(result));
    }

    template <typename Work>
    friend WorkerJob<WorkerResult<Work>>* runOnWorker(QObject* context, Work&& work,
                                                      std::function<void(WorkerResult<Work>)> done,
                                                      std::function<void(WorkerResult<Work>)> dropped);

    QFutureWatcher<Result>* _watcher = nullptr;
    Handler _done;
    Handler _dropped;
};

template <typename Work>
WorkerJob<WorkerResult<Work>>* runOnWorker(QObject* context, Work&& work, std::function<void(WorkerResult<Work>)> done,
                                           std::function<void(WorkerResult<Work>)> dropped)
{
    return new WorkerJob<WorkerResult<Work>>(context, std::forward<Work>(work), std::move(done), std::move(dropped));
}

}  // namespace QGC
