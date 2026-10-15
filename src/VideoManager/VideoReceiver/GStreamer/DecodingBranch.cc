#include "DecodingBranch.h"

#include <gst/gst.h>

#include <QtCore/QMutexLocker>
#include <QtCore/QSize>

#include "GStreamerHelpers.h"
#include "GstVideoReceiver.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCQVideoSinkController.h"
#include "gstqgc/gstqgcvideosinkbin.h"

namespace {

constexpr guint kDisplayQueueMaxBuffers = 2;

}  // namespace

DecodingBranch::DecodingBranch(GstVideoReceiver& receiver)
    : _receiver(receiver)
{}

bool DecodingBranch::build(GstElement* pipeline, GstElement* tee)
{
    _pipeline = pipeline;

    GstElement* queue = gst_element_factory_make("queue", nullptr);
    if (!queue) {
        qCWarning(GstVideoReceiverLog) << "gst_element_factory_make('queue') failed";
        return false;
    }

    // Leaky downstream + tiny depth: the live-display branch must drop the oldest
    // buffer on backpressure, not stall the streaming thread. The recording branch
    // keeps default non-leaky semantics so every frame reaches the muxer.
    gst_util_set_object_arg(G_OBJECT(queue), "leaky", "downstream");
    g_object_set(queue, "max-size-buffers", kDisplayQueueMaxBuffers, "max-size-bytes", 0, "max-size-time",
                 G_GUINT64_CONSTANT(0), nullptr);
    (void) gst_bin_add(GST_BIN(_pipeline), queue);

    _valve = gst_element_factory_make("valve", nullptr);
    if (!_valve) {
        qCWarning(GstVideoReceiverLog) << "gst_element_factory_make('valve') failed";
        return false;
    }

    g_object_set(_valve, "drop", TRUE, nullptr);
    (void) gst_bin_add(GST_BIN(_pipeline), _valve);

    if (!gst_element_link_many(tee, queue, _valve, nullptr)) {
        qCWarning(GstVideoReceiverLog) << "Unable to link decoder queue";
        return false;
    }
    return true;
}

void DecodingBranch::reset()
{
    // The video sink outlives the pipeline (owned by the caller of startDecoding), so its probe
    // and pipeline membership must be dropped even when start() failed before decoding began.
    if (_decoder || _videoSink) {
        _shutdown();
    }
    // Owned by the pipeline.
    _valve = nullptr;
    _pipeline = nullptr;
}

void DecodingBranch::start(GstElement* videoSink, bool hasWidget, bool streaming)
{
    qCDebug(GstVideoReceiverLog) << "Starting decoding" << _receiver._redactedUri();

    if (!hasWidget) {
        qCDebug(GstVideoReceiverLog) << "Video Widget is NULL" << _receiver._redactedUri();
        emit _receiver.onStartDecodingComplete(VideoReceiver::STATUS_FAIL);
        return;
    }

    if (!_pipeline && _videoSink) {
        // Replace a sink stored before start(); also drops its frame probe.
        _shutdown();
    }

    if (_videoSink) {
        qCDebug(GstVideoReceiverLog) << "Already decoding!" << _receiver._redactedUri();
        emit _receiver.onStartDecodingComplete(VideoReceiver::STATUS_INVALID_STATE);
        return;
    }

    const GStreamer::GstPadPtr pad = GStreamer::adoptPad(gst_element_get_static_pad(videoSink, "sink"));
    if (!pad) {
        qCCritical(GstVideoReceiverLog) << "Unable to find sink pad of video sink" << _receiver._redactedUri();
        emit _receiver.onStartDecodingComplete(VideoReceiver::STATUS_FAIL);
        return;
    }

    _receiver._watchdog.clearSinkFrames();

    _sinkProbeId = gst_pad_add_probe(pad.get(), GST_PAD_PROBE_TYPE_BUFFER, _sinkProbe, this, nullptr);

    _videoSink = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(videoSink)));
    _sinkStatsAtStart = _readSinkStats();

    if (!streaming) {
        emit _receiver.onStartDecodingComplete(VideoReceiver::STATUS_OK);
        return;
    }

    emit _receiver.onStartDecodingComplete(_insertDecoder() ? VideoReceiver::STATUS_OK : VideoReceiver::STATUS_FAIL);
}

void DecodingBranch::stop()
{
    qCDebug(GstVideoReceiverLog) << "Stopping decoding" << _receiver._redactedUri();

    // Gate on _videoSink (set by startDecoding) instead of _decoding (which only flips on the first frame),
    // so a stop before the first frame still removes the branch.
    if (!_videoSink) {
        qCDebug(GstVideoReceiverLog) << "Not decoding!" << _receiver._redactedUri();
        emit _receiver.onStopDecodingComplete(VideoReceiver::STATUS_INVALID_STATE);
        return;
    }

    // The display branch has nothing to flush, so it is torn down at once instead of draining an EOS.
    if (_valve) {
        g_object_set(_valve, "drop", TRUE, nullptr);
    }
    _shutdown();

    emit _receiver.onStopDecodingComplete(VideoReceiver::STATUS_OK);
}

