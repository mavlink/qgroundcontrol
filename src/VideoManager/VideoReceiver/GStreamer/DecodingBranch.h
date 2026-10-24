#pragma once

#include <atomic>
#include <gst/gst.h>

#include <QtCore/QString>

#include "BranchContext.h"
#include "GstScoped.h"

namespace GstQgc {
struct PathStats;
}

/// \brief The display branch behind the session's tee: leaky queue ! valve [! decodebin3 ! video sink].
///
/// Takes the receiver's video sink, inserts decodebin3 once the source streams and links its pad to the sink, asking
/// upstream for a keyframe so the picture starts soon. Decoding is reported from the first frame that reaches the sink.
/// A failing or stalled decoder is replaced without touching the rest of the pipeline. The sink outlives the session:
/// the branch holds a ref and takes it out of the pipeline again.
///
/// The queue drops the oldest frame when the decoder falls behind, so the tee (and with it the recording) never waits
/// for the decoder. A decoder can't recover a dropped compressed frame, so after a drop the branch also drops delta
/// frames until the next keyframe, which it requests: the picture freezes briefly instead of smearing for a whole GOP.
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
        /// The decoder kept failing: kMaxDecoderRestarts replacements in a row produced no frame.
        virtual void onDecoderFailing() = 0;

    protected:
        ~Listener() = default;
    };

    static constexpr int kMaxDecoderRestarts = 3;

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

    bool decoding() const { return _decoding; }

    /// A decoder is inserted, so frames are due at the sink.
    bool displaying() const { return _decoder != nullptr; }

    GstElement* valve() const { return _valve; }

    /// Whether @p source is (inside) the current decoder or video sink, or the branch's queue or valve. Errors from
    /// anything else of the branch are left over from a replaced decoder.
    bool isCurrent(GstObject* source) const;

    /// The frames the video sink delivered since setVideoSink(), from qgcqvideosink's "frame-stats"; empty for other
    /// sinks.
    GstQgc::PathStats sinkPathStats() const;

private:
    void _insertDecoder();
    /// Closes the valve and takes the video sink and the decoder out of the pipeline; keeps the sink.
    void _removeDecoder();
    void _linkDecoderPad(GstPad* pad);
    void _onFirstFrame(quint64 insertion);
    void _setDecoding(bool decoding);
    void _setDecoderName(const QString& name);

    static void _onQueueOverrun(GstElement* queue, gpointer data);
    static GstPadProbeReturn _gateProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn _sinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static void _onDecoderPadAdded(GstElement* decoder, GstPad* pad, gpointer data);
    static void _onDecoderElementAdded(GstBin* decodebin, GstBin* subBin, GstElement* element, gpointer data);

    const BranchContext _context;
    Listener& _listener;
    const bool _lowLatency;

    // Owned by the pipeline.
    GstElement* _queue = nullptr;
    GstElement* _valve = nullptr;

    GStreamer::GstElementPtr _sink;
    GStreamer::GstElementPtr _decoder;
    // The sink's "frame-stats" when setVideoSink() took it: the sink outlives sessions and its counters don't restart.
    GStreamer::GstStructurePtr _sinkStatsAtStart;
    GStreamer::PadProbe _sinkFrameProbe;
    GStreamer::PadProbe _keyframeGate;
    GStreamer::ScopedSignal _queueOverrun;
    GStreamer::ScopedSignal _decoderPadAdded;
    GStreamer::ScopedSignal _decoderElementAdded;

    /// Set when the queue drops a frame; delta frames are dropped while it is set.
    std::atomic<bool> _gateClosed = false;
    std::atomic<int> _gateDrops = 0;
    /// Counts decoder insertions, so a first-frame report from a replaced decoder is ignored.
    std::atomic<quint64> _insertions = 0;

    bool _streaming = false;
    bool _decoding = false;
    int _restartsWithoutFrame = 0;
    QString _decoderName;
};
