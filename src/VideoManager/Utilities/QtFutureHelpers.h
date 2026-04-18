#pragma once

#include <optional>

#include <QtCore/QEventLoop>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QTimer>

/// Synchronously wait for a QFuture<T> to complete, with a timeout.
///
/// Spins a local QEventLoop so pending Qt events are processed during the wait
/// (prevents deadlocking callers that hold no locks). NOT safe to call from
/// a thread that holds a mutex needed by the future's producer.
///
/// \param future    The future to wait on.
/// \param timeoutMs Maximum wait in milliseconds. 0 = no timeout.
/// \returns std::nullopt on timeout; the result value otherwise.
///
/// Usage:
///   auto result = waitFor(myFuture, 5000);
///   if (!result) { /* timed out */ }
///   else { doSomethingWith(*result); }
template<typename T>
[[nodiscard]] std::optional<T> waitFor(QFuture<T> future, int timeoutMs = 0)
{
    if (future.isFinished())
        return future.result();

    QEventLoop loop;
    QFutureWatcher<T> watcher;
    QTimer timer;
    timer.setSingleShot(true);

    QObject::connect(&watcher, &QFutureWatcher<T>::finished, &loop, &QEventLoop::quit);
    if (timeoutMs > 0) {
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(timeoutMs);
    }

    watcher.setFuture(future);
    if (!watcher.isFinished())
        loop.exec();

    if (!watcher.isFinished())
        return std::nullopt; // timed out

    return watcher.result();
}
