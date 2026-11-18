#include "DecodingBranch.h"

#include <gst/gst.h>

#include "GStreamerDecoderRanks.h"
#include "GStreamerHelpers.h"
#include "GStreamerLibavWorkaround.h"
#include "GstVideoReceiver.h"
#include "HwBuffers/HwBuffers.h"
#include "PipelineDotDumper.h"
#include "StreamWatchdog.h"
#include "gstqgc/GstQgcSinkStats.h"

namespace {

constexpr guint kDisplayQueueMaxBuffers = 2;
// How long the keyframe gate waits for a keyframe: time for one that was asked for to arrive, and most streams' own
// keyframe interval. A stream without keyframes (intra refresh), or one whose sender takes no requests, would
// otherwise freeze for good, or for a whole GOP, after a dropped frame.
constexpr qint64 kMaxGateMs = 2000;

/// decodebin3's "select-stream": only video is decoded, so that no audio decoder is built. decodebin3 asks on the
/// streaming thread that brings the stream. A SELECT_STREAMS event can't be sent from there: sending it takes the
/// decoder's state lock, which a state change holds while it waits for that thread to stop.
gint decodeVideoOnly([[maybe_unused]] GstElement* decodebin, [[maybe_unused]] GstStreamCollection* collection,
                     GstStream* stream, [[maybe_unused]] gpointer data)
{
    return ((gst_stream_get_stream_type(stream) & GST_STREAM_TYPE_VIDEO) != 0) ? 1 : 0;
}

/// Sets "sync" on @p sink, or on the sinks inside it when it is a bin without the property.
void setSinkSync(GstElement* sink, bool sync)
{
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(sink), "sync")) {
        g_object_set(sink, "sync", sync, nullptr);
        return;
    }
    if (!GST_IS_BIN(sink)) {
        return;
    }
    GstIterator* const sinks = gst_bin_iterate_sinks(GST_BIN(sink));
    (void) gst_iterator_foreach(
        sinks,
        [](const GValue* item, gpointer data) {
            auto* const child = GST_ELEMENT(g_value_get_object(item));
            if (g_object_class_find_property(G_OBJECT_GET_CLASS(child), "sync")) {
                g_object_set(child, "sync", GPOINTER_TO_INT(data), nullptr);
            }
        },
        GINT_TO_POINTER(sync ? TRUE : FALSE));
    gst_iterator_free(sinks);
}

/// The frames the base sink @p element dropped because they came too late for their time, from its "stats"
/// (GStreamer 1.18); 0 without them.
guint64 lateFramesDroppedBy(GstElement* element)
{
    GParamSpec* const spec = g_object_class_find_property(G_OBJECT_GET_CLASS(element), "stats");
    if (!spec || (G_PARAM_SPEC_VALUE_TYPE(spec) != GST_TYPE_STRUCTURE)) {
        return 0;
    }
    GstStructure* rawStats = nullptr;
    g_object_get(element, "stats", &rawStats, nullptr);
    const GStreamer::GstStructurePtr stats = GStreamer::adoptStructure(rawStats);
    return GstQgc::frameStatsField(stats.get(), "dropped");
}

/// The frames @p sink, or the sinks inside it when it is a bin, dropped as late. A sink starts its count over when it
/// flushes or leaves PLAYING.
quint64 sinkLateFramesDropped(GstElement* sink)
{
    if (!sink) {
        return 0;
    }
    if (!GST_IS_BIN(sink)) {
        return lateFramesDroppedBy(sink);
    }
    guint64 dropped = 0;
    GstIterator* const sinks = gst_bin_iterate_sinks(GST_BIN(sink));
    (void) gst_iterator_foreach(
        sinks,
        [](const GValue* item, gpointer data) {
            *static_cast<guint64*>(data) += lateFramesDroppedBy(GST_ELEMENT(g_value_get_object(item)));
        },
        &dropped);
    gst_iterator_free(sinks);
    return dropped;
}

/// Whether decodebin3 @p decodebin has plugged a hardware decoder. Read from its elements rather than from what the
/// branch was told of them, which reaches the worker after the decoder is in place.
bool hasHardwareDecoder(GstElement* decodebin)
{
    bool hardware = false;
    GstIterator* const elements = gst_bin_iterate_recurse(GST_BIN(decodebin));
    const auto note = [](const GValue* item, gpointer data) {
        GstElementFactory* const factory = gst_element_get_factory(GST_ELEMENT(g_value_get_object(item)));
        if (factory && gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER) &&
            GStreamer::isHardwareDecoderFactory(factory)) {
            *static_cast<bool*>(data) = true;
        }
    };
    while (gst_iterator_foreach(elements, note, &hardware) == GST_ITERATOR_RESYNC) {
        gst_iterator_resync(elements);
    }
    gst_iterator_free(elements);
    return hardware;
}

