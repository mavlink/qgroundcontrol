#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

#include <QtCore/QtTypes>

/// Health of one receiver's stream: notices when frames stop arriving. StreamSupervisor paces the reconnects.
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
        Decoder,  ///< Displaying, but no frame reached the video sink within twice the timeout.
    };

    struct Result
    {
        Stall stall = Stall::None;
        qint64 silentMs = 0;  ///< How long the stalled side has gone without a frame.
    };

    static qint64 now();

    /// Watches a new pipeline, which stalls after @p timeout (at least 1 s) without frames, or after @p timeout plus
    /// @p firstFrameGrace before its first frame.
    void start(std::chrono::seconds timeout, std::chrono::milliseconds firstFrameGrace = std::chrono::milliseconds{0});

    /// Returns the number of source frames since start().
    quint64 noteSourceFrame(qint64 nowMs);
    /// Returns true for the first sink frame since startSinkClock().
    bool noteSinkFrame(qint64 nowMs);

    /// A decoder starts displaying at @p nowMs: its silence counts from here until the first sink frame.
    void startSinkClock(qint64 nowMs);

    /// The first check without a source frame starts the source timeout. @p displaying: a decoder is inserted, so sink
    /// frames are due.
    Result check(qint64 nowMs, bool displaying);

private:
    qint64 _timeoutMs = 0;
    qint64 _firstFrameGraceMs = 0;
    std::atomic<quint64> _sourceFrames = 0;
    std::atomic<qint64> _lastSourceFrameMs = 0;
    std::atomic<qint64> _lastSinkFrameMs = 0;
    std::atomic<bool> _sinkFrameSeen = false;
};
