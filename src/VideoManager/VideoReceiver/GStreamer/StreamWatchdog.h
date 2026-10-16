#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

#include <QtCore/QtTypes>

#include "ExponentialBackoff.h"

/// Health of one receiver's stream: notices when frames stop arriving, and paces the reconnects after a failure.
///
/// Times are monotonic milliseconds from now(), passed in so the policy can be checked without waiting on a clock.
/// Threading: noteSourceFrame() and noteSinkFrame() run on streaming threads; everything else on the receiver's worker.
class StreamWatchdog
{
public:
    enum class Stall
    {
        None,
        Source,   ///< No frame reached the tee within the timeout.
        Decoder,  ///< Decoding, but no frame reached the video sink within twice the timeout.
    };

    struct Result
    {
        Stall stall = Stall::None;
        qint64 silentMs = 0;  ///< How long the stalled side has gone without a frame.
    };

    static qint64 now();

    /// Watches a new pipeline, which stalls after @p timeoutSec without frames (at least 1 s).
    void start(uint32_t timeoutSec);

    /// Returns the number of source frames since start().
    quint64 noteSourceFrame(qint64 nowMs);
    /// Returns true for the first sink frame since start() or clearSinkFrames().
    bool noteSinkFrame(qint64 nowMs);

    void clearSinkFrames() { _lastSinkFrameMs.store(0, std::memory_order_relaxed); }

    /// The first check without a source frame starts the source timeout.
    Result check(qint64 nowMs, bool decoding);

    /// Delay before the next reconnect: 1 s, 2 s, 4 s, ... capped at 30 s. A pipeline that delivered frames before it
    /// failed had recovered, so the backoff starts over after it.
    std::chrono::seconds nextReconnectDelay();

    /// A user start or stop starts the backoff over.
    void resetBackoff() { _reconnectBackoff.reset(); }

    int reconnectAttempts() const { return _reconnectBackoff.attempts(); }

private:
    qint64 _timeoutMs = 0;
    // The cap bounds worst-case recovery after a long RF outage and stays below a typical RTSP server keepalive (60 s).
    ExponentialBackoff _reconnectBackoff{std::chrono::seconds(1), 2, std::chrono::seconds(30)};
    std::atomic<quint64> _sourceFrames = 0;
    std::atomic<qint64> _lastSourceFrameMs = 0;
    std::atomic<qint64> _lastSinkFrameMs = 0;
};