/// Whether a live source feeds @p decodebin, as the decoders inside it ask their own upstream.
bool fedByLiveSource(GstElement* decodebin)
{
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(decodebin, "sink"));
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_latency());
    gboolean live = FALSE;
    if (sinkPad && gst_pad_peer_query(sinkPad.get(), query.get())) {
        gst_query_parse_latency(query.get(), &live, nullptr, nullptr);
    }
    return live;
}

/// Has @p decoder give out each frame as soon as it can, as it does by itself behind a live source. Each property is
/// set where the decoder has it and takes the value.
void decodeWithoutDelay(GstElement* decoder)
{
    GObject* const object = G_OBJECT(decoder);
    // libav: every frame thread holds a frame back.
    gst_util_set_object_arg(object, "thread-type", "slice");
    // nvcodec: frames held back to pipeline decoding with display.
    gst_util_set_object_arg(object, "max-display-delay", "0");
    // The stateless H.264 decoders (va, d3d11, d3d12, v4l2sl, nv; GStreamer 1.20): a frame leaves the picture buffer
    // once its place in the output order is known, not when the buffer is full.
    gst_util_set_object_arg(object, "compliance", "normal");
}

bool isInside(GstObject* object, GstElement* element)
{
    return element && object && gst_object_has_as_ancestor(object, GST_OBJECT_CAST(element));
}

struct SinkFrameCounts
{
    quint64 displayed = 0;
    quint64 superseded = 0;
    quint64 gpuFallbacks = 0;
};

/// The frames a sink displayed, superseded and fell back to the CPU for, between its "frame-stats" @p since and
/// @p stats.
SinkFrameCounts sinkFrameCountsSince(const GstStructure* stats, const GstStructure* since)
{
    const auto growth = [stats, since](const char* field) { return GstQgc::frameStatsGrowth(stats, since, field); };
    quint64 delivered = 0;
    for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
        delivered += growth(GstQgc::sinkFramesField(HwVideoBufferPath(i)));
    }
    const quint64 superseded = growth(GstQgc::kSupersededFramesField);
    return {.displayed = (delivered > superseded) ? (delivered - superseded) : 0,
            .superseded = superseded,
            .gpuFallbacks = growth(GstQgc::kGpuFallbacksField)};
}

}  // namespace

DecodingBranch::DecodingBranch(const BranchContext& context, Listener& listener, bool lowLatency)
    : _context(context)
    , _listener(listener)
    , _lowLatency(lowLatency)
{}

DecodingBranch::~DecodingBranch()
{
    reset();
}

bool DecodingBranch::build()
{
    if (!_head.build(_context.pipeline, _context.tee, GStreamer::PipelineBranch::Decoding, "display")) {
        return false;
    }
    GstElement* const queue = _head.queue();

    // Tiny and leaky: the live display must drop its oldest frame on backpressure, never stall the tee. The
    // backpressure comes late, though: ahead of its decoder, decodebin3's multiqueue first buffers the stream, by
    // time and with no limit on buffers. It holds its min-interleave-time: 250 ms by default, the decoder's latency
    // plus 100 ms once the decoder reports one, and no less than the pipeline's latency (GStreamer 1.24, and the
    // later 1.22 releases). Only a decoder that falls further behind than that makes this queue leak.
    gst_util_set_object_arg(G_OBJECT(queue), "leaky", "downstream");
    g_object_set(queue, "max-size-buffers", kDisplayQueueMaxBuffers, "max-size-bytes", 0, "max-size-time",
                 G_GUINT64_CONSTANT(0), nullptr);

    // "overrun" announces a leak; the gate's probe sees whether one followed.
    _head.onOverrun(G_CALLBACK(_onQueueOverrun), this);
    const GStreamer::GstPadPtr queueSrc = GStreamer::adoptPad(gst_element_get_static_pad(queue, "src"));
    if (!queueSrc || !_keyframeGate.add(queueSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, _gateProbe, this)) {
        qCCritical(GstVideoReceiverLog) << "Failed to add the display keyframe gate";
        return false;
    }
    return true;
}

