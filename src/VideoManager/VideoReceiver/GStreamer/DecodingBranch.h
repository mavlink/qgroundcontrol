#pragma once

#include <atomic>
#include <gst/gstbin.h>
#include <gst/gstpad.h>

#include <QtCore/QMutex>
#include <QtCore/QString>

#include "GstScoped.h"

class GstVideoReceiver;

namespace GstQgc {
struct PathStats;
}

/// \brief The display branch behind GstVideoReceiver's tee: leaky queue ! valve [! decodebin3 ! video sink].
///
/// Holds the caller's video sink until the source streams, then inserts decodebin3 and links its pad to the sink.
/// The caller keeps its own ref on the sink; the branch takes one more, and the pipeline its own while the sink is
/// inside. Decoding is reported from the first frame that reaches the sink; taking the sink out again clears the frame
/// its QVideoSink displays. Also keeps the name of the decoder decodebin3 plugged, which the receiver exposes. Reports
/// through the receiver's signals.
///
/// Threading: the receiver's worker, except the sink probe and the decoder-added callback (streaming threads) and
/// onBusCapsInfo() (whichever thread posts). decoderName() is thread-safe.
class DecodingBranch
{
public:
    explicit DecodingBranch(GstVideoReceiver& receiver);

    /// Adds the branch's leaky queue and closed valve to @p pipeline behind @p tee.
    [[nodiscard]] bool build(GstElement* pipeline, GstElement* tee);
    /// Detaches the video sink and decoder and forgets the pipeline, which must be in NULL with its callbacks stopped.
    void reset();

    /// Takes @p videoSink and, once @p streaming, inserts the decoder. Reports onStartDecodingComplete.
    void start(GstElement* videoSink, bool hasWidget, bool streaming);
    /// Tears the branch down at once (there is nothing to flush). Reports onStopDecodingComplete.
    void stop();
    /// The source pad is linked: inserts the decoder when a video sink is waiting for it.
    void onSourceStreaming();
    void onDecoderPad(GstPad* pad);

    GstElement* decoder() const { return _decoder.get(); }

    bool isDecoding() const { return _decoding.load(std::memory_order_relaxed); }

    /// The sink's negotiated raw caps (GstQgc::kCapsInfoMessage).
    void onBusCapsInfo(GstMessage* message);

    QString decoderName() const;

    /// The frames the video sink delivered since start() took it, from qgcqvideosink's "frame-stats"; empty for other
    /// sinks.
    GstQgc::PathStats sinkPathStats() const;

private:
    /// Puts the video sink into the pipeline and decodebin3 between it and the valve, then opens the valve.
    bool _insertDecoder();
    bool _addDecoder();
    void _ensureVideoSinkInPipeline();
    bool _addVideoSink(GstPad* pad);
    void _onFirstFrame();
    void _shutdown();
    /// Has the receiver's QVideoSink controllers drop the frame on display, which pins the last decoder buffer.
    void _releaseDisplayedFrame();

    static void _onDecoderElementAdded(GstBin* decodebin, GstBin* subBin, GstElement* element, gpointer data);
    static GstPadProbeReturn _sinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

    GstVideoReceiver& _receiver;
    // Borrowed: the receiver owns the pipeline, the pipeline the valve.
    GstElement* _pipeline = nullptr;
    GstElement* _valve = nullptr;

    GStreamer::GstElementPtr _decoder;
    GStreamer::GstElementPtr _videoSink;
    // The sink's "frame-stats" when start() took it: the sink outlives sessions and its counters don't restart.
    GStreamer::GstStructurePtr _sinkStatsAtStart;
    GStreamer::PadProbe _sinkFrameProbe;
    std::atomic<bool> _decoding = false;  ///< Read by the receiver's tee probe log.

    mutable QMutex _decoderNameMutex;     // Written on the worker, read from any thread.
    QString _decoderName;
};
