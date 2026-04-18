#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <functional>

#include "VideoReceiver.h"
#include "VideoSourceResolver.h"

Q_DECLARE_LOGGING_CATEGORY(VideoRestartPolicyLog)

/// Owns per-stream restart intent, retry backoff, and stale-callback rejection.
///
/// VideoStream still performs the concrete receiver/FSM operations through
/// callbacks, but this class is the single place that decides whether a timeout
/// should soft-reconnect, hard-restart, auto-reconnect, or be ignored because
/// the stream generation has changed.
class VideoRestartPolicy
{
public:
    enum class Event : quint8
    {
        UserRestart,
        ReceiverTimeout,
        ReceiverStopped,
        ReconnectRequested,
    };

    enum class Action : quint8
    {
        Ignore,
        StartNow,
        StopNow,
        MarkRestartPending,
        SoftReconnect,
        HardRestart,
        StartAfterBackoff,
    };

    struct Decision
    {
        Action action = Action::Ignore;
        int delayMs = 0;
    };

    static constexpr int kSoftReconnectLimit = 3;
    static constexpr int kBaseBackoffMs = 1000;
    static constexpr int kMaxBackoffMs = 8000;

    explicit VideoRestartPolicy(QObject* timerContext);
    ~VideoRestartPolicy() = default;

    VideoRestartPolicy(const VideoRestartPolicy&) = delete;
    VideoRestartPolicy& operator=(const VideoRestartPolicy&) = delete;

    void noteSourceChanged();
    void noteStartRequested();
    void noteStopRequested();
    void noteReceiverFullyStarted();

    void requestRestart(VideoReceiver::ReceiverState state,
                        bool hasUri,
                        std::function<void()> start,
                        std::function<void()> stop);

    void handleReceiverTimeout(const QString& name,
                               const VideoSourceResolver::SourceDescriptor& source,
                               std::function<void()> pause,
                               std::function<void()> resume,
                               std::function<void()> hardRestart);

    void handleReceiverFullyStopped(const QString& name,
                                    const QString& uri,
                                    bool hasUri,
                                    std::function<void()> start);

    void handleReconnectRequested(const QString& name,
                                  bool hasUri,
                                  std::function<void()> requestStart);

    void cancelPending();

    [[nodiscard]] bool wantsRunning() const { return _wantRunning; }
    [[nodiscard]] int attempts() const { return _attempts; }
    [[nodiscard]] bool hasPendingRestart() const { return _timer.isActive(); }
    [[nodiscard]] Decision decide(Event event,
                                  VideoReceiver::ReceiverState state,
                                  const VideoSourceResolver::VideoSource& source,
                                  bool hasUri) const;

private:
    [[nodiscard]] int _peekBackoffMs() const;
    int _consumeBackoffMs();
    [[nodiscard]] bool _canSoftReconnect() const { return _attempts < kSoftReconnectLimit; }
    [[nodiscard]] bool _isCurrentGeneration(quint64 generation) const { return _generation == generation; }
    void _scheduleRestart(int delayMs, quint64 generation, std::function<void()> fn);

    quint64 _generation = 0;
    int _attempts = 0;
    bool _wantRunning = false;
    bool _restartPending = false;
    QTimer _timer;
    std::function<void()> _pendingFn;
};
