#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

// Owns a QThread plus a worker QObject moved onto it, with a bounded teardown.
//
// stopAndWait() quits the worker's event loop and joins up to a timeout. On a clean join it deletes the
// worker then the thread; on timeout (worker wedged in a JNI/USB call) it detaches both to self-delete on
// QThread::finished, and NEVER calls terminate() (which qFatals and can strand JNI global refs). A worker
// whose blocking call never returns also never emits finished, so it is leaked deliberately — the accepted
// cost of not terminating. Callers must close the OS handle before teardown so a blocked read can unwind.
//
// The worker is owned here: pass a heap-allocated, unparented QObject; do not delete it elsewhere.
class WorkerThread final
{
public:
    WorkerThread(QObject *worker, const QString &threadName)
        : _thread(new QThread), _worker(worker)
    {
        _thread->setObjectName(threadName);
        _worker->setParent(nullptr);
        (void) _worker->moveToThread(_thread);
    }

    ~WorkerThread()
    {
        // Best-effort guard if the owner forgot to stopAndWait(): detach immediately, never block in a dtor.
        (void) stopAndWait(0);
    }

    WorkerThread(const WorkerThread &) = delete;
    WorkerThread &operator=(const WorkerThread &) = delete;

    QThread *thread() const { return _thread; }
    bool isRunning() const { return _thread && _thread->isRunning(); }
    void start() { if (_thread) _thread->start(); }

    // Quit + join up to timeoutMs. Returns true if joined (worker+thread deleted), false if detached.
    // Idempotent: a no-op once already stopped.
    bool stopAndWait(int timeoutMs)
    {
        if (!_thread) {
            return true;
        }

        _thread->quit();
        if (_thread->wait(timeoutMs)) {
            // Joined: the worker is quiescent, so deleting it from the owner thread is safe.
            delete _worker;
            delete _thread;
            _worker = nullptr;
            _thread = nullptr;
            return true;
        }

        // Wedged: hand both to deleteLater on the eventual finish and release ownership.
        QObject::connect(_thread, &QThread::finished, _worker, &QObject::deleteLater);
        QObject::connect(_thread, &QThread::finished, _thread, &QObject::deleteLater);
        _worker = nullptr;
        _thread = nullptr;
        return false;
    }

private:
    QThread *_thread = nullptr;
    QObject *_worker = nullptr;
};
