#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>

#include <QtCore/QtTypes>

/// Health of one receiver's stream: notices when frames stop arriving. StreamSupervisor paces the reconnects.
///
/// It watches two sides: the source, whose frames are the stream as it arrives, and the sink, whose frames are the
/// pictures that reach the output, due only while the receiver displays. A receiver that can't tell its stream apart
/// from its pictures notes sink frames alone.
///
/// Times are monotonic milliseconds from now(), passed in so the policy can be checked without waiting on a clock.
/// Threading: noteSourceFrame() and noteSinkFrame() may run on the threads the stream arrives on; everything else on
/// the one thread that checks.
class StreamWatchdog
{
public:
    enum class Stall
    {
        None,
        Source,   ///< No frame arrived within the timeout.
        Decoder,  ///< Displaying, but no frame reached the output within the sink's limits.
    };

    struct Result
    {
        Stall stall = Stall::None;
        qint64 silentMs = 0;  ///< How long the stalled side has gone without a frame.
    };

    /// How long the sink may go without a frame while displaying.
    struct SinkLimits
    {
        /// In timeouts. Two by default: a decoder may lag its source that far before it counts as stalled.
        int timeouts = 2;
        /// On top of that, before the first sink frame since startSinkClock().
        std::chrono::milliseconds firstFrameGrace{0};
    };

    static qint64 now();

    /// Watches a new session, whose source stalls after @p timeout (at least 1 s) without frames. Before its first
    /// frame it stalls after @p timeout plus @p firstFrameGrace, or never without one (nullopt).
    void start(std::chrono::seconds timeout, std::optional<std::chrono::milliseconds> firstFrameGrace, SinkLimits sink);
    /// With the default SinkLimits.
    void start(std::chrono::seconds timeout,
               std::optional<std::chrono::milliseconds> firstFrameGrace = std::chrono::milliseconds{0});

    /// Returns the number of source frames since start().
    quint64 noteSourceFrame(qint64 nowMs);
    /// Returns true for the first sink frame since startSinkClock().
    bool noteSinkFrame(qint64 nowMs);

    /// The sink owes no frame until @p nowMs, so its silence counts from here: one was kept from the decoder, as by
    /// the display's keyframe gate, or there was no output to show it.
    void excuseSinkSilence(qint64 nowMs);

    /// The sink's frames are due from @p nowMs, as a decoder starts displaying: its silence counts from here, and the
    /// next sink frame is the first.
    void startSinkClock(qint64 nowMs);

    /// The first check without a source frame starts the source timeout, where the first frame has one. @p displaying:
    /// sink frames are due, as once a decoder is inserted.
    Result check(qint64 nowMs, bool displaying);

private:
    qint64 _timeoutMs = 0;
    /// nullopt: the first frame may take any time.
    std::optional<qint64> _firstFrameGraceMs = 0;
    qint64 _sinkTimeoutMs = 0;
    qint64 _firstSinkFrameGraceMs = 0;
    std::atomic<quint64> _sourceFrames = 0;
    std::atomic<qint64> _lastSourceFrameMs = 0;
    std::atomic<qint64> _lastSinkFrameMs = 0;
    std::atomic<bool> _sinkFrameSeen = false;
};
