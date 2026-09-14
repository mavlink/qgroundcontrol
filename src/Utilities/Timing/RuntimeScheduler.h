#pragma once

#include <QtCore/QObject>

#include <chrono>
#include <functional>

/// One-thread monotonic clock and deferred callbacks, shared by production controllers and deterministic replay.
class RuntimeScheduler : public QObject
{
    Q_OBJECT
public:
    using TaskId = quint64;
    using Callback = std::function<void()>;
    static constexpr auto MAX_DELAY =
        std::chrono::duration_cast<std::chrono::microseconds>((std::chrono::nanoseconds::max) ());

    explicit RuntimeScheduler(QObject* parent = nullptr);
    ~RuntimeScheduler() override;
    virtual quint64 nowUs() const = 0;

    qint64 nowMs() const { return static_cast<qint64>(nowUs() / 1000); }

    /// Nonpositive delays defer until the next dispatch. Context must share this object's thread.
    /// Calls from other threads and delays above MAX_DELAY are rejected with task ID zero.
    /// Destroying context suppresses its callback. No ordering is promised for equal deadlines.
    virtual TaskId schedule(QObject* context, std::chrono::microseconds delay, Callback callback) = 0;
    /// Cancellation from another thread is ignored. All other access and destruction must occur on the owner thread.
    virtual void cancel(TaskId task) = 0;

protected:
    bool _canSchedule(QObject* context, std::chrono::microseconds delay, const Callback& callback) const;
    bool _isCurrentThread() const;
};
