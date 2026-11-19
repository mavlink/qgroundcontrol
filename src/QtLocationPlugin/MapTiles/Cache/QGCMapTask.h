#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QString>

class QGCCacheWorker;

/// Work for the thread of the tile cache (QGCCacheWorker). QGCMapTasks makes the tasks of the cache.
class QGCMapTask : public QObject
{
    Q_OBJECT

public:
    /// What the task does on the worker thread, where it has the worker-thread functions of \a worker and can emit
    /// progress() of \a task. It returns what went wrong, or an empty string.
    using Work = std::function<QString(QGCCacheWorker& worker, QGCMapTask& task)>;

    /// \a name tells the user which task failed. A task without \a work only starts the worker.
    explicit QGCMapTask(const QString& name, Work work = {}, QObject* parent = nullptr)
        : QObject(parent)
        , _name(name)
        , _work(std::move(work))
    {}

    const QString& name() const { return _name; }

    const Work& work() const { return _work; }

    /// For the worker: ends the task
    void finish(const QString& error) { emit finished(error); }

signals:
    /// The outcome of the task: \a error is empty for a task that succeeded. Emitted once, by the worker thread for
    /// a task that ran, and within QGCCacheWorker::enqueueTask() for a task that the worker cannot run. A task that
    /// is still queued when the worker stops is deleted without it.
    void finished(const QString& error);

    /// How much of an import or an export is done
    void progress(int percentage);

private:
    const QString _name;
    const Work _work;
};