void DecodingBranch::onSourceStreaming()
{
    if (!_videoSink) {
        return;
    }

    _receiver._dotDumper.dump(_pipeline, "pipeline-with-new-source-pad");

    (void) _insertDecoder();
}

void DecodingBranch::onDecoderPad(GstPad* pad)
{
    qCDebug(GstVideoReceiverLog) << "New decoder pad" << _receiver._redactedUri();

    _receiver._dotDumper.dump(_pipeline, "pipeline-with-new-decoder-pad");

    (void) _addVideoSink(pad);
}

void DecodingBranch::onBusCapsInfo(GstMessage* message)
{
    const GstStructure* structure = gst_message_get_structure(message);
    gint w = 0, h = 0;
    const gchar* fmt = gst_structure_get_string(structure, "format");
    gst_structure_get_int(structure, "width", &w);
    gst_structure_get_int(structure, "height", &h);
    const QString format = QString::fromUtf8(fmt ? fmt : "");
    const QSize resolution(w, h);
    // src compared by address only on the GUI thread; never dereferenced (may be gone by then).
    void* src = GST_MESSAGE_SRC(message);
    QMetaObject::invokeMethod(
        &_receiver,
        [this, format, resolution, src]() {
            for (auto* c : QGCQVideoSinkController::controllersOf(&_receiver)) {
                if (static_cast<const void*>(c->element()) == src) {
                    c->updateNegotiation(format, resolution);
                }
            }
            // The sink's negotiated raw caps are always fixed, unlike the encoded caps upstream of the decoder.
            emit _receiver.videoSizeChanged(resolution);
        },
        Qt::QueuedConnection);
}

QString DecodingBranch::decoderName() const
{
    QMutexLocker locker(&_decoderNameMutex);
    return _decoderName;
}

HwBuffers::PathStats DecodingBranch::sinkPathStats() const
{
    const GStreamer::GstStructurePtr stats = _readSinkStats();
    return HwBuffers::formatSinkStats(stats.get(), _sinkStatsAtStart.get());
}

GStreamer::GstStructurePtr DecodingBranch::_readSinkStats() const
{
    if (!_videoSink || !GST_IS_QGC_VIDEO_SINK_BIN(_videoSink.get())) {
        return {};
    }
    const GStreamer::GstElementPtr qvideosink =
        GStreamer::adoptElement(gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(_videoSink.get())));
    if (!qvideosink) {
        return {};
    }
    GstStructure* stats = nullptr;
    g_object_get(qvideosink.get(), "frame-stats", &stats, nullptr);
    return GStreamer::adoptStructure(stats);
}

bool DecodingBranch::_insertDecoder()
{
    _ensureVideoSinkInPipeline();

    if (!_addDecoder()) {
        qCWarning(GstVideoReceiverLog) << "Failed to add the decoder" << _receiver._redactedUri();
        _shutdown();
        return false;
    }

    g_object_set(_valve, "drop", FALSE, nullptr);

    qCDebug(GstVideoReceiverLog) << "Decoding started" << _receiver._redactedUri();
    return true;
}

bool DecodingBranch::_addDecoder()
{
    _decoder = GStreamer::adoptElement(gst_element_factory_make("decodebin3", nullptr));
    if (!_decoder) {
        qCWarning(GstVideoReceiverLog) << "gst_element_factory_make('decodebin3') failed";
        return false;
    }
    // Ours alongside the pipeline's, so decoder() stays valid until _shutdown().
    (void) gst_object_ref_sink(_decoder.get());

    (void) g_signal_connect(_decoder.get(), "deep-element-added", G_CALLBACK(_onDecoderElementAdded), this);

    (void) gst_bin_add(GST_BIN(_pipeline), _decoder.get());
    (void) gst_element_sync_state_with_parent(_decoder.get());

    _receiver._dotDumper.dump(_pipeline, "pipeline-with-decoder");

    if (!gst_element_link(_valve, _decoder.get())) {
        qCWarning(GstVideoReceiverLog) << "Unable to link decoder";
        GStreamer::disposeChild(_decoder.get());
        _decoder.reset();
        return false;
    }

    if (const GStreamer::GstPadPtr srcPad = GStreamer::firstSrcPad(_decoder.get())) {
        onDecoderPad(srcPad.get());
    } else {
        (void) g_signal_connect(_decoder.get(), "pad-added", G_CALLBACK(GstVideoReceiver::_onNewPad), &_receiver);
    }

    return true;
}

void DecodingBranch::_ensureVideoSinkInPipeline()
{
    if (!_videoSink || !_pipeline) {
        return;
    }

    if (const GStreamer::GstObjectPtr parent =
            GStreamer::adoptObject(gst_object_get_parent(GST_OBJECT(_videoSink.get())))) {
        return;
    }

    g_object_set(_videoSink.get(), "sync", !_receiver._config.lowLatency, NULL);

    (void) gst_bin_add(GST_BIN(_pipeline), _videoSink.get());

    // PAUSED (not READY) triggers downstream caps negotiation before source data arrives.
    (void) gst_element_set_state(_videoSink.get(), GST_STATE_PAUSED);
}

