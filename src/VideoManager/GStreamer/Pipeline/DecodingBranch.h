#pragma once

#include <array>
#include <atomic>
#include <gst/gst.h>
#include <memory>

#include <QtCore/QString>

#include "BranchContext.h"
#include "GstScoped.h"
#include "HwBuffers/common/GstHwPathTelemetry.h"
#include "TeeBranchHead.h"

namespace GstQgc {
struct PathStats;
}

/// \brief The display branch behind the session's tee: leaky queue ! valve [! decodebin3 ! video sink].
///
/// Takes the receiver's video sink, inserts decodebin3 once the source streams and links its pad to the sink, asking
/// upstream for a keyframe so the picture starts soon. Decoding is reported from the first frame that reaches the sink.
/// A failing or stalled decoder is replaced without touching the rest of the pipeline; one that keeps failing, or that
/// can't be inserted at all, is reported to the listener. A hardware decoder whose replacement brought no frame either
/// is taken out of autoplug before the next one, so decodebin3 plugs the decoder ranked after it. The sink outlives
/// the session: the branch holds a ref and takes it out of the pipeline again.
///
/// The queue drops the oldest frame when the decoder falls behind, so the tee (and with it the recording) never waits
/// for the decoder. A decoder can't recover a dropped compressed frame, so after a drop the branch also drops delta
/// frames until the next keyframe, which it requests: the picture freezes briefly instead of smearing for a whole GOP.
/// It waits two seconds at most, and says so when no keyframe came, so a stream without keyframes goes on.
///
/// Threading: the receiver's worker, except the probes and the decoder's signal handlers (streaming threads), which
/// hand their work to the worker through BranchContext::post().
class DecodingBranch
{
public:
    class Listener
    {
    public:
        virtual void onDecodingChanged(bool decoding) = 0;
        virtual void onDecoderNameChanged(const QString& name) = 0;
        /// The decoder kept failing: kMaxDecoderRestarts replacements in a row produced no frame, or one couldn't be
        /// inserted, which leaves the stream without a picture until it is set up anew.
        virtual void onDecoderFailing() = 0;

    protected:
        ~Listener() = default;
    };

    static constexpr int kMaxDecoderRestarts = 3;

    /// What the branch measured for VideoStreamStats.
    struct StatsSample
    {
        /// Since the branch was built: frames the video sink delivered that no newer frame replaced before display.
        quint64 framesDisplayed = 0;
        /// Since the branch was built: frames the queue and its keyframe gate dropped, frames the video sink dropped
        /// because they came too late for their time, and frames it delivered that a newer frame replaced before
        /// display.
        quint64 framesDropped = 0;
        /// The path most frames took to the display since the previous sample, as GstHw::toString() names it; the
        /// previous one when no frame arrived, empty before the first frame.
        QString displayPath;
        /// Since the branch was built: frames a GPU path was tried for but the CPU delivered.
        quint64 gpuFallbacks = 0;
        /// Since the branch was built: decoders replaced by restartDecoder().
        quint64 decoderRestarts = 0;
    };

    /// @p lowLatency: frames are shown as soon as decoded instead of at their presentation time.
    DecodingBranch(const BranchContext& context, Listener& listener, bool lowLatency);
    ~DecodingBranch();

    DecodingBranch(const DecodingBranch&) = delete;
    DecodingBranch& operator=(const DecodingBranch&) = delete;

    /// Adds the branch's leaky queue and closed valve behind the tee.
    [[nodiscard]] bool build();
    /// Takes the video sink and decoder out of the pipeline, which must be in NULL, and releases the sink.
    void reset();

    /// Decodes into @p sink from now on, or stops decoding for nullptr.
    void setVideoSink(GstElement* sink);
    /// The first frame reached the tee: inserts the decoder when a video sink is waiting for it.
    void onSourceStreaming();
    /// Replaces the decoder and resets the sink, leaving the rest of the pipeline (and a recording) running.
    void restartDecoder(const char* reason);
    /// The pipeline got the graphics device Qt renders with after its elements had asked for it
    /// (HwBuffers::kGraphicsDeviceServedMessage). A hardware decoder plugged meanwhile decodes on a device of its own,
    /// which it keeps, so its frames reach Qt through the CPU: replaces it by one that takes the pipeline's device.
    /// Not counted as a restart: the decoder didn't fail.
    void onGraphicsDeviceServed();

    /// A decoder is inserted, so frames are due at the sink.
    bool displaying() const { return _decoder != nullptr; }

    GstElement* valve() const { return _head.valve(); }

    /// Whether @p source is (inside) the current decoder or video sink, or the branch's queue or valve. Errors from
    /// anything else of the branch are left over from a replaced decoder.
    bool isCurrent(GstObject* source) const;

