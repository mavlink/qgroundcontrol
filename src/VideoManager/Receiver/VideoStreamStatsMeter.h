#pragma once

#include <deque>
#include <functional>
#include <optional>

#include <QtCore/QtTypes>

struct VideoStreamStats;

/// Turns a stream's cumulative counters, sampled about once a second, into the rates and totals of VideoStreamStats.
///
/// Counters count from the start of the stream. A counter below its previous value (the element counting it was
/// replaced) has restarted from zero, so totals keep growing and rates never go negative.
///
/// fps and bitrateKbps average over the last kRateWindowIntervals intervals: frames and packets arrive in bursts, so a
/// one-second count of a steady 15 fps stream reads anywhere from 12 to 18. droppedFps and lossPercent cover the last
/// interval only, as they tell whether the stream degrades now.
class VideoStreamStatsMeter
{
public:
    static constexpr int kRateWindowIntervals = 3;

    /// Monotonic milliseconds.
    using Clock = std::function<qint64()>;

    struct RtpCounters
    {
        quint64 packetsReceived = 0;
        quint64 packetsLost = 0;
    };

    struct Counters
    {
        quint64 framesDisplayed = 0;
        quint64 bytesReceived = 0;
        quint64 framesDropped = 0;
        /// Only for a stream that goes through an RTP jitter buffer.
        std::optional<RtpCounters> rtp;
    };

    /// @p clock defaults to the steady clock.
    explicit VideoStreamStatsMeter(Clock clock = {});

    /// A new stream starts: its counters start from zero, and the next update() only sets the baseline for rates.
    void reset();

    /// Fills the fields of @p stats that @p counters measure: valid, fps, bitrateKbps, framesDropped, droppedFps,
    /// packetsLost and lossPercent (-1 without RTP counters). Returns false, leaving @p stats alone, for the first
    /// sample after reset() and for a sample taken no later than the previous one.
    bool update(const Counters& counters, VideoStreamStats& stats);

private:
    /// Running totals at one sample, for the rates over the window.
    struct RatePoint
    {
        qint64 ms = 0;
        quint64 framesDisplayed = 0;
        quint64 bytesReceived = 0;
    };

    Clock _clock;
    std::optional<qint64> _lastSampleMs;
    Counters _last;
    quint64 _framesDisplayed = 0;
    quint64 _bytesReceived = 0;
    quint64 _framesDropped = 0;
    quint64 _packetsLost = 0;
    /// The last kRateWindowIntervals + 1 samples, oldest first.
    std::deque<RatePoint> _window;
};
