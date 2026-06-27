//-----------------------------------------------------------------------------
// Our pipeline look like this:
//
//              +-->queue-->_decoderValve[-->_decoder-->_videoSink]
//              |
// _source-->_tee
//              |
//              +-->queue-->_recorderValve[-->_fileSink]
//-----------------------------------------------------------------------------

#include "GstVideoReceiver.h"

#include "HwBuffers/common/HwBuffers.h"

#include "GStreamerHelpers.h"
#include "GstSourceFactory.h"
#include "QGCLoggingCategory.h"
#include "QGCQVideoSinkController.h"

#include <QtCore/QDateTime>
#include <QtCore/QMutexLocker>
#include <QtCore/QUrl>
#include <QtQuick/QQuickItem>

#include <algorithm>

#include <gst/gst.h>
#include <gst/video/video.h>

QGC_LOGGING_CATEGORY(GstVideoReceiverLog, "Video.GStreamer.GstVideoReceiver")

namespace {
// kEosTimeoutNs: bus wait budget for EOS/ERROR during stop(); 3 s covers slow hw decoders.
constexpr GstClockTime kEosTimeoutNs = 3 * GST_SECOND;

// Refs the element's first src pad into *userData and stops iterating. Resync is handled
// internally by gst_element_foreach_src_pad (unlike a bare gst_iterator_next loop).
gboolean grabFirstSrcPad(GstElement * /*element*/, GstPad *pad, gpointer userData)
{
    *static_cast<GstPad **>(userData) = GST_PAD(gst_object_ref(pad));
    return FALSE;
}

bool isRecoverableH265PaciError(GstMessage *msg, const GError *error, const gchar *debug)
{
    if (!msg || !error || (error->domain != GST_STREAM_ERROR) || (error->code != GST_STREAM_ERROR_FORMAT) ||
        !debug || !g_strrstr(debug, "NAL unit type 50 not supported yet")) {
        return false;
    }

    GstObject *src = GST_MESSAGE_SRC(msg);
    if (!src || !GST_IS_ELEMENT(src)) {
        return false;
    }

    GstElementFactory *factory = gst_element_get_factory(GST_ELEMENT(src));
    return factory && (g_strcmp0(GST_OBJECT_NAME(factory), "rtph265depay") == 0);
}

} // namespace

GstVideoReceiver::GstVideoReceiver(QObject *parent)
    : VideoReceiver(parent)
    , _worker(new GstVideoWorker(this))
{
    qCDebug(GstVideoReceiverLog) << this;

    _worker->start();
    (void) connect(&_watchdogTimer, &QTimer::timeout, this, &GstVideoReceiver::_watchdog);
}

GstVideoReceiver::~GstVideoReceiver()
{
    stop();
    _worker->shutdown();

    qCDebug(GstVideoReceiverLog) << this;
}