bool DecodingBranch::_addVideoSink(GstPad* pad)
{
    _ensureVideoSinkInPipeline();

    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(_videoSink.get(), "sink"));
    const GstPadLinkReturn linkRet = sinkPad ? gst_pad_link(pad, sinkPad.get()) : GST_PAD_LINK_WRONG_HIERARCHY;
    if (linkRet != GST_PAD_LINK_OK) {
        qCWarning(GstVideoReceiverLog) << "Unable to link decoder pad to video sink:" << gst_pad_link_get_name(linkRet);

        // _ensureVideoSinkInPipeline() added it before linking; detach for the next retry.
        GStreamer::disposeChild(_videoSink.get());
        return false;
    }

    (void) gst_element_sync_state_with_parent(_videoSink.get());

    _receiver._dotDumper.dump(_pipeline, "pipeline-with-videosink");

    return true;
}

void DecodingBranch::_onFirstFrame()
{
    if (!_videoSink || _decoding) {
        return;
    }
    _decoding = true;
    qCDebug(GstVideoReceiverLog) << "Decoding started";
    emit _receiver.decodingChanged(true);
}

void DecodingBranch::_shutdown()
{
    if (_decoder) {
        GStreamer::disposeChild(_decoder.get());
        _decoder.reset();
    }

    if (_sinkProbeId != 0 && _videoSink) {
        if (const GStreamer::GstPadPtr sinkPad =
                GStreamer::adoptPad(gst_element_get_static_pad(_videoSink.get(), "sink"))) {
            gst_pad_remove_probe(sinkPad.get(), _sinkProbeId);
        }
    }
    _sinkProbeId = 0;

    _receiver._watchdog.clearSinkFrames();

    if (_videoSink) {
        GStreamer::disposeChild(_videoSink.get());
        _videoSink.reset();
        _releaseDisplayedFrame();
    }
    _sinkStatsAtStart.reset();

    if (_decoding.exchange(false)) {
        qCDebug(GstVideoReceiverLog) << "Decoding stopped";
        emit _receiver.decodingChanged(false);
    }

    _receiver._dotDumper.dump(_pipeline, "pipeline-decoding-stopped");
}

void DecodingBranch::_releaseDisplayedFrame()
{
    QMetaObject::invokeMethod(
        &_receiver,
        [this]() {
            for (auto* c : QGCQVideoSinkController::controllersOf(&_receiver)) {
                c->releaseFrame();
            }
        },
        Qt::QueuedConnection);
}

void DecodingBranch::_onDecoderElementAdded(GstBin* /*decodebin*/, GstBin* /*subBin*/, GstElement* element,
                                            gpointer data)
{
    GstElementFactory* factory = gst_element_get_factory(element);
    if (!factory || !gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER)) {
        return;
    }

    // Live streams: once a frame is late, the videodecoder base class advances earliest_time and drops every frame
    // after it. Turned off here, before the decoder sees data, for each decoder decodebin3 plugs, later swaps included.
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(element), "qos")) {
        g_object_set(element, "qos", FALSE, nullptr);
    }

    GstPluginFeature* feature = GST_PLUGIN_FEATURE(factory);
    const QString decoderName = QString::fromUtf8(gst_plugin_feature_get_name(feature));
    QString pluginName = decoderName;
    if (const GStreamer::GstObjectPtr plugin =
            GStreamer::adoptObject(GST_OBJECT(gst_plugin_feature_get_plugin(feature)))) {
        pluginName = QString::fromUtf8(gst_plugin_get_name(GST_PLUGIN(plugin.get())));
    }
    qCDebug(GstVideoReceiverLog) << "Decodebin3 selected codec:rank -" << pluginName << "/" << decoderName << "-"
                                 << gst_element_factory_get_klass(factory)
                                 << (GStreamer::isHardwareDecoderFactory(factory) ? "(HW)" : "(SW)") << ":"
                                 << gst_plugin_feature_get_rank(feature) << "- QoS disabled";

    auto* self = static_cast<DecodingBranch*>(data);
    self->_receiver._runOnWorkerForPipeline([self, decoderName]() {
        {
            QMutexLocker locker(&self->_decoderNameMutex);
            if (decoderName == self->_decoderName) {
                return;
            }
            self->_decoderName = decoderName;
        }
        (void) QMetaObject::invokeMethod(
            &self->_receiver, [self]() { emit self->_receiver.decoderNameChanged(); }, Qt::QueuedConnection);
    });
}

GstPadProbeReturn DecodingBranch::_sinkProbe(GstPad* /*pad*/, GstPadProbeInfo* /*info*/, gpointer user_data)
{
    auto* self = static_cast<DecodingBranch*>(user_data);
    if (self && self->_receiver._watchdog.noteSinkFrame(StreamWatchdog::now())) {
        self->_receiver._runOnWorkerForPipeline([self]() { self->_onFirstFrame(); });
    }
    return GST_PAD_PROBE_OK;
}
