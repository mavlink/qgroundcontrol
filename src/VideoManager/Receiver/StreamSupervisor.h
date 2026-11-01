#pragma once

#include <chrono>
#include <cstdint>

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>

#include "ExponentialBackoff.h"
#include "VideoReceiver.h"

/// \brief The reconnect policy both receivers share: the phase a stream is in, when a failed one connects again, and
/// when it gives up.
///
/// The receiver runs the sessions (a GStreamer pipeline, a QMediaPlayer) and tells the supervisor what becomes of them;
/// the supervisor reports the VideoReceiver::State that follows and signals reconnectDue() once a backoff has passed.
/// A failure reconnects after 1 s, doubling up to 30 s, unless it is permanent or autoReconnect is off: then the stream
/// halts until startOver(). A session that streamed had recovered, so the backoff starts over after it fails.
///
/// Threading: the receiver's thread.
class StreamSupervisor : public QObject
{
    Q_OBJECT

public:
    enum class Phase : uint8_t
    {
        Idle,          ///< Not streaming and not about to: not asked to, or before the first session.
        Connecting,    ///< A session runs, without data yet.
        Streaming,     ///< The session delivers.
        Reconnecting,  ///< After a failure, until the session that replaces it starts.
        Halted,        ///< Given up after a failure, until startOver() or autoReconnect resumes it.
    };

    struct FailureOutcome
    {
        bool halted = false;
        /// Until reconnectDue(), when not halted.
        std::chrono::milliseconds reconnectDelay{0};
    };

    enum class AutoReconnectChange : uint8_t
    {
        None,
        Halted,     ///< Turned off while a reconnect was pending: the stream halted.
        Resumable,  ///< Turned on while halted after a failure that wasn't permanent: the halt is lifted.
    };

    /// The cap bounds worst-case recovery after a long RF outage and stays below a typical RTSP server keepalive (60
    /// s).
    static constexpr ExponentialBackoff kDefaultBackoff{std::chrono::seconds(1), 2, std::chrono::seconds(30)};

    explicit StreamSupervisor(QObject* parent = nullptr, ExponentialBackoff backoff = kDefaultBackoff);

    Phase phase() const { return _phase; }

    VideoReceiver::State state() const { return stateFor(_phase); }

    static VideoReceiver::State stateFor(Phase phase);

    bool halted() const { return _phase == Phase::Halted; }

    /// A failure's backoff is running; reconnectDue() ends it.
    bool reconnectPending() const { return _reconnectTimer.isActive(); }

    /// The failure the stream halted or reconnects after.
    const QString& lastFailure() const { return _lastFailure; }

    /// Failures backed off from since the backoff last started over.
    int reconnectAttempts() const { return _backoff.attempts(); }

    /// For VideoStreamStats::reconnects: sessions that streamed since startOver(), less the first.
    int reconnects() const;
    /// For VideoStreamStats::uptimeMs: since the current session started streaming; zero while it isn't.
    std::chrono::milliseconds uptime() const;

    /// A user start or restart, or a new stream: the backoff and the reconnect count start over, a pending reconnect is
    /// dropped and a halt lifted. A running session keeps its phase.
    void startOver();
    /// The stream isn't wanted (anymore): Idle, without a reconnect.
    void stop();
    /// A new session runs.
    void sessionStarted();
    /// The session delivers. The first call per session counts it as streamed.
    void sessionStreaming();
    /// The session failed, or failed to start, and the receiver ended it. Reconnects after the backoff, unless
    /// @p permanent or not @p autoReconnect: then halts.
    FailureOutcome sessionFailed(const QString& message, bool permanent, bool autoReconnect);
    /// The config's autoReconnect became @p autoReconnect.
    AutoReconnectChange autoReconnectChanged(bool autoReconnect);

signals:
    void stateChanged(VideoReceiver::State state);
    /// A failure's backoff has passed: the receiver starts the next session.
    void reconnectDue();

private:
    void _setPhase(Phase phase);
    void _endSession();

    ExponentialBackoff _backoff;
    QTimer _reconnectTimer;
    Phase _phase = Phase::Idle;
    bool _haltedPermanently = false;
    /// The current session counted as streamed.
    bool _sessionStreamed = false;
    int _streamedSessions = 0;
    QElapsedTimer _sinceStreaming;
    QString _lastFailure;
};
