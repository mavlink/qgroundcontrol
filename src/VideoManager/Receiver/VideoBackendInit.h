#pragma once

#include <cstdint>

#include <QtCore/QFuture>
#include <QtCore/QObject>

/// Runs the video backend's global initialization (VideoBackend::initialize()) once, off the GUI thread, and tells
/// when it is over. A backend that needs none, as every backend under unit tests, is ready as soon as it is started.
///
/// Threading: the GUI thread.
class VideoBackendInit : public QObject
{
    Q_OBJECT

public:
    enum class State : uint8_t
    {
        NotStarted,
        Pending,
        Ready,
        Failed,
    };

    explicit VideoBackendInit(QObject* parent = nullptr);

    State state() const { return _state; }

    /// Ready or failed: either way the backend makes receivers, after a failure the ones that need no initialization.
    bool isFinished() const { return (_state == State::Ready) || (_state == State::Failed); }

    /// Starts the initialization, unless it was started already: Pending until finished(), or Ready at once for a
    /// backend that needs none.
    void start();
    /// Takes @p initialization for the backend's: Pending until it finishes. What start() does with the backend's
    /// own, and tests with one they finish themselves.
    void start(const QFuture<bool>& initialization);
    /// Blocks until the initialization is over, starting it if needed; a pending one emits finished() before this
    /// returns. True when the backend is ready.
    bool waitUntilFinished();

signals:
    /// A pending initialization is over: state() is Ready when @p ready, else Failed. Emitted once.
    void finished(bool ready);

private:
    void _finish(bool ready);

    State _state = State::NotStarted;
    QFuture<bool> _initialization;
};