    /// The frames the video sink delivered since setVideoSink(), from qgcqvideosink's "frame-stats", followed by the
    /// process-wide GPU-path counters since then (HwBuffers::extraPathStatsSince()); empty for other sinks.
    GstQgc::PathStats sinkPathStats() const;

    /// Not const: displayPath compares the sink's counts with the previous sample's.
    StatsSample sampleStats();

    /// The factory of the decoder decodebin3 plugged last; empty while no decoder is inserted or it has plugged none.
    const QString& decoderName() const { return _decoderName; }

    /// Whether decoderName() is a hardware decoder.
    bool hardwareDecoder() const { return _hardwareDecoder; }

private:
    /// Inserts the decoder and the video sink, or reports onDecoderFailing() when that fails.
    void _insertDecoder();
    /// False, logged, with neither the decoder nor the video sink left in the pipeline.
    [[nodiscard]] bool _tryInsertDecoder();
    /// Closes the valve and takes the video sink and the decoder out of the pipeline; keeps the sink.
    void _removeDecoder();
    void _linkDecoderPad(GstPad* pad);
    /// Takes the hardware decoder decodebin3 plugged out of autoplug, when another decoder can take the stream.
    void _demoteFailedDecoder();
    void _onFirstFrame(quint64 insertion);
    void _setDecoding(bool decoding);
    void _setDecoderName(const QString& name);
    /// Adds the frames the current video sink delivered since setVideoSink() to the branch's totals, before the sink
    /// is replaced: its counters don't restart, and the next sink's count from their own baseline.
    void _retireSinkCounts();
    /// Adds the frames the video sink dropped as late since the previous call to the branch's total.
    void _countLateFrames();

    static void _onQueueOverrun(GstElement* queue, gpointer data);
    static GstPadProbeReturn _gateProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn _sinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static void _onDecoderPadAdded(GstElement* decoder, GstPad* pad, gpointer data);
    static void _onDecoderElementAdded(GstBin* decodebin, GstBin* subBin, GstElement* element, gpointer data);

    const BranchContext _context;
    Listener& _listener;
    const bool _lowLatency;

    TeeBranchHead _head;

    GStreamer::GstElementPtr _sink;
    GStreamer::GstElementPtr _decoder;
    // The sink's "frame-stats" when setVideoSink() took it: the sink outlives sessions and its counters don't restart.
    GStreamer::GstStructurePtr _sinkStatsAtStart;
    // The process-wide GPU-path counters then, which never restart either.
    GstHwPathTelemetry::Snapshot _gpuCountersAtStart;
    // The sink's per-path delivered counts at the previous sampleStats(), absolute like its "frame-stats".
    std::array<quint64, GstHw::kPathCount> _pathFramesAtSample{};
    // Displayed, superseded and GPU-fallback frames of the sinks replaced since the branch was built.
    quint64 _retiredFramesDisplayed = 0;
    quint64 _retiredFramesSuperseded = 0;
    quint64 _retiredGpuFallbacks = 0;
    // Frames the video sinks dropped as late since the branch was built, and the current sink's own count of them
    // when they were last added.
    quint64 _lateFramesDropped = 0;
    quint64 _sinkLateFramesSeen = 0;
    QString _displayPath;
    GStreamer::PadProbe _sinkFrameProbe;
    GStreamer::PadProbe _keyframeGate;
    GStreamer::ScopedSignal _decoderPadAdded;
    GStreamer::ScopedSignal _decoderElementAdded;

    /// The queue's overruns since the last buffer it pushed: the frames it leaked, if it marks the next one DISCONT.
    std::atomic<quint64> _overruns = 0;
    /// Set when the queue drops a frame; delta frames are dropped while it is set.
    std::atomic<bool> _gateClosed = false;
    /// When the gate closed, as StreamWatchdog::now() tells time.
    std::atomic<qint64> _gateClosedAtMs = 0;
    /// The gate already warned that it opened without a keyframe.
    std::atomic<bool> _gateTimeoutWarned = false;
    /// Frames the queue and the keyframe gate dropped.
    std::atomic<quint64> _framesDropped = 0;
    /// Counts decoder insertions, so a first-frame report from a replaced decoder is ignored.
    std::atomic<quint64> _insertions = 0;

    bool _streaming = false;
    bool _decoding = false;
    int _restartsWithoutFrame = 0;
    quint64 _decoderRestarts = 0;
    QString _decoderName;
    bool _hardwareDecoder = false;
    // The decoder decodebin3 plugged last, of the factory _decoderName names.
    std::shared_ptr<GstElement> _pluggedDecoder;
};
