#pragma once

#include <QtCore/QObject>

#include <chrono>
#include <functional>

/// One-thread monotonic clock and deferred callbacks, shared by production controllers and deterministic replay.
class GPSRuntimeScheduler : public QObject
{
    Q_OBJECT
public:
    using TaskId = quint64;
    using Callback = std::function<void()>;

    explicit GPSRuntimeScheduler(QObject* parent = nullptr);
    ~GPSRuntimeScheduler() override;
    virtual quint64 nowUs() const = 0;

    qint64 nowMs() const { return static_cast<qint64>(nowUs() / 1000); }

    /// A zero delay still defers invocation. Destroying context suppresses its callback. Zero means not scheduled.
    virtual TaskId schedule(QObject* context, std::chrono::microseconds delay, Callback callback) = 0;
    virtual void cancel(TaskId task) = 0;
};