void GstVideoReceiver::start(uint32_t timeout)
{
    if (_needDispatch()) {
        _worker->dispatch([this, timeout]() { start(timeout); });
        return;
    }

    if (_pipeline) {
        qCDebug(GstVideoReceiverLog) << "Already running!" << _uri;
        emit onStartComplete(STATUS_INVALID_STATE);
        return;
    }

    if (_uri.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Failed because URI is not specified";
        emit onStartComplete(STATUS_INVALID_URL);
        return;
    }

    _timeout = timeout;
    _buffer = lowLatency() ? -1 : 0;

    qCDebug(GstVideoReceiverLog) << "Starting" << _uri << ", lowLatency" << lowLatency() << ", timeout" << _timeout;

    // GST_DEBUG_BIN_TO_DOT_FILE is a no-op unless GST_DEBUG_DUMP_DOT_DIR is set; surface that
    // once per process so field debugging doesn't require re-reading the source.
    [[maybe_unused]] static const bool dotDirHinted = []() {
        if (qgetenv("GST_DEBUG_DUMP_DOT_DIR").isEmpty()) {
            qCInfo(GstVideoReceiverLog).noquote()
                << "Pipeline dot-graph dumps are disabled. Set GST_DEBUG_DUMP_DOT_DIR=/path/to/dir to enable.";
        }
        return true;
    }();

    _endOfStream = false;

    bool running = false;
    bool pipelineUp = false;

    GstElement *decoderQueue = nullptr;
    GstElement *recorderQueue = nullptr;

    do {
        _tee = gst_element_factory_make("tee", nullptr);
        if (!_tee)  {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('tee') failed";
            break;
        }

        GstPad *pad = gst_element_get_static_pad(_tee, "sink");
        if (!pad) {
            qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
            break;
        }

        _lastSourceFrameTime = 0;

        _teeProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, _teeProbe, this, nullptr);
        gst_clear_object(&pad);
        if (_teeProbeId == 0) {
            // _teeProbe updates _lastSourceFrameTime; without it the watchdog timer fires spuriously instead of reporting a real failure.
            qCCritical(GstVideoReceiverLog) << "gst_pad_add_probe(_teeProbe) failed";
            break;
        }

        decoderQueue = gst_element_factory_make("queue", nullptr);
        if (!decoderQueue)  {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('queue') failed";
            break;
        }

        // leaky=downstream (2) + tiny depth: the live-display branch must drop the oldest
        // buffer on backpressure, not stall the streaming thread. Recording branch (below)
        // keeps default non-leaky semantics so every frame reaches the muxer.
        g_object_set(decoderQueue,
                     "leaky", 2,
                     "max-size-buffers", 2,
                     "max-size-bytes", 0,
                     "max-size-time", G_GUINT64_CONSTANT(0),
                     nullptr);

        _decoderValve = gst_element_factory_make("valve", nullptr);
        if (!_decoderValve)  {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('valve') failed";
            break;
        }

        g_object_set(_decoderValve,
                     "drop", TRUE,
                     nullptr);

        recorderQueue = gst_element_factory_make("queue", nullptr);
        if (!recorderQueue)  {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('queue') failed";
            break;
        }

        _recorderValve = gst_element_factory_make("valve", nullptr);
        if (!_recorderValve) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('valve') failed";
            break;
        }

        g_object_set(_recorderValve,
                     "drop", TRUE,
                     nullptr);

        _pipeline = gst_pipeline_new("receiver");
        if (!_pipeline) {
            qCCritical(GstVideoReceiverLog) << "gst_pipeline_new() failed";
            break;
        }

        g_object_set(_pipeline,
                     "message-forward", TRUE,
                     nullptr);

        GStreamer::SourceFactory::Config sourceConfig;
        sourceConfig.jitterBuffer = (_buffer < 0)
            ? GStreamer::SourceFactory::JitterBuffer::None
            : (_buffer == 0
                ? GStreamer::SourceFactory::JitterBuffer::DropOnLatency
                : GStreamer::SourceFactory::JitterBuffer::Buffered);
        sourceConfig.latencyMs = _rtpJitterLatencyMs;
        // do-retransmission needs ≥40 ms latency headroom over the default 20 ms rtx-delay;
        // forcibly disable for sub-frame latency configurations to avoid retransmit storms.
        sourceConfig.doRetransmission = (_rtpJitterLatencyMs >= 40) && (sourceConfig.jitterBuffer != GStreamer::SourceFactory::JitterBuffer::None);
        _source = GStreamer::SourceFactory::create(_uri, sourceConfig);
        if (!_source) {
            qCCritical(GstVideoReceiverLog) << "SourceFactory::create() failed";
            break;
        }

        gst_bin_add_many(GST_BIN(_pipeline), _source, _tee, decoderQueue, _decoderValve, recorderQueue, _recorderValve, nullptr);

        pipelineUp = true;

        GstPad *srcPad = nullptr;
        (void) gst_element_foreach_src_pad(_source, grabFirstSrcPad, &srcPad);

        if (srcPad) {
            _onNewSourcePad(srcPad);
            gst_clear_object(&srcPad);
        } else {
            (void) g_signal_connect(_source, "pad-added", G_CALLBACK(_onNewPad), this);
        }

        if (!gst_element_link_many(_tee, decoderQueue, _decoderValve, nullptr)) {
            qCCritical(GstVideoReceiverLog) << "Unable to link decoder queue";
            break;
        }

        if (!gst_element_link_many(_tee, recorderQueue, _recorderValve, nullptr)) {
            qCCritical(GstVideoReceiverLog) << "Unable to link recorder queue";
            break;
        }

        GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
        if (bus) {
            gst_bus_enable_sync_message_emission(bus);
            (void) g_signal_connect(bus, "sync-message", G_CALLBACK(_onBusMessage), this);
            // HwBuffers facade chains every compiled context bridge so they don't clobber each
            // other via gst_bus_set_sync_handler. Must run before GST_STATE_PLAYING — upstream
            // queries context during PAUSED→PLAYING. No-op when no bridge-using GPU path is compiled.
            gst_bus_set_sync_handler(bus, HwBuffers::onBusSyncMessage, nullptr, nullptr);
            gst_clear_object(&bus);
        }

        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-initial");
        running = (gst_element_set_state(_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    } while(0);

    if (!running) {
        qCCritical(GstVideoReceiverLog) << "Failed";

        if (_pipeline) {
            (void) gst_element_set_state(_pipeline, GST_STATE_NULL);
            (void) gst_element_get_state(_pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            gst_clear_object(&_pipeline);
        }

        if (!pipelineUp) {
            gst_clear_object(&_recorderValve);
            gst_clear_object(&recorderQueue);
            gst_clear_object(&_decoderValve);
            gst_clear_object(&decoderQueue);
            gst_clear_object(&_tee);
            gst_clear_object(&_source);
        }

        emit onStartComplete(STATUS_FAIL);
    } else {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-started");
        qCDebug(GstVideoReceiverLog) << "Started" << _uri;

        // _watchdogTimer lives on `this` (GUI thread); the emit runs synchronously on the
        // worker thread, so the timer start has to be queued separately or QObject warns.
        QMetaObject::invokeMethod(this, [this]() { _watchdogTimer.start(1000); }, Qt::QueuedConnection);
        emit onStartComplete(STATUS_OK);
    }
}

void GstVideoReceiver::stop()
{
    if (_needDispatch()) {
        _worker->dispatch([this]() { stop(); });
        return;
    }

    if (_uri.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Stop called on empty URI (no-op)";
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping" << _uri;

    // Bump the epoch synchronously (atomic — no GUI thread needed) so any in-flight reconnect lambda
    // is superseded before this stop() returns; cross-callsite QueuedConnection FIFO is not guaranteed.
    _reconnectEpoch.fetch_add(1, std::memory_order_relaxed);
    // Only _watchdogTimer.stop() must run on the GUI thread (the timer lives on `this`).
    QMetaObject::invokeMethod(this, [this]() { _watchdogTimer.stop(); }, Qt::QueuedConnection);

    if (_teeProbeId != 0) {
        if (_tee) {
            GstPad *sinkpad = gst_element_get_static_pad(_tee, "sink");
            if (sinkpad) {
                gst_pad_remove_probe(sinkpad, _teeProbeId);
                gst_clear_object(&sinkpad);
            }
        }
        _teeProbeId = 0;
    }

    if (_pipeline) {
        GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
        if (bus) {
            gst_bus_disable_sync_message_emission(bus);
            (void) g_signal_handlers_disconnect_by_data(bus, this);

            gboolean recordingValveClosed = TRUE;
            g_object_get(_recorderValve, "drop", &recordingValveClosed, nullptr);

            if (!recordingValveClosed) {
                (void) gst_element_send_event(_pipeline, gst_event_new_eos());

                // Wait for splitmuxsink to actually finalize its current fragment. async-finalize
                // pushes muxer teardown off the streaming thread; the splitmuxsink-fragment-closed
                // element message is posted (via message-forward=TRUE) exactly when the muxer's
                // state has gone NULL. EOS is the fallback for older builds / unexpected paths;
                // ERROR breaks out so we don't burn the full budget on a known failure. Track
                // elapsed time so unrelated ELEMENT messages don't abort the wait early.
                const GstClockTime deadline = kEosTimeoutNs;
                const qint64 startMs = QDateTime::currentMSecsSinceEpoch();
                bool finalized = false;
                for (;;) {
                    const qint64 elapsedNs = (QDateTime::currentMSecsSinceEpoch() - startMs)
                                             * qint64(GST_MSECOND);
                    if (elapsedNs >= qint64(deadline)) break;
                    const GstClockTime remaining = GstClockTime(qint64(deadline) - elapsedNs);
                    GstMessage *msg = gst_bus_timed_pop_filtered(bus, remaining,
                            (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR | GST_MESSAGE_ELEMENT));
                    if (!msg) break;
                    switch (GST_MESSAGE_TYPE(msg)) {
                    case GST_MESSAGE_ELEMENT: {
                        const GstStructure *s = gst_message_get_structure(msg);
                        if (s && gst_structure_has_name(s, "splitmuxsink-fragment-closed")) {
                            qCDebug(GstVideoReceiverLog) << "splitmuxsink fragment finalized";
                            finalized = true;
                        }
                        break;
                    }
                    case GST_MESSAGE_EOS:
                        qCDebug(GstVideoReceiverLog) << "End of stream received (fallback path)";
                        finalized = true;
                        break;
                    case GST_MESSAGE_ERROR:
                        qCCritical(GstVideoReceiverLog) << "Error stopping pipeline!";
                        finalized = true;
                        break;
                    default:
                        break;
                    }
                    gst_clear_message(&msg);
                    if (finalized) break;
                }
                if (!finalized) {
                    qCWarning(GstVideoReceiverLog) << "splitmuxsink finalize signal not received within"
                                                   << (kEosTimeoutNs / GST_MSECOND)
                                                   << "ms — forcing pipeline NULL (recording may be truncated; "
                                                   << "faststart + reserved-moov-update-period keep the file playable)";
                }
            }

            gst_clear_object(&bus);
        } else {
            qCCritical(GstVideoReceiverLog) << "gst_pipeline_get_bus() failed";
        }

        (void) gst_element_set_state(_pipeline, GST_STATE_NULL);
        (void) gst_element_get_state(_pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

        // FIXME: check if branch is connected and remove all elements from branch
        if (_fileSink) {
           _shutdownRecordingBranch();
        }

        if (_videoSink) {
            _shutdownDecodingBranch();
        }

        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-stopped");

        // Lock before nulling so an in-flight _onBusMessage on the streaming thread cannot read
        // a half-destroyed _pipeline. _acquirePipelineRef takes its own ref under the same lock.
        {
            QMutexLocker lock(&_pipelineMutex);
            gst_clear_object(&_pipeline);
            _pipeline = nullptr;
        }

        _recorderValve = nullptr;
        _decoderValve = nullptr;
        _tee = nullptr;
        _source = nullptr;

        _lastSourceFrameTime = 0;

        if (_streaming) {
            _streaming = false;
            qCDebug(GstVideoReceiverLog) << "Streaming stopped" << _uri;
            emit streamingChanged(_streaming);
        } else {
            qCDebug(GstVideoReceiverLog) << "Streaming did not start" << _uri;
        }
    }

    qCDebug(GstVideoReceiverLog) << "Stopped" << _uri;

    if (const HwBuffers::PathStats hwStats = HwBuffers::formatPathStats(true); hwStats.totalDelivered > 0) {
        qCInfo(GstVideoReceiverLog).noquote()
            << "HW path stats" << _uri << hwStats.line + HwBuffers::takeExtraPathStats();
    }

    emit onStopComplete(STATUS_OK);
}

void GstVideoReceiver::startDecoding(void *sink)
{
    if (!sink) {
        qCCritical(GstVideoReceiverLog) << "VideoSink is NULL" << _uri;
        return;
    }

    if (_needDispatch()) {
        _worker->dispatch([this, sink]() mutable { startDecoding(sink); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Starting decoding" << _uri;

    if (!_widget) {
        qCDebug(GstVideoReceiverLog) << "Video Widget is NULL" << _uri;
        emit onStartDecodingComplete(STATUS_FAIL);
        return;
    }

    if (!_pipeline) {
        gst_clear_object(&_videoSink);
    }

    if (_videoSink || _decoding) {
        qCDebug(GstVideoReceiverLog) << "Already decoding!" << _uri;
        emit onStartDecodingComplete(STATUS_INVALID_STATE);
        return;
    }

    GstElement *videoSink = GST_ELEMENT(sink);
    GstPad *pad = gst_element_get_static_pad(videoSink, "sink");
    if (!pad) {
        qCCritical(GstVideoReceiverLog) << "Unable to find sink pad of video sink" << _uri;
        emit onStartDecodingComplete(STATUS_FAIL);
        return;
    }

    _lastVideoFrameTime = 0;
    _resetVideoSink = true;

    _videoSinkProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, _videoSinkProbe, this, nullptr);
    gst_clear_object(&pad);

    _videoSink = videoSink;
    gst_object_ref(_videoSink);

    _removingDecoder = false;

    if (!_streaming) {
        emit onStartDecodingComplete(STATUS_OK);
        return;
    }

    _ensureVideoSinkInPipeline();

    if (!_addDecoder(_decoderValve)) {
        qCCritical(GstVideoReceiverLog) << "_addDecoder() failed" << _uri;
        _shutdownDecodingBranch();
        emit onStartDecodingComplete(STATUS_FAIL);
        return;
    }

    g_object_set(_decoderValve,
                 "drop", FALSE,
                 nullptr);

    qCDebug(GstVideoReceiverLog) << "Decoding started" << _uri;

    emit onStartDecodingComplete(STATUS_OK);
}

void GstVideoReceiver::stopDecoding()
{
    if (_needDispatch()) {
        _worker->dispatch([this]() { stopDecoding(); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping decoding" << _uri;

    // Gate on _videoSink (set by startDecoding) instead of _decoding (which only flips on
    // first sink-buffer probe). Without this, stopDecoding() called between
    // onStartDecodingComplete(OK) and the first frame returns STATUS_INVALID_STATE and
    // leaves the decoder/sink branch live.
    if (!_pipeline || !_videoSink) {
        qCDebug(GstVideoReceiverLog) << "Not decoding!" << _uri;
        emit onStopDecodingComplete(STATUS_INVALID_STATE);
        return;
    }

    g_object_set(_decoderValve,
                 "drop", TRUE,
                 nullptr);

    _removingDecoder = true;

    const bool ret = _unlinkBranch(_decoderValve);

    // FIXME: it is much better to emit onStopDecodingComplete() after decoding is really stopped
    // (which happens later due to async design) but as for now it is also not so bad...
    emit onStopDecodingComplete(ret ? STATUS_OK : STATUS_FAIL);
}

void GstVideoReceiver::startRecording(const QString &videoFile, FILE_FORMAT format)
{
    if (_needDispatch()) {
        const QString cachedVideoFile = videoFile;
        _worker->dispatch([this, cachedVideoFile, format]() { startRecording(cachedVideoFile, format); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Starting recording" << _uri;

    if (!_pipeline) {
        qCDebug(GstVideoReceiverLog) << "Streaming is not active!" << _uri;
        emit onStartRecordingComplete(STATUS_INVALID_STATE);
        return;
    }

    if (_recording) {
        qCDebug(GstVideoReceiverLog) << "Already recording!" << _uri;
        emit onStartRecordingComplete(STATUS_INVALID_STATE);
        return;
    }

    qCDebug(GstVideoReceiverLog) << "New video file:" << videoFile << _uri;

    _fileSink = _makeFileSink(videoFile, format);
    if (!_fileSink) {
        qCCritical(GstVideoReceiverLog) << "_makeFileSink() failed" << _uri;
        emit onStartRecordingComplete(STATUS_FAIL);
        return;
    }

    _removingRecorder = false;

    (void) gst_object_ref(_fileSink);

    gst_bin_add(GST_BIN(_pipeline), _fileSink);

    if (!gst_element_link(_recorderValve, _fileSink)) {
        qCCritical(GstVideoReceiverLog) << "Failed to link valve and file sink" << _uri;
        emit onStartRecordingComplete(STATUS_FAIL);
        return;
    }

    (void) gst_element_sync_state_with_parent(_fileSink);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-filesink");

    // Install a probe on the recording branch to drop buffers until we hit our first keyframe
    // When we hit our first keyframe, we can offset the timestamps appropriately according to the first keyframe time
    // This will ensure the first frame is a keyframe at t=0, and decoding can begin immediately on playback
    GstPad *probepad = gst_element_get_static_pad(_recorderValve, "src");
    if (!probepad) {
        qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed" << _uri;
        emit onStartRecordingComplete(STATUS_FAIL);
        return;
    }

    _keyframeWatchId = gst_pad_add_probe(probepad, GST_PAD_PROBE_TYPE_BUFFER, _keyframeWatch, this, nullptr);
    gst_clear_object(&probepad);

    g_object_set(_recorderValve,
                 "drop", FALSE,
                 nullptr);

    _recordingOutput = videoFile;
    _recording = true;
    qCDebug(GstVideoReceiverLog) << "Recording started" << _uri;
    emit onStartRecordingComplete(STATUS_OK);
    emit recordingChanged(_recording);
}

void GstVideoReceiver::stopRecording()
{
    if (_needDispatch()) {
        _worker->dispatch([this]() { stopRecording(); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping recording" << _uri;

    if (!_pipeline || !_recording) {
        qCDebug(GstVideoReceiverLog) << "Not recording!" << _uri;
        emit onStopRecordingComplete(STATUS_INVALID_STATE);
        return;
    }

    g_object_set(_recorderValve,
                 "drop", TRUE,
                 nullptr);

    _removingRecorder = true;

    if (!_unlinkBranch(_recorderValve)) {
        _removingRecorder = false;
        emit onStopRecordingComplete(STATUS_FAIL);
        return;
    }

    // EOS event propagates valve→mux→filesink; _shutdownRecordingBranch emits the
    // complete signal once the muxer index is written and the file is closed.
    _recordingStopRequested = true;
}

void GstVideoReceiver::takeScreenshot(const QString &imageFile)
{
    if (_needDispatch()) {
        const QString cachedImageFile = imageFile;
        _worker->dispatch([this, cachedImageFile]() { takeScreenshot(cachedImageFile); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "taking screenshot" << _uri;

    // FIXME: record screenshot here
    emit onTakeScreenshotComplete(STATUS_NOT_IMPLEMENTED);
}

void GstVideoReceiver::_watchdog()
{
    _worker->dispatch([this]() {
        if (!_pipeline) {
            return;
        }

        const qint64 now = QDateTime::currentSecsSinceEpoch();
        qint64 lastSourceFrameTime = _lastSourceFrameTime.load(std::memory_order_relaxed);
        if (lastSourceFrameTime == 0) {
            lastSourceFrameTime = now;
            _lastSourceFrameTime.store(now, std::memory_order_relaxed);
        }

        if (++_statsTickCounter >= 10) {
            _statsTickCounter = 0;
            if (const HwBuffers::PathStats hwStats = HwBuffers::formatPathStats(false); hwStats.totalDelivered > 0) {
                qCDebug(GstVideoReceiverLog).noquote() << "HW path live" << _uri << hwStats.line;
            }
        }

        // Drain QoS updates accumulated since the last tick (see GST_MESSAGE_QOS).
        if (_qosStatsDirty.exchange(false, std::memory_order_acq_rel)) {
            emit decoderStatsChanged();
        }

        qint64 elapsed = now - lastSourceFrameTime;
        if (elapsed > _timeout) {
            qCDebug(GstVideoReceiverLog) << "Stream timeout, no frames for" << elapsed << _uri;
            GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-watchdog-timeout");
            emit timeout();
            _scheduleReconnect("source watchdog");
            return;
        }

        if (_decoding && !_removingDecoder) {
            qint64 lastVideoFrameTime = _lastVideoFrameTime.load(std::memory_order_relaxed);
            if (lastVideoFrameTime == 0) {
                lastVideoFrameTime = now;
                _lastVideoFrameTime.store(now, std::memory_order_relaxed);
            }

            elapsed = now - lastVideoFrameTime;
            if (elapsed > (_timeout * 2)) {
                qCDebug(GstVideoReceiverLog) << "Video decoder timeout, no frames for" << elapsed << _uri;
                GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-watchdog-timeout");
                emit timeout();
                _scheduleReconnect("decoder watchdog");
            }
        }
    });
}

void GstVideoReceiver::_scheduleReconnect(const char *reason)
{
    // Always tear down — even when autoReconnect is off we still want a clean stop.
    // stop() bumps _reconnectEpoch, so any prior singleShot lambda becomes a no-op.
    stop();

    if (!_autoReconnect) {
        qCDebug(GstVideoReceiverLog) << "Auto-reconnect disabled — not retrying after" << reason;
        return;
    }

    if (_uri.isEmpty()) {
        return;
    }

    // Snapshot on the worker thread — where start() last wrote _timeout and where _uri reads
    // are already sequenced — so the GUI-thread lambdas below don't read racy members.
    const uint32_t reconnectTimeout = (_timeout != 0) ? _timeout : 8;
    const QString uri = _uri;

    // Schedule on the GUI thread (QTimer::singleShot requires its receiver's thread). Worker
    // is the only caller today, but route through invokeMethod so a future direct GUI-thread
    // call (e.g. user-initiated retry) stays correct.
    QMetaObject::invokeMethod(this, [this, reason, reconnectTimeout, uri]() {
        const int next = std::min(_reconnectAttempts.load(std::memory_order_relaxed) + 1, 30);
        _reconnectAttempts.store(next, std::memory_order_relaxed);
        // 1s → 2s → 4s → 8s → 16s, capped at 30s. Capping bounds worst-case "vehicle in flight,
        // RF down for 5 min" recovery; lower than typical RTSP server keepalive (60s).
        const int delaySec = std::min(1 << std::min(next - 1, 5), 30);
        const quint64 epoch = _reconnectEpoch.load(std::memory_order_relaxed);
        const int attempts = next;
        qCInfo(GstVideoReceiverLog) << "Scheduling reconnect #" << attempts
                                    << "in" << delaySec << "s after" << reason << uri;
        QTimer::singleShot(delaySec * 1000, this, [this, epoch, attempts, reconnectTimeout, uri]() {
            if (epoch != _reconnectEpoch.load(std::memory_order_relaxed)) return;  // superseded by stop()
            // _pipeline is mutated by the worker under _pipelineMutex; a bare deref here (GUI
            // thread) races teardown, so probe liveness through the mutex-guarded accessor.
            GstElement *livePipeline = _acquirePipelineRef();
            const bool pipelineUp = (livePipeline != nullptr);
            if (livePipeline) gst_object_unref(livePipeline);
            if (uri.isEmpty() || pipelineUp) return;  // pipeline already came back
            qCInfo(GstVideoReceiverLog) << "Reconnecting (attempt" << attempts << ")" << uri;
            start(reconnectTimeout);
        });
    }, Qt::QueuedConnection);
}

void GstVideoReceiver::dumpPipelineGraph(const QString &tag)
{
    _worker->dispatch([this, tag]() {
        GstElement *pipelineRef = _acquirePipelineRef();
        if (!pipelineRef) {
            qCDebug(GstVideoReceiverLog) << "dumpPipelineGraph: pipeline not running";
            return;
        }
        const QByteArray tagUtf8 = tag.toUtf8();
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipelineRef), GST_DEBUG_GRAPH_SHOW_ALL, tagUtf8.constData());
        const QString dotPath = GStreamer::writePipelineDot(pipelineRef, tagUtf8.constData());
        if (!dotPath.isEmpty()) {
            qCInfo(GstVideoReceiverLog) << "Pipeline graph saved to" << dotPath;
        }
        gst_object_unref(pipelineRef);
    });
}

void GstVideoReceiver::_handleEOS()
{
    if (!_pipeline) {
        return;
    }

    if (_endOfStream) {
        stop();
    } else if (_decoding && _removingDecoder) {
        _shutdownDecodingBranch();
    } else if (_recording && _removingRecorder) {
        _shutdownRecordingBranch();
    } /*else {
        qCWarning(GstVideoReceiverLog) << "Unexpected EOS!";
        stop();
    }*/
}

GstElement *GstVideoReceiver::_makeDecoder()
{
    GstElement *decoder = gst_element_factory_make("decodebin3", nullptr);
    if (!decoder) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('decodebin3') failed";
    }
    return decoder;
}

GstElement *GstVideoReceiver::_makeFileSink(const QString &videoFile, FILE_FORMAT format)
{
    GstElement *fileSink = nullptr;
    GstElement *splitmux = nullptr;
    GstElement *bin = nullptr;
    GstPad *videopad = nullptr;
    GstPad *ghostpad = nullptr;

    do {
        if (!isValidFileFormat(format)) {
            qCCritical(GstVideoReceiverLog) << "Unsupported file format";
            break;
        }

        // splitmuxsink owns its own muxer + filesink internally, handles request-pad
        // lifetime, and finalizes asynchronously so EOS no longer wedges the worker
        // thread (replaces the manual qtmux/matroskamux+filesink combo + "stuck muxer"
        // bounded-wait in stop()). max-size-time=0 keeps single-file behaviour.
        splitmux = gst_element_factory_make("splitmuxsink", nullptr);
        if (!splitmux) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('splitmuxsink') failed";
            break;
        }

        g_object_set(splitmux,
                     "location", qPrintable(videoFile),
                     "muxer-factory", _kFileMux[format],
                     "max-size-time", G_GUINT64_CONSTANT(0),
                     "max-size-bytes", G_GUINT64_CONSTANT(0),
                     "async-finalize", TRUE,
                     // Surface "splitmuxsink-fragment-closed" element messages on the pipeline bus so
                     // stop() can wait on the precise per-fragment finalize signal instead of EOS
                     // (gstsplitmuxsink.c:send_fragment_opened_closed_msg posts this per fragment,
                     // including the final fragment torn down on EOS).
                     "message-forward", TRUE,
                     nullptr);

        // Crash-safe MP4/MOV: faststart writes moov up-front; reserved-moov-update-period
        // refreshes the moov on a 1 s cadence so an abrupt kill still leaves a playable file.
        // matroskamux is naturally streamable; skip the GstStructure dance.
        if (format == FILE_FORMAT_MP4 || format == FILE_FORMAT_MOV) {
            GstStructure *muxerProps = gst_structure_new("properties",
                "faststart", G_TYPE_BOOLEAN, TRUE,
                "reserved-moov-update-period", G_TYPE_UINT64, G_GUINT64_CONSTANT(1000000000),
                nullptr);
            g_object_set(splitmux, "muxer-properties", muxerProps, nullptr);
            gst_structure_free(muxerProps);
        }

        bin = gst_bin_new("sinkbin");
        if (!bin) {
            qCCritical(GstVideoReceiverLog) << "gst_bin_new('sinkbin') failed";
            break;
        }

        // splitmuxsink's video sink pad is a request pad — request once during construction
        // and ghost it as "sink" so the existing recorderValve→fileSink link works unchanged.
        // request_pad_simple (1.20+) does the pad-template lookup internally.
        videopad = gst_element_request_pad_simple(splitmux, "video");
        if (!videopad) {
            qCCritical(GstVideoReceiverLog) << "gst_element_request_pad_simple(splitmuxsink, video) failed";
            break;
        }

        if (!gst_bin_add(GST_BIN(bin), splitmux)) {
            qCCritical(GstVideoReceiverLog) << "gst_bin_add(splitmuxsink) failed";
            break;
        }
        splitmux = nullptr;  // bin now owns it

        ghostpad = gst_ghost_pad_new("sink", videopad);
        if (!ghostpad) {
            qCCritical(GstVideoReceiverLog) << "gst_ghost_pad_new() failed";
            break;
        }

        if (!gst_element_add_pad(bin, ghostpad)) {
            qCCritical(GstVideoReceiverLog) << "gst_element_add_pad(ghost) failed";
            break;
        }
        ghostpad = nullptr;  // bin now owns it

        fileSink = bin;
        bin = nullptr;
    } while(0);

    gst_clear_object(&ghostpad);
    // No release_request_pad: on success splitmux is already NULL (owned by bin), and on failure the
    // bin/splitmux unref below finalizes splitmuxsink, which reclaims its "video" request pad itself.
    gst_clear_object(&videopad);
    gst_clear_object(&splitmux);
    gst_clear_object(&bin);
    return fileSink;
}

void GstVideoReceiver::_onNewSourcePad(GstPad *pad)
{
    // FIXME: check for caps - if this is not video stream (and preferably - one of these which we have to support) then simply skip it
    if (!gst_element_link(_source, _tee)) {
        qCCritical(GstVideoReceiverLog) << "Unable to link source";
        return;
    }

    if (!_streaming) {
        _streaming = true;
        qCDebug(GstVideoReceiverLog) << "Streaming started" << _uri;
        emit streamingChanged(_streaming);
    }

    _eosProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, _eosProbe, this, nullptr);
    if (_eosProbeId != 0) {
        // Hold a ref so _shutdownDecodingBranch can remove the probe even after _decoder is gone.
        _eosProbePad = GST_PAD_CAST(gst_object_ref(pad));
    }
    if (!_videoSink) {
        return;
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-new-source-pad");

    _ensureVideoSinkInPipeline();

    if (!_addDecoder(_decoderValve)) {
        qCCritical(GstVideoReceiverLog) << "_addDecoder() failed";
        _shutdownDecodingBranch();
        return;
    }

    g_object_set(_decoderValve,
                 "drop", FALSE,
                 nullptr);

    qCDebug(GstVideoReceiverLog) << "Decoding started" << _uri;
}

void GstVideoReceiver::_logDecodebin3SelectedCodec(GstElement *decodebin3)
{
    GValue value = G_VALUE_INIT;
    GstIterator *iter = gst_bin_iterate_elements(GST_BIN(decodebin3));
    GstElement *child;

    while (gst_iterator_next(iter, &value) == GST_ITERATOR_OK) {
        child = GST_ELEMENT(g_value_get_object(&value));
        GstElementFactory *factory = gst_element_get_factory(child);

        if (factory) {
            gboolean is_decoder = gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER);
            if (is_decoder) {
                const gchar *decoderKlass = gst_element_factory_get_klass(factory);
                GstPluginFeature *feature = GST_PLUGIN_FEATURE(factory);
                const gchar *featureName = gst_plugin_feature_get_name(feature);
                const guint rank = gst_plugin_feature_get_rank(feature);
                bool isHardwareDecoder = GStreamer::isHardwareDecoderFactory(factory);

                QString pluginName = featureName;
                GstPlugin *plugin = gst_plugin_feature_get_plugin(feature);
                if (plugin) {
                    pluginName = gst_plugin_get_name(plugin);
                    gst_object_unref(plugin);
                }
                qCDebug(GstVideoReceiverLog) << "Decodebin3 selected codec:rank -" << pluginName << "/" << featureName << "-" << decoderKlass << (isHardwareDecoder ? "(HW)" : "(SW)") << ":" << rank;

                const QString newName = QString::fromUtf8(featureName);
                bool nameChanged = false;
                {
                    QMutexLocker locker(&_decoderNameMutex);
                    if (newName != _decoderName) {
                        _decoderName = newName;
                        nameChanged = true;
                    }
                }
                if (nameChanged) {
                    emit decoderStatsChanged();
                }

                // Disable QoS on the internal decoder to prevent cascading
                // frame drops on live streams.  The videodecoder base class
                // aggressively advances earliest_time after the first late
                // frame, causing all subsequent frames to be dropped.
                g_object_set(child, "qos", FALSE, nullptr);
                qCDebug(GstVideoReceiverLog) << "Disabled QoS on internal decoder" << featureName;
            }
        }
        g_value_reset(&value);
    }
    g_value_unset(&value);
    gst_iterator_free(iter);
}


void GstVideoReceiver::_onNewDecoderPad(GstPad *pad)
{
    qCDebug(GstVideoReceiverLog) << "_onNewDecoderPad" << _uri;

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-new-decoder-pad");

    // We should now know what codec decodebin3 selected.
    _logDecodebin3SelectedCodec(_decoder);

    if (!_addVideoSink(pad)) {
        qCCritical(GstVideoReceiverLog) << "_addVideoSink() failed";
    }
}

bool GstVideoReceiver::_addDecoder(GstElement *src)
{
    _decoder = _makeDecoder();
    if (!_decoder) {
        qCCritical(GstVideoReceiverLog) << "_makeDecoder() failed";
        return false;
    }

    (void) gst_object_ref(_decoder);

    (void) gst_bin_add(GST_BIN(_pipeline), _decoder);
    (void) gst_element_sync_state_with_parent(_decoder);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-decoder");

    if (!gst_element_link(src, _decoder)) {
        qCCritical(GstVideoReceiverLog) << "Unable to link decoder";
        gst_element_set_state(_decoder, GST_STATE_NULL);
        (void) gst_element_get_state(_decoder, nullptr, nullptr, GST_CLOCK_TIME_NONE);
        (void) gst_bin_remove(GST_BIN(_pipeline), _decoder);
        gst_clear_object(&_decoder);
        return false;
    }

    GstPad *srcPad = nullptr;
    (void) gst_element_foreach_src_pad(_decoder, grabFirstSrcPad, &srcPad);

    if (srcPad) {
        _onNewDecoderPad(srcPad);
    } else {
        (void) g_signal_connect(_decoder, "pad-added", G_CALLBACK(_onNewPad), this);
    }

    gst_clear_object(&srcPad);
    return true;
}

void GstVideoReceiver::_ensureVideoSinkInPipeline()
{
    if (!_videoSink || !_pipeline) {
        return;
    }

    GstObject *parent = gst_element_get_parent(_videoSink);
    if (parent) {
        gst_object_unref(parent);
        return;
    }

    g_object_set(_videoSink,
                 "sync", (_buffer >= 0),
                 NULL);

    (void) gst_object_ref(_videoSink);
    (void) gst_bin_add(GST_BIN(_pipeline), _videoSink);

    // PAUSED (not READY) triggers downstream caps negotiation before source data arrives.
    (void) gst_element_set_state(_videoSink, GST_STATE_PAUSED);
}

bool GstVideoReceiver::_addVideoSink(GstPad *pad)
{
    GstCaps *caps = gst_pad_query_caps(pad, nullptr);

    _ensureVideoSinkInPipeline();

    GstPad *sinkPad = gst_element_get_static_pad(_videoSink, "sink");
    GstPadLinkReturn linkRet = sinkPad ? gst_pad_link(pad, sinkPad) : GST_PAD_LINK_WRONG_HIERARCHY;
    if (linkRet != GST_PAD_LINK_OK) {
        qCCritical(GstVideoReceiverLog) << "Unable to link decoder pad to video sink, result:" << linkRet;

        // _ensureVideoSinkInPipeline() added it before linking; detach for the next retry.
        GstObject *parent = gst_element_get_parent(_videoSink);
        if (parent) {
            (void) gst_element_set_state(_videoSink, GST_STATE_NULL);
            (void) gst_element_get_state(_videoSink, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            (void) gst_bin_remove(GST_BIN(_pipeline), _videoSink);
            gst_clear_object(&parent);
        }

        gst_clear_object(&sinkPad);
        gst_clear_caps(&caps);
        return false;
    }
    gst_clear_object(&sinkPad);

    (void) gst_element_sync_state_with_parent(_videoSink);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-videosink");

    // Determine video size. Errors here are non-fatal.
    QSize videoSize;
    do {
        if (!_decoderValve) {
            qCCritical(GstVideoReceiverLog) << "Unable to determine video size - _decoderValve is NULL" << _uri;
            break;
        }

        GstPad *valveSrcPad = gst_element_get_static_pad(_decoderValve, "src");
        if (!valveSrcPad) {
            qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
            break;
        }

        GstCaps *valveSrcPadCaps = gst_pad_query_caps(valveSrcPad, nullptr);
        if (!valveSrcPadCaps) {
            qCCritical(GstVideoReceiverLog) << "gst_pad_query_caps() failed";
            gst_clear_object(&valveSrcPad);
            break;
        }

        const GstStructure *structure = gst_caps_get_structure(valveSrcPadCaps, 0);
        if (!structure) {
            qCCritical(GstVideoReceiverLog) << "Unable to determine video size - structure is NULL" << _uri;
            gst_clear_object(&valveSrcPad);
            break;
        }

        gint width = 0;
        gint height = 0;
        (void) gst_structure_get_int(structure, "width", &width);
        (void) gst_structure_get_int(structure, "height", &height);

        // Swap W×H for 90°/270° streams so QML AR is computed on display dimensions.
        gint orientation = 0;
        if (gst_structure_get_int(structure, "video-orientation", &orientation)
            && (orientation == GST_VIDEO_ORIENTATION_90R
                || orientation == GST_VIDEO_ORIENTATION_90L
                || orientation == GST_VIDEO_ORIENTATION_UL_LR
                || orientation == GST_VIDEO_ORIENTATION_UR_LL)) {
            videoSize.setWidth(height);
            videoSize.setHeight(width);
        } else {
            videoSize.setWidth(width);
            videoSize.setHeight(height);
        }

        gst_clear_caps(&valveSrcPadCaps);
        gst_clear_object(&valveSrcPad);
    } while (false);
    emit videoSizeChanged(videoSize);

    gst_clear_caps(&caps);
    return true;
}

void GstVideoReceiver::_noteTeeFrame()
{
    _lastSourceFrameTime.store(QDateTime::currentSecsSinceEpoch(), std::memory_order_relaxed);
    // Successful frame arrival: drop the reconnect backoff so the next failure starts at 1 s,
    // not minutes-into-the-curve. This probe runs on the streaming thread while the backoff
    // increment runs on the GUI thread; post the reset there too so all mutation of
    // _reconnectAttempts is single-threaded and the increment can't clobber the reset.
    if (_reconnectAttempts.load(std::memory_order_relaxed) != 0) {
        QMetaObject::invokeMethod(
            this, [this]() { _reconnectAttempts.store(0, std::memory_order_relaxed); }, Qt::QueuedConnection);
    }
    const quint64 sourceFrames = _sourceFrameCount.fetch_add(1, std::memory_order_relaxed) + 1;
    if (sourceFrames == 1) {
        qCInfo(GstVideoReceiverLog).noquote() << "Source receiving frames (tee):" << _uri;
    } else if ((sourceFrames % 300) == 0) {
        qCDebug(GstVideoReceiverLog).noquote()
            << "Source flow: teeFrames=" << sourceFrames << "decoding=" << _decoding << _uri;
    }
}

void GstVideoReceiver::_noteVideoSinkFrame()
{
    _lastVideoFrameTime.store(QDateTime::currentSecsSinceEpoch(), std::memory_order_relaxed);
    if (!_decoding) {
        _decoding = true;
        qCDebug(GstVideoReceiverLog) << "Decoding started";
        emit decodingChanged(_decoding);
    }
}

void GstVideoReceiver::_noteEndOfStream()
{
    _endOfStream = true;
}

bool GstVideoReceiver::_unlinkBranch(GstElement *from)
{
    GstPad *src = gst_element_get_static_pad(from, "src");
    if (!src) {
        qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
        return false;
    }

    GstPad *sink = gst_pad_get_peer(src);
    if (!sink) {
        gst_clear_object(&src);
        qCCritical(GstVideoReceiverLog) << "gst_pad_get_peer() failed";
        return false;
    }

    if (!gst_pad_unlink(src, sink)) {
        gst_clear_object(&src);
        gst_clear_object(&sink);
        qCCritical(GstVideoReceiverLog) << "gst_pad_unlink() failed";
        return false;
    }

    gst_clear_object(&src);

    // Send EOS at the beginning of the branch
    const gboolean ret = gst_pad_send_event(sink, gst_event_new_eos());

    gst_clear_object(&sink);

    if (!ret) {
        qCCritical(GstVideoReceiverLog) << "Branch EOS was NOT sent";
        return false;
    }

    qCDebug(GstVideoReceiverLog) << "Branch EOS was sent";

    return true;
}

void GstVideoReceiver::_shutdownDecodingBranch()
{
    if (_decoder) {
        GstObject *parent = gst_element_get_parent(_decoder);
        if (parent) {
            (void) gst_bin_remove(GST_BIN(_pipeline), _decoder);
            (void) gst_element_set_state(_decoder, GST_STATE_NULL);
            (void) gst_element_get_state(_decoder, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            gst_clear_object(&parent);
        }

        gst_clear_object(&_decoder);
    }

    if (_videoSinkProbeId != 0 && _videoSink) {
        GstPad *sinkpad = gst_element_get_static_pad(_videoSink, "sink");
        if (sinkpad) {
            gst_pad_remove_probe(sinkpad, _videoSinkProbeId);
            gst_clear_object(&sinkpad);
        }
    }
    _videoSinkProbeId = 0;

    if (_eosProbeId != 0 && _eosProbePad) {
        // Probe was installed on the source pad in _onNewSourcePad; remove from that exact pad — not from _decoder, which may already be cleared above.
        gst_pad_remove_probe(_eosProbePad, _eosProbeId);
    }
    _eosProbeId = 0;
    gst_clear_object(&_eosProbePad);

    _lastVideoFrameTime = 0;

    if (_videoSink) {
        GstObject *parent = gst_element_get_parent(_videoSink);
        if (parent) {
            (void) gst_bin_remove(GST_BIN(_pipeline), _videoSink);
            (void) gst_element_set_state(_videoSink, GST_STATE_NULL);
            (void) gst_element_get_state(_videoSink, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            gst_clear_object(&parent);
        }
        gst_clear_object(&_videoSink);
    }

    _removingDecoder = false;

    if (_decoding) {
        _decoding = false;
        qCDebug(GstVideoReceiverLog) << "Decoding stopped";
        emit decodingChanged(_decoding);
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-decoding-stopped");
}

void GstVideoReceiver::_shutdownRecordingBranch()
{
    if (_keyframeWatchId != 0 && _recorderValve) {
        GstPad *probepad = gst_element_get_static_pad(_recorderValve, "src");
        if (probepad) {
            gst_pad_remove_probe(probepad, _keyframeWatchId);
            gst_clear_object(&probepad);
        }
        _keyframeWatchId = 0;
    }

    gst_bin_remove(GST_BIN(_pipeline), _fileSink);
    gst_element_set_state(_fileSink, GST_STATE_NULL);
    (void) gst_element_get_state(_fileSink, nullptr, nullptr, GST_CLOCK_TIME_NONE);
    gst_clear_object(&_fileSink);

    _removingRecorder = false;

    if (_recording) {
        _recording = false;
        qCDebug(GstVideoReceiverLog) << "Recording stopped";
        emit recordingChanged(_recording);
    }

    if (_recordingStopRequested) {
        _recordingStopRequested = false;
        emit onStopRecordingComplete(STATUS_OK);
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-recording-stopped");
}

bool GstVideoReceiver::_needDispatch()
{
    return _worker->needDispatch();
}

GstElement *GstVideoReceiver::_acquirePipelineRef() const
{
    QMutexLocker lock(&_pipelineMutex);
    if (!_pipeline) return nullptr;
    return GST_ELEMENT(gst_object_ref(_pipeline));
}

gboolean GstVideoReceiver::_onBusMessage(GstBus * /* bus */, GstMessage *msg, gpointer data)
{
    if (!msg || !data) {
        qCCritical(GstVideoReceiverLog) << "Invalid parameters in _onBusMessage: msg=" << msg << "data=" << data;
        return TRUE;
    }

    GstVideoReceiver *pThis = static_cast<GstVideoReceiver*>(data);

    if (GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR) {
        HwBuffers::dispatchBusMessage(msg);
    }

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
        gchar *debug = nullptr;
        GError *error = nullptr;
        gst_message_parse_error(msg, &error, &debug);
        const bool recoverableH265PaciError = isRecoverableH265PaciError(msg, error, debug);

        if (debug) {
            qCDebug(GstVideoReceiverLog) << "GStreamer debug:" << debug;
            g_clear_pointer(&debug, g_free);
        }

        if (error) {
            if (recoverableH265PaciError) {
                qCWarning(GstVideoReceiverLog)
                    << "Ignoring unsupported H.265 RTP PACI packet from rtph265depay:" << error->message;
            } else {
                qCCritical(GstVideoReceiverLog) << "GStreamer error:" << error->message;
            }
            g_clear_error(&error);
        }

        if (recoverableH265PaciError) {
            break;
        }

        HwBuffers::dispatchBusMessage(msg);

        if (GstElement *pipelineRef = pThis->_acquirePipelineRef()) {
            // Native dump path (no-op without GST_DEBUG_DUMP_DOT_DIR) plus an unconditional
            // CacheLocation fallback so field-bug-report bundles include pipeline topology.
            GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipelineRef), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-error");
            const QString dotPath = GStreamer::writePipelineDot(pipelineRef, "pipeline-error");
            if (!dotPath.isEmpty()) {
                qCInfo(GstVideoReceiverLog) << "Pipeline graph saved to" << dotPath;
            }
            gst_object_unref(pipelineRef);
        }

        // GPU-side ERROR handling (cached-device drop) runs in HwBuffers::dispatchBusMessage above.
        // _scheduleReconnect calls stop() then queues a backoff retry if autoReconnect is on.
        pThis->_worker->dispatch([pThis]() {
            qCDebug(GstVideoReceiverLog) << "Stopping because of error";
            pThis->_scheduleReconnect("pipeline error");
        });
        break;
    }
    case GST_MESSAGE_WARNING: {
        // GStreamer posts WARNING for caps mismatches, decoder fallbacks, clock drift —
        // surfacing keeps these visible without escalating to STATUS_FAIL.
        gchar *debug = nullptr;
        GError *error = nullptr;
        gst_message_parse_warning(msg, &error, &debug);
        qCWarning(GstVideoReceiverLog) << "GStreamer warning:"
                                       << (error ? error->message : "(no message)")
                                       << "debug:" << (debug ? debug : "(none)");
        g_clear_error(&error);
        g_clear_pointer(&debug, g_free);
        break;
    }
    case GST_MESSAGE_EOS:
        pThis->_worker->dispatch([pThis]() {
            qCDebug(GstVideoReceiverLog) << "Received EOS";
            pThis->_handleEOS();
        });
        break;
    case GST_MESSAGE_STREAM_COLLECTION: {
        GstStreamCollection *collection = nullptr;
        gst_message_parse_stream_collection(msg, &collection);
        if (!collection) {
            break;
        }
        // SELECT_STREAMS keeps decodebin3 from instantiating audio decoder branches.
        GList *selectedIds = nullptr;
        const guint nStreams = gst_stream_collection_get_size(collection);
        for (guint i = 0; i < nStreams; ++i) {
            GstStream *stream = gst_stream_collection_get_stream(collection, i);
            const GstStreamType type = gst_stream_get_stream_type(stream);
            if (type & GST_STREAM_TYPE_VIDEO) {
                selectedIds = g_list_append(selectedIds,
                    g_strdup(gst_stream_get_stream_id(stream)));
            }
        }
        if (selectedIds) {
            GstEvent *event = gst_event_new_select_streams(selectedIds);
            gst_element_send_event(GST_ELEMENT(GST_MESSAGE_SRC(msg)), event);
            g_list_free_full(selectedIds, g_free);
        }
        gst_object_unref(collection);
        break;
    }
    case GST_MESSAGE_QOS: {
        guint64 processed = 0, dropped = 0;
        gst_message_parse_qos_stats(msg, nullptr, &processed, &dropped);

        gint64 jitter = 0;
        gdouble proportion = 0;
        gint quality = 0;
        gst_message_parse_qos_values(msg, &jitter, &proportion, &quality);

        pThis->_processedFrames.store(processed, std::memory_order_relaxed);
        pThis->_droppedFrames.store(dropped, std::memory_order_relaxed);
        pThis->_currentJitterNs.store(jitter, std::memory_order_relaxed);
        pThis->_qosProportion.store(proportion, std::memory_order_relaxed);
        pThis->_qosQuality.store(quality, std::memory_order_relaxed);
        // GstBaseSink can post QOS per dropped buffer; defer the emit to the 1 Hz
        // watchdog tick so QML isn't flooded from the streaming thread.
        pThis->_qosStatsDirty.store(true, std::memory_order_release);
        break;
    }
    case GST_MESSAGE_ELEMENT: {
        const GstStructure *structure = gst_message_get_structure(msg);
        if (structure && gst_structure_has_name(structure, "qgc-caps-info")) {
            gint w = 0, h = 0;
            const gchar *fmt = gst_structure_get_string(structure, "format");
            gst_structure_get_int(structure, "width", &w);
            gst_structure_get_int(structure, "height", &h);
            const QString format = QString::fromUtf8(fmt ? fmt : "");
            const QSize resolution(w, h);
            // src compared by address only on the GUI thread; never dereferenced (may be gone by then).
            void *src = GST_MESSAGE_SRC(msg);
            QMetaObject::invokeMethod(pThis, [pThis, format, resolution, src]() {
                for (auto *c : QGCQVideoSinkController::controllersOf(pThis)) {
                    if (static_cast<const void*>(c->element()) == src) {
                        c->updateNegotiation(format, resolution);
                    }
                }
            }, Qt::QueuedConnection);
            break;
        }
        if (!gst_structure_has_name(structure, "GstBinForwarded")) {
            break;
        }

        GstMessage *forward_msg = nullptr;
        gst_structure_get(structure, "message", GST_TYPE_MESSAGE, &forward_msg, NULL);
        if (!forward_msg) {
            break;
        }

        if (GST_MESSAGE_TYPE(forward_msg) == GST_MESSAGE_EOS) {
            pThis->_worker->dispatch([pThis]() {
                qCDebug(GstVideoReceiverLog) << "Received branch EOS";
                pThis->_handleEOS();
            });
        }

        gst_clear_message(&forward_msg);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
        GstElement *pipelineRef = pThis->_acquirePipelineRef();
        if (!pipelineRef) break;
        const bool fromPipeline = (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipelineRef));
        if (!fromPipeline) {
            gst_object_unref(pipelineRef);
            break;
        }
        GstState oldState = GST_STATE_NULL, newState = GST_STATE_NULL;
        gst_message_parse_state_changed(msg, &oldState, &newState, nullptr);
        if (newState == GST_STATE_PLAYING && oldState != GST_STATE_PLAYING) {
            GstClockTime min = 0, max = 0;
            GstQuery *q = gst_query_new_latency();
            if (gst_element_query(pipelineRef, q)) {
                gboolean live = FALSE;
                gst_query_parse_latency(q, &live, &min, &max);
            }
            gst_query_unref(q);
            const QString decName = pThis->decoderName();
            qCDebug(GstVideoReceiverLog).noquote()
                << "Pipeline PLAYING:" << pThis->_uri
                << "decoder:" << (decName.isEmpty() ? QStringLiteral("(pending)") : decName)
                << "min-latency:" << (min / 1000000) << "ms"
                << "max-latency:" << (max / 1000000) << "ms";
        }
        gst_object_unref(pipelineRef);
        break;
    }
    case GST_MESSAGE_LATENCY:
        pThis->_worker->dispatch([pThis]() {
            GstElement* pipeline = pThis->_acquirePipelineRef();
            if (pipeline) {
                (void) gst_bin_recalculate_latency(GST_BIN(pipeline));
                gst_object_unref(pipeline);
            }
        });
        // Re-prime sink-side latency tracking after the pipeline recalculation (e.g. RTSP
        // jitter-buffer reconfigure). Controllers live on the GUI thread; hop there to query.
        QMetaObject::invokeMethod(pThis, [pThis]() {
            for (auto* c : QGCQVideoSinkController::controllersOf(pThis))
                c->refreshLatency();
        }, Qt::QueuedConnection);
        break;
    default:
        break;
    }

    return TRUE;
}

void GstVideoReceiver::_onNewPad(GstElement *element, GstPad *pad, gpointer data)
{
    GstVideoReceiver *self = static_cast<GstVideoReceiver*>(data);

    if (element == self->_source) {
        self->_onNewSourcePad(pad);
    } else if (element == self->_decoder) {
        self->_onNewDecoderPad(pad);
    } else {
        qCDebug(GstVideoReceiverLog) << "Unexpected call!";
    }
}

GstPadProbeReturn GstVideoReceiver::_teeProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad); Q_UNUSED(info)

    if (user_data) {
        GstVideoReceiver *pThis = static_cast<GstVideoReceiver*>(user_data);
        pThis->_noteTeeFrame();
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn GstVideoReceiver::_videoSinkProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad); Q_UNUSED(info)

    if (user_data) {
        GstVideoReceiver *pThis = static_cast<GstVideoReceiver*>(user_data);

        if (pThis->_resetVideoSink) {
            pThis->_resetVideoSink = false;

#if 0 // FIXME: this makes MPEG2-TS playing smooth but breaks RTSP
           gst_pad_send_event(pad, gst_event_new_flush_start());
           gst_pad_send_event(pad, gst_event_new_flush_stop(TRUE));

           GstBuffer* buf;

           if ((buf = gst_pad_probe_info_get_buffer(info)) != nullptr) {
               GstSegment* seg;

               if ((seg = gst_segment_new()) != nullptr) {
                   gst_segment_init(seg, GST_FORMAT_TIME);

                   seg->start = buf->pts;

                   gst_pad_send_event(pad, gst_event_new_segment(seg));

                   gst_segment_free(seg);
                   seg = nullptr;
               }

               gst_pad_set_offset(pad, -static_cast<gint64>(buf->pts));
           }
#endif
        }

        pThis->_noteVideoSinkFrame();
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn GstVideoReceiver::_eosProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad);
    Q_ASSERT(user_data);

    if (info) {
        const GstEvent *event = gst_pad_probe_info_get_event(info);
        if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
            GstVideoReceiver *pThis = static_cast<GstVideoReceiver*>(user_data);
            pThis->_noteEndOfStream();
        }
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn GstVideoReceiver::_keyframeWatch(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    if (!info || !user_data) {
        qCCritical(GstVideoReceiverLog) << "Invalid arguments";
        return GST_PAD_PROBE_DROP;
    }

    GstBuffer *buf = gst_pad_probe_info_get_buffer(info);
    if (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT)) {
        // wait for a keyframe
        return GST_PAD_PROBE_DROP;
    }

    // set media file '0' offset to current timeline position - we don't want to touch other elements in the graph, except these which are downstream!
    gst_pad_set_offset(pad, -static_cast<gint64>(buf->pts));

    qCDebug(GstVideoReceiverLog) << "Got keyframe, stop dropping buffers";

    GstVideoReceiver *pThis = static_cast<GstVideoReceiver*>(user_data);
    emit pThis->recordingStarted(pThis->recordingOutput());

    return GST_PAD_PROBE_REMOVE;
}

GstVideoWorker::GstVideoWorker(QObject *parent)
    : QThread(parent)
{
    qCDebug(GstVideoReceiverLog) << this;
}

GstVideoWorker::~GstVideoWorker()
{
    qCDebug(GstVideoReceiverLog) << this;
}

bool GstVideoWorker::needDispatch() const
{
    return (QThread::currentThread() != this);
}

void GstVideoWorker::dispatch(Task task)
{
    QMutexLocker lock(&_taskQueueSync);
    _taskQueue.enqueue(task);
    _taskQueueUpdate.wakeOne();
}

void GstVideoWorker::shutdown()
{
    if (needDispatch()) {
        dispatch([this]() { _shutdown = true; });
        (void) QThread::wait(2000);
    } else {
        QThread::quit();
    }
}

void GstVideoWorker::run()
{
    while (!_shutdown) {
        _taskQueueSync.lock();

        while (_taskQueue.isEmpty()) {
            _taskQueueUpdate.wait(&_taskQueueSync);
        }

        const Task task = _taskQueue.dequeue();

        _taskQueueSync.unlock();

        task();
    }
}
