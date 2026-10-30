#include "DecodingBranch.h"

#include <gst/gst.h>

#include "GStreamerHelpers.h"
#include "HwBuffers/common/HwBuffers.h"
#include "PipelineDotDumper.h"
#include "StreamWatchdog.h"
#include "gstqgc/gstqgcqvideosink.h"

namespace {

constexpr guint kDisplayQueueMaxBuffers = 2;
// A stream without keyframes (intra refresh) would otherwise freeze for good after a dropped frame.
constexpr int kMaxGateDrops = 300;

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

bool isInside(GstObject* object, GstElement* element)
{
    return element && object && gst_object_has_as_ancestor(object, GST_OBJECT_CAST(element));
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
    const GStreamer::TeeBranch branch = GStreamer::addTeeBranch(_context.pipeline, _context.tee, "display");
    if (!branch.valve) {
        return false;
    }
    _queue = branch.queue;
    _valve = branch.valve;
    GStreamer::markBranch(_queue, PipelineBranch::Decoding);
    GStreamer::markBranch(_valve, PipelineBranch::Decoding);

    // Tiny and leaky: the live display must drop its oldest frame on backpressure, never stall the tee.
    gst_util_set_object_arg(G_OBJECT(_queue), "leaky", "downstream");
    g_object_set(_queue, "max-size-buffers", kDisplayQueueMaxBuffers, "max-size-bytes", 0, "max-size-time",
                 G_GUINT64_CONSTANT(0), nullptr);

    // "overrun" fires before the queue drops its oldest frame.
    (void) _queueOverrun.connect(_queue, "overrun", G_CALLBACK(_onQueueOverrun), this);
    const GStreamer::GstPadPtr queueSrc = GStreamer::adoptPad(gst_element_get_static_pad(_queue, "src"));
    if (!queueSrc || !_keyframeGate.add(queueSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, _gateProbe, this)) {
        qCCritical(GstVideoReceiverLog) << "Failed to add the display keyframe gate";
        return false;
    }
    return true;
}

void DecodingBranch::reset()
{
    _queueOverrun.disconnect();
    _keyframeGate.remove();
    _removeDecoder();
    _sinkFrameProbe.remove();
    _sink.reset();
    _sinkStatsAtStart.reset();
    _setDecoding(false);
    _queue = nullptr;
    _valve = nullptr;
}

void DecodingBranch::setVideoSink(GstElement* sink)
{
    if (sink == _sink.get()) {
        return;
    }

    _removeDecoder();
    _sinkFrameProbe.remove();
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
    _gpuCountersAtStart = GstHwPathTelemetry::snapshot();
    GStreamer::markBranch(_sink.get(), PipelineBranch::Decoding);
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
    _removeDecoder();
    _setDecoding(false);
    _insertDecoder();
}

bool DecodingBranch::isCurrent(GstObject* source) const
{
    return (source == GST_OBJECT_CAST(_queue)) || (source == GST_OBJECT_CAST(_valve)) ||
           isInside(source, _decoder.get()) || (_decoder && isInside(source, _sink.get()));
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

void DecodingBranch::_insertDecoder()
{
    setSinkSync(_sink.get(), !_lowLatency);
    if (!gst_bin_add(GST_BIN(_context.pipeline), _sink.get())) {
        qCWarning(GstVideoReceiverLog) << "Failed to add the video sink" << _context.redactedUri;
        return;
    }
    // PAUSED (not READY) negotiates downstream caps before the decoder pushes; PLAYING once linked.
    (void) gst_element_set_state(_sink.get(), GST_STATE_PAUSED);

    _decoder = GStreamer::makeElement("decodebin3");
    if (!_decoder) {
        qCWarning(GstVideoReceiverLog) << "gst_element_factory_make('decodebin3') failed";
        GStreamer::disposeChild(_sink.get());
        return;
    }
    GStreamer::markBranch(_decoder.get(), PipelineBranch::Decoding);
    (void) _decoderElementAdded.connect(_decoder.get(), "deep-element-added", G_CALLBACK(_onDecoderElementAdded), this);
    (void) _decoderPadAdded.connect(_decoder.get(), "pad-added", G_CALLBACK(_onDecoderPadAdded), this);

    if (!gst_bin_add(GST_BIN(_context.pipeline), _decoder.get()) ||
        !gst_element_sync_state_with_parent(_decoder.get()) || !gst_element_link(_valve, _decoder.get())) {
        qCWarning(GstVideoReceiverLog) << "Unable to insert the decoder" << _context.redactedUri;
        _removeDecoder();
        return;
    }
    _context.dotDumper->dump(_context.pipeline, "pipeline-with-decoder");

    (void) _insertions.fetch_add(1, std::memory_order_relaxed);
    _context.watchdog->startSinkClock(StreamWatchdog::now());
    _gateClosed.store(false, std::memory_order_relaxed);
    g_object_set(_valve, "drop", FALSE, nullptr);

    // Without a keyframe the new decoder has nothing to start from until the stream's next one.
    const GStreamer::GstPadPtr valveSink = GStreamer::adoptPad(gst_element_get_static_pad(_valve, "sink"));
    GStreamer::requestKeyframe(valveSink.get());

    qCDebug(GstVideoReceiverLog) << "Decoder inserted" << _context.redactedUri;
}

void DecodingBranch::_removeDecoder()
{
    if (_valve) {
        g_object_set(_valve, "drop", TRUE, nullptr);
    }
    // The sink first: going NULL releases a decoder thread waiting inside it, so the decoder can stop.
    if (_sink) {
        GStreamer::disposeChild(_sink.get());
    }
    if (_decoder) {
        _decoderPadAdded.disconnect();
        _decoderElementAdded.disconnect();
        GStreamer::disposeChild(_decoder.get());
        _decoder.reset();
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
    _context.dotDumper->dump(_context.pipeline, "pipeline-with-videosink");
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
    if (self->_gateClosed.exchange(true, std::memory_order_relaxed)) {
        return;
    }
    self->_gateDrops.store(0, std::memory_order_relaxed);
    self->_context.post([self]() {
        if (self->_valve) {
            const GStreamer::GstPadPtr valveSink =
                GStreamer::adoptPad(gst_element_get_static_pad(self->_valve, "sink"));
            GStreamer::requestKeyframe(valveSink.get());
        }
    });
}

GstPadProbeReturn DecodingBranch::_gateProbe(GstPad* /*pad*/, GstPadProbeInfo* info, gpointer data)
{
    auto* const self = static_cast<DecodingBranch*>(data);
    if (!self->_gateClosed.load(std::memory_order_relaxed)) {
        return GST_PAD_PROBE_OK;
    }
    const GstBuffer* const buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    const bool delta = buffer && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    if (delta && (self->_gateDrops.fetch_add(1, std::memory_order_relaxed) < kMaxGateDrops)) {
        return GST_PAD_PROBE_DROP;
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

void DecodingBranch::_onDecoderElementAdded(GstBin* /*decodebin*/, GstBin* /*subBin*/, GstElement* element,
                                            gpointer data)
{
    // A message can outlive the decoder that held its source, which leaves nothing to blame it on but its own mark.
    GStreamer::markBranch(element, PipelineBranch::Decoding);

    GstElementFactory* factory = gst_element_get_factory(element);
    if (!factory || !gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER)) {
        return;
    }

    // Live streams: once a frame is late, the videodecoder base class advances earliest_time and drops every frame
    // after it. Turned off here, before the decoder sees data, for each decoder decodebin3 plugs, later swaps included.
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(element), "qos")) {
        g_object_set(element, "qos", FALSE, nullptr);
    }

    qCDebug(GstVideoReceiverLog).noquote()
        << "Decodebin3 selected" << GStreamer::describeDecoderFactory(factory) << "- QoS disabled";
    const QString name = QString::fromUtf8(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));

    auto* const self = static_cast<DecodingBranch*>(data);
    self->_context.post([self, name]() { self->_setDecoderName(name); });
}