void DecodingBranch::reset()
{
    _keyframeGate.remove();
    _removeDecoder();
    _sinkFrameProbe.remove();
    _sink.reset();
    _sinkStatsAtStart.reset();
    _setDecoding(false);
    _head.reset();
}

void DecodingBranch::setVideoSink(GstElement* sink)
{
    if (sink == _sink.get()) {
        return;
    }

    _removeDecoder();
    _sinkFrameProbe.remove();
    _retireSinkCounts();
    _sink.reset();
    _sinkStatsAtStart.reset();
    _setDecoding(false);

    if (!sink) {
        return;
    }

    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(sink, "sink"));
    if (!sinkPad) {
        qCCritical(GstVideoReceiverLog) << "The video sink has no sink pad" << _context.redactedUri;
        return;
    }
    _sink = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(sink)));
    _sinkStatsAtStart = GstQgc::frameStats(_sink.get());
    for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
        _pathFramesAtSample[i] =
            GstQgc::frameStatsField(_sinkStatsAtStart.get(), GstQgc::sinkFramesField(HwVideoBufferPath(i)));
    }
    _gpuCountersAtStart = GstHwPathTelemetry::snapshot();
    GStreamer::markBranch(_sink.get(), GStreamer::PipelineBranch::Decoding);
    (void) _sinkFrameProbe.add(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, _sinkProbe, this);

    if (_streaming) {
        _insertDecoder();
    }
}

void DecodingBranch::onSourceStreaming()
{
    _streaming = true;
    if (_sink && !_decoder) {
        _insertDecoder();
    }
}

void DecodingBranch::restartDecoder(const char* reason)
{
    if (!_decoder) {
        return;
    }

    if (++_restartsWithoutFrame > kMaxDecoderRestarts) {
        qCDebug(GstVideoReceiverLog) << "Decoder failing after" << kMaxDecoderRestarts << "restarts, last:" << reason
                                     << _context.redactedUri;
        _context.post([this]() { _listener.onDecoderFailing(); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Restarting the decoder after" << reason << _context.redactedUri;
    ++_decoderRestarts;
    // The restart before this one brought no frame: with the same ranks decodebin3 would plug the same decoder again.
    if (_restartsWithoutFrame > 1) {
        _demoteFailedDecoder();
    }
    _removeDecoder();
    _setDecoding(false);
    _insertDecoder();
}

void DecodingBranch::onGraphicsDeviceServed()
{
    if (!_decoder || !hasHardwareDecoder(_decoder.get())) {
        return;
    }
    qCDebug(GstVideoReceiverLog) << "Replacing the decoder" << _decoderName
                                 << "by one on the graphics device Qt renders with" << _context.redactedUri;
    _removeDecoder();
    _setDecoding(false);
    _insertDecoder();
}

bool DecodingBranch::isCurrent(GstObject* source) const
{
    return _head.contains(source) || isInside(source, _decoder.get()) || (_decoder && isInside(source, _sink.get()));
}

GstQgc::PathStats DecodingBranch::sinkPathStats() const
{
    const GStreamer::GstStructurePtr stats = GstQgc::frameStats(_sink.get());
    GstQgc::PathStats out = GstQgc::formatSinkStats(stats.get(), _sinkStatsAtStart.get());
    if (stats) {
        out.line += HwBuffers::extraPathStatsSince(_gpuCountersAtStart);
    }
    return out;
}

DecodingBranch::StatsSample DecodingBranch::sampleStats()
{
    const GStreamer::GstStructurePtr stats = GstQgc::frameStats(_sink.get());
    const SinkFrameCounts sinkCounts = sinkFrameCountsSince(stats.get(), _sinkStatsAtStart.get());

    quint64 busiestPathFrames = 0;
    for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
        const auto path = HwVideoBufferPath(i);
        const quint64 frames = GstQgc::frameStatsField(stats.get(), GstQgc::sinkFramesField(path));
        const quint64 sinceSample = (frames > _pathFramesAtSample[i]) ? (frames - _pathFramesAtSample[i]) : 0;
        if (sinceSample > busiestPathFrames) {
            busiestPathFrames = sinceSample;
            _displayPath = QString::fromLatin1(GstHw::toString(path));
        }
        _pathFramesAtSample[i] = frames;
    }
    _countLateFrames();

    return {
        .framesDisplayed = _retiredFramesDisplayed + sinkCounts.displayed,
        .framesDropped = _framesDropped.load(std::memory_order_relaxed) + _retiredFramesSuperseded +
                         sinkCounts.superseded + _lateFramesDropped,
        .displayPath = _displayPath,
        .gpuFallbacks = _retiredGpuFallbacks + sinkCounts.gpuFallbacks,
        .decoderRestarts = _decoderRestarts,
    };
}

void DecodingBranch::_retireSinkCounts()
{
    if (!_sink) {
        return;
    }
    const GStreamer::GstStructurePtr stats = GstQgc::frameStats(_sink.get());
    const SinkFrameCounts counts = sinkFrameCountsSince(stats.get(), _sinkStatsAtStart.get());
    _retiredFramesDisplayed += counts.displayed;
    _retiredFramesSuperseded += counts.superseded;
    _retiredGpuFallbacks += counts.gpuFallbacks;
}

void DecodingBranch::_countLateFrames()
{
    const quint64 dropped = sinkLateFramesDropped(_sink.get());
    // A count below the last one started over.
    _lateFramesDropped += (dropped >= _sinkLateFramesSeen) ? (dropped - _sinkLateFramesSeen) : dropped;
    _sinkLateFramesSeen = dropped;
}

void DecodingBranch::_insertDecoder()
{
    if (_tryInsertDecoder()) {
        return;
    }
    // Without a decoder no frame is due at the sink, so neither the watchdog nor a bus error would ever report this.
    _context.post([this]() { _listener.onDecoderFailing(); });
}

bool DecodingBranch::_tryInsertDecoder()
{
    if (!gst_bin_add(GST_BIN(_context.pipeline), _sink.get())) {
        qCWarning(GstVideoReceiverLog) << "Failed to add the video sink" << _context.redactedUri;
        return false;
    }
    // PAUSED (not READY) negotiates downstream caps before the decoder pushes; PLAYING once linked.
    (void) gst_element_set_state(_sink.get(), GST_STATE_PAUSED);

    _decoder = GStreamer::makeElement("decodebin3");
    if (!_decoder) {
        qCWarning(GstVideoReceiverLog) << "gst_element_factory_make('decodebin3') failed";
        GStreamer::disposeChild(_sink.get());
        return false;
    }
    GStreamer::markBranch(_decoder.get(), GStreamer::PipelineBranch::Decoding);
    TeeBranchHead::absorbFlowErrors(_decoder.get());
    (void) _decoderElementAdded.connect(_decoder.get(), "deep-element-added", G_CALLBACK(_onDecoderElementAdded), this);
    (void) _decoderPadAdded.connect(_decoder.get(), "pad-added", G_CALLBACK(_onDecoderPadAdded), this);
    (void) g_signal_connect(_decoder.get(), "select-stream", G_CALLBACK(decodeVideoOnly), nullptr);

    if (!gst_bin_add(GST_BIN(_context.pipeline), _decoder.get()) ||
        !gst_element_sync_state_with_parent(_decoder.get()) || !gst_element_link(_head.valve(), _decoder.get())) {
        qCWarning(GstVideoReceiverLog) << "Unable to insert the decoder" << _context.redactedUri;
        _removeDecoder();
        return false;
    }
    // The clock paces frames stamped as they arrived. Behind a source that isn't live (tcp://) they are stamped from
    // zero, while the pipeline's time has run since it started: each would be late for the clock, and dropped.
    setSinkSync(_sink.get(), !_lowLatency && fedByLiveSource(_decoder.get()));
    _context.dotDumper->dump(_context.pipeline, "pipeline-with-decoder");

    (void) _insertions.fetch_add(1, std::memory_order_relaxed);
    _context.watchdog->startSinkClock(StreamWatchdog::now());
    _gateClosed.store(false, std::memory_order_relaxed);
    // Without a keyframe the new decoder has nothing to start from until the stream's next one.
    _head.open();

    qCDebug(GstVideoReceiverLog) << "Decoder inserted" << _context.redactedUri;
    return true;
}

void DecodingBranch::_removeDecoder()
{
    _head.close();
    // The sink first: going NULL releases a decoder thread waiting inside it, so the decoder can stop.
    if (_sink) {
        // Leaving PLAYING starts the sink's count of late frames over.
        _countLateFrames();
        GStreamer::disposeChild(_sink.get());
        _sinkLateFramesSeen = 0;
    }
    if (_decoder) {
        _decoderPadAdded.disconnect();
        _decoderElementAdded.disconnect();
        GStreamer::disposeChild(_decoder.get());
        _decoder.reset();
        _pluggedDecoder.reset();
        _hardwareDecoder = false;
        _setDecoderName(QString());
        _context.dotDumper->dump(_context.pipeline, "pipeline-decoding-stopped");
    }
}

void DecodingBranch::_linkDecoderPad(GstPad* pad)
{
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(_sink.get(), "sink"));
    const GstPadLinkReturn linkResult = sinkPad ? gst_pad_link(pad, sinkPad.get()) : GST_PAD_LINK_WRONG_HIERARCHY;
    if (linkResult != GST_PAD_LINK_OK) {
        qCWarning(GstVideoReceiverLog) << "Unable to link the decoder to the video sink:"
                                       << gst_pad_link_get_name(linkResult);
        restartDecoder("link failure");
        return;
    }
    (void) gst_element_sync_state_with_parent(_sink.get());
    // The sink posted LATENCY when it was added, with nothing linked to it yet, so that recalculation failed. Without
    // this one it would take its first frames for late ones, until another element posted LATENCY.
    (void) gst_bin_recalculate_latency(GST_BIN(_context.pipeline));
    _context.dotDumper->dump(_context.pipeline, "pipeline-with-videosink");
}

void DecodingBranch::_demoteFailedDecoder()
{
    if (!_hardwareDecoder || !_pluggedDecoder) {
        return;
    }
    // Every decoder fails on a device that was removed, and none is at fault.
    if (HwBuffers::graphicsDeviceLost()) {
        return;
    }
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(_pluggedDecoder.get(), "sink"));
    const GStreamer::GstCapsPtr caps =
        GStreamer::adoptCaps(sinkPad ? gst_pad_get_current_caps(sinkPad.get()) : nullptr);
    if (caps && GStreamer::demoteFailedDecoder(_decoderName.toUtf8().constData(), caps.get())) {
        qCWarning(GstVideoReceiverLog) << "The hardware decoder" << _decoderName
                                       << "brought no frame after a restart: the next decoder takes over"
                                       << _context.redactedUri;
    }
}

void DecodingBranch::_onFirstFrame(quint64 insertion)
{
    if (!_decoder || (insertion != _insertions.load(std::memory_order_relaxed))) {
        return;
    }
    _restartsWithoutFrame = 0;
    _setDecoding(true);
}

void DecodingBranch::_setDecoding(bool decoding)
{
    if (decoding == _decoding) {
        return;
    }
    _decoding = decoding;
    qCDebug(GstVideoReceiverLog) << (decoding ? "Decoding started" : "Decoding stopped") << _context.redactedUri;
    _listener.onDecodingChanged(decoding);
}

void DecodingBranch::_setDecoderName(const QString& name)
{
    if (name == _decoderName) {
        return;
    }
    _decoderName = name;
    _listener.onDecoderNameChanged(name);
}

void DecodingBranch::_onQueueOverrun(GstElement* /*queue*/, gpointer data)
{
    auto* const self = static_cast<DecodingBranch*>(data);
    // No frame is lost yet: the queue looks again once this returns, and leaks its oldest buffer only if it is still
    // full. The decoder may have taken one meanwhile.
    (void) self->_overruns.fetch_add(1, std::memory_order_relaxed);
}

GstPadProbeReturn DecodingBranch::_gateProbe(GstPad* /*pad*/, GstPadProbeInfo* info, gpointer data)
{
    auto* const self = static_cast<DecodingBranch*>(data);
    const GstBuffer* const buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    // The queue marks the buffer that follows a leak. Each overrun before it leaked one buffer, as the queue fills by
    // buffer count only. An overrun before a buffer without the mark leaked nothing, and is forgotten: it would be
    // taken for the leak behind a mark that upstream sets later, for a loss ahead of the tee.
    if (buffer && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT)) {
        const quint64 leaked = self->_overruns.exchange(0, std::memory_order_relaxed);
        (void) self->_framesDropped.fetch_add(leaked, std::memory_order_relaxed);
        if ((leaked > 0) && !self->_gateClosed.exchange(true, std::memory_order_relaxed)) {
            self->_gateClosedAtMs.store(StreamWatchdog::now(), std::memory_order_relaxed);
            self->_context.post([self]() { self->_head.requestKeyframe(); });
        }
    } else {
        self->_overruns.store(0, std::memory_order_relaxed);
    }
    if (!self->_gateClosed.load(std::memory_order_relaxed)) {
        return GST_PAD_PROBE_OK;
    }
    if (buffer && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
        const qint64 now = StreamWatchdog::now();
        if ((now - self->_gateClosedAtMs.load(std::memory_order_relaxed)) < kMaxGateMs) {
            (void) self->_framesDropped.fetch_add(1, std::memory_order_relaxed);
            // The gate's time is not the decoder's: with the shortest stream timeout the decoder counts as stalled
            // after as long as the gate may stay closed.
            self->_context.watchdog->noteFrameWithheld(now);
            return GST_PAD_PROBE_DROP;
        }
        // Told once at a level that shows: the same holds for every later request to this stream's sender.
        if (!self->_gateTimeoutWarned.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstVideoReceiverLog) << "No keyframe within" << kMaxGateMs
                                           << "ms of the request that followed a dropped frame: the picture goes on "
                                              "without one. The stream's sender may not take keyframe requests."
                                           << self->_context.redactedUri;
        } else {
            qCDebug(GstVideoReceiverLog) << "No keyframe within" << kMaxGateMs << "ms of the request"
                                         << self->_context.redactedUri;
        }
    }
    self->_gateClosed.store(false, std::memory_order_relaxed);
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn DecodingBranch::_sinkProbe(GstPad* /*pad*/, GstPadProbeInfo* /*info*/, gpointer data)
{
    auto* const self = static_cast<DecodingBranch*>(data);
    if (self->_context.watchdog->noteSinkFrame(StreamWatchdog::now())) {
        const quint64 insertion = self->_insertions.load(std::memory_order_relaxed);
        self->_context.post([self, insertion]() { self->_onFirstFrame(insertion); });
    }
    return GST_PAD_PROBE_OK;
}

void DecodingBranch::_onDecoderPadAdded(GstElement* decoder, GstPad* pad, gpointer data)
{
    auto* const self = static_cast<DecodingBranch*>(data);

    // Runs on a streaming thread; the pipeline is only changed on the worker.
    self->_context.post(
        GStreamer::holdPadUntilLinked(pad, [self, padDecoder = GStreamer::shareObject(decoder)](GstPad* newPad) {
            if (padDecoder.get() == self->_decoder.get()) {
                self->_linkDecoderPad(newPad);
            }
        }));
}

void DecodingBranch::_onDecoderElementAdded(GstBin* decodebin, GstBin* /*subBin*/, GstElement* element, gpointer data)
{
    // A message can outlive the decoder that held its source, which leaves nothing to blame it on but its own mark.
    GStreamer::markBranch(element, GStreamer::PipelineBranch::Decoding);

    GstElementFactory* factory = gst_element_get_factory(element);
    if (!factory || !gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER)) {
        return;
    }

    // Live streams: once a frame is late, a QoS event has the videodecoder base class drop the frames due before the
    // time it names. Turned off here, before the decoder sees data, for each decoder decodebin3 plugs, later swaps
    // included. That stops the base class only: avdec and amcvideodec act on the event's deadline themselves, and it
    // is the sink bin, which keeps its sink's QoS off, that sends them none.
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(element), "qos")) {
        g_object_set(element, "qos", FALSE, nullptr);
    }
    // A decoder takes a source that isn't live (tcp://) for a file, and trades delay for throughput.
    const bool live = fedByLiveSource(GST_ELEMENT(decodebin));
    if (!live) {
        decodeWithoutDelay(element);
    }
    const bool singleThread = GStreamer::avoidHevcWavefrontDeadlock(element);

    qCDebug(GstVideoReceiverLog).noquote()
        << "Decodebin3 selected" << GStreamer::describeDecoderFactory(factory) << "- QoS disabled"
        << (live ? "" : "- set to decode without delay, as the source isn't live")
        << (singleThread ? "- on one thread, as this FFmpeg's HEVC slice threads can deadlock" : "");
    const QString name = QString::fromUtf8(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
    const bool hardware = GStreamer::isHardwareDecoderFactory(factory);

    auto* const self = static_cast<DecodingBranch*>(data);
    self->_context.post([self, name, hardware, decoder = GStreamer::shareObject(GST_ELEMENT(decodebin)),
                         plugged = GStreamer::shareObject(element)]() {
        // A decodebin3 taken out meanwhile has no decoder left to name.
        if (decoder.get() != self->_decoder.get()) {
            return;
        }
        self->_hardwareDecoder = hardware;
        self->_pluggedDecoder = plugged;
        self->_setDecoderName(name);
    });
}
