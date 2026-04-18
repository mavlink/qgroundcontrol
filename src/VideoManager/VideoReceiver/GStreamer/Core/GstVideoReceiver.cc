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

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <gst/gst.h>

#include "GStreamerHelpers.h"
#include "GstAppsinkBridge.h"
#include "GstTeeRecorder.h"
#include "QGCLoggingCategory.h"
#include "StreamHealthMonitor.h"
#include "VideoFrameDelivery.h"
#include "VideoSourceResolver.h"

QGC_LOGGING_CATEGORY(GstVideoReceiverLog, "Video.GstVideoReceiver")

GstVideoReceiver::GstVideoReceiver(QObject* parent)
    : VideoReceiver(nullptr)  // no parent — moveToThread requires parentless
{
    Q_UNUSED(parent)  // parent ownership not transferable across threads; caller must manage lifetime

    qCDebug(GstVideoReceiverLog) << this;

    // Run all slots on a dedicated worker thread so GstElement access is
    // serialized without per-call dispatch guards.
    _thread = new QThread();
    _thread->setObjectName(QStringLiteral("GstWorker"));
    moveToThread(_thread);
    _thread->start();

    // Defer timer setup to the worker thread so connections have correct affinity.
    QMetaObject::invokeMethod(this, &GstVideoReceiver::_initTimers, Qt::QueuedConnection);
}

void GstVideoReceiver::_initTimers()
{
    // Runs on _thread — instantiate the monitor here so its timers (which it
    // creates in initTimers()) are affinity-bound to the worker thread.
    _healthMonitor = new StreamHealthMonitor(this);
    _healthMonitor->setDecodingPredicate([this]() { return _decoderActive.load(std::memory_order_relaxed); });
    (void)connect(_healthMonitor, &StreamHealthMonitor::sourceTimeout, this, &GstVideoReceiver::_onHealthTimeout);
    (void)connect(_healthMonitor, &StreamHealthMonitor::decoderTimeout, this, &GstVideoReceiver::_onHealthTimeout);
    _healthMonitor->initTimers();
}

void GstVideoReceiver::_onHealthTimeout(qint64 elapsedSec)
{
    qCDebug(GstVideoReceiverLog) << "Stream timeout, no frames for" << elapsedSec << "s" << _uri;
    // Delegate reconnect/restart decision to VideoStream via timeout() signal.
    // VideoStream::_onReceiverTimeout decides whether to pause/resume or restart.
    emit timeout();
}

void GstVideoReceiver::_setDecoderActive(bool active)
{
    if (_decoderActive == active)
        return;
    _decoderActive = active;
    emit decodingChanged(active);
}

GstVideoReceiver::~GstVideoReceiver()
{
    // Signal all in-flight GLib sync-message callbacks that this object is
    // being destroyed. Checked before every QMetaObject::invokeMethod lambda.
    _destroyed->store(true, std::memory_order_release);

    // stop() must run on the worker thread and complete before we tear down.
    // Use a blocking queued call to drain the thread's event queue.
    if (_thread && _thread->isRunning()) {
        QMetaObject::invokeMethod(this, &GstVideoReceiver::stop, Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(
            this,
            [this]() {
                delete _healthMonitor;
                _healthMonitor = nullptr;
            },
            Qt::BlockingQueuedConnection);
    }

    if (_thread) {
        _thread->quit();
        if (!_thread->wait(5000))
            qCWarning(GstVideoReceiverLog) << "Worker thread did not stop within 5s";
        delete _thread;
        _thread = nullptr;
    }

    qCDebug(GstVideoReceiverLog) << this;
}

VideoReceiver::Capabilities GstVideoReceiver::capabilities() const
{
    static const Capabilities cached = [] {
        Capabilities caps = CapStreaming | CapHWDecode | CapRecording | CapLowLatency | CapRecordingLossless;

        GstNonFloatingPtr<GstElementFactory> factory(gst_element_factory_find("vulkanh264dec"));
        if (factory)
            caps |= CapVulkanDecode;

        return caps;
    }();
    return cached;
}

VideoRecorder* GstVideoReceiver::createRecorder(VideoFrameDelivery* delivery, QObject* parent)
{
    Q_UNUSED(delivery)

    auto* recorder = new GstTeeRecorder(parent);
    recorder->setReceiver(this);
    return recorder;
}

void GstVideoReceiver::onSinkAboutToChange()
{
    _decodingBranch.teardownSink();
}

void GstVideoReceiver::onSinkChanged(QVideoSink* newSink)
{
    if (!newSink) {
        qCDebug(GstVideoReceiverLog) << "Detached video sink";
        return;
    }

    const bool lateAttach = _streaming;
    _decodingBranch.setupSink(_delivery, this);

    if (lateAttach) {
        // The pipeline has been running without a sink (boot-at-RTSP case):
        // rtspsrc buffered RTP packets mid-GOP, and a decoder attached now
        // would stall until the camera's next IDR — potentially 10+ seconds.
        // Ask VideoStream to restart so rtspsrc re-SETUPs and starts on a
        // fresh keyframe.
        qCDebug(GstVideoReceiverLog) << "Late sink attach on live pipeline — requesting restart";
        emit sinkAttachedToLivePipeline();
        return;
    }

    _flushPendingDecoding();
}

void GstVideoReceiver::_flushPendingDecoding()
{
    if (_decodingBranch.isDecodingPending() && _decodingBranch.hasSink()) {
        qCDebug(GstVideoReceiverLog) << "Sink registered — starting deferred decoding";
        _decodingBranch.setDecodingPending(false);
        startDecoding();
    }
}

void GstVideoReceiver::start(uint32_t timeout)
{
    if (_reinvokeIfOffThread([this, timeout]() { start(timeout); }))
        return;

    if (_pipelineController.hasPipeline()) {
        qCDebug(GstVideoReceiverLog) << "Already running!" << _uri;
        return;
    }

    if (_uri.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Failed because URI is not specified";
        emit receiverError(ErrorCategory::Fatal, QStringLiteral("Empty URI"));
        return;
    }

    _timeout = timeout;
    _buffer = lowLatency() ? -1 : 0;

    qCDebug(GstVideoReceiverLog) << "Starting" << _uri << ", lowLatency" << lowLatency() << ", timeout" << _timeout;

    _endOfStream.store(false, std::memory_order_relaxed);

    // NOTE: _decodingBranch._hwDecoderFailed is intentionally NOT reset here.
    // Once a HW decoder has failed on this branch, it stays on the SW path for
    // the rest of the receiver's lifetime. A full receiver recreation (e.g.
    // backend switch, URI change) is required to retry HW. This avoids an
    // infinite fallback loop if HW failure is deterministic.

    _decodingBranch.setState(BranchState::Off);
    _decodingBranch.setPendingStop(nullptr);

    // Prime watchdog for a fresh run — monitor starts from clean state.
    if (_healthMonitor) {
        _healthMonitor->setTimeoutSec(_timeout);
        _healthMonitor->reset();
    }

    if (!_pipelineController.build(VideoSourceResolver::describeUri(_uri),
                                   _buffer,
                                   _decodingBranch.hwDecoderFailed())) {
        // Rate limit restarts via delayed signal — avoids blocking the worker thread
        QTimer::singleShot(1000, this, [this]() { emit receiverError(ErrorCategory::Fatal, QStringLiteral("Pipeline build failed")); });
        return;
    }

    _pipelineController.installTeeProbe(_teeProbe, this);

    // Link source→tee: static src pad links immediately; dynamic pads
    // are handled by the pad-added signal once the source negotiates.
    auto srcPad = _pipelineController.firstSourcePad();
    if (srcPad) {
        _onNewSourcePad(srcPad.get());
    } else {
        (void)g_signal_connect(_pipelineController.source(), "pad-added", G_CALLBACK(_onNewPad), this);
    }

    _pipelineController.connectBus(G_CALLBACK(_onBusMessage), this);

    _pipelineController.dumpGraph("pipeline-initial");
    if (!_pipelineController.setPlaying()) {
        qCCritical(GstVideoReceiverLog) << "gst_element_set_state(PLAYING) failed";
        _pipelineController.setNull();
        _pipelineController.clear();
        QTimer::singleShot(1000, this, [this]() { emit receiverError(ErrorCategory::Fatal, QStringLiteral("Pipeline PLAYING transition failed")); });
        return;
    }

    if (_healthMonitor) {
        _healthMonitor->attach(_pipelineController.pipeline(), &_decodingBranch, _delivery);
    }

    _pipelineController.dumpGraph("pipeline-started");
    qCDebug(GstVideoReceiverLog) << "Started" << _uri;
    emit receiverStarted();
}

void GstVideoReceiver::_clearPipelineElements()
{
    _pipelineController.dumpGraph("pipeline-stopped");

    // Detach the monitor BEFORE unrefing the pipeline — clears its cached
    // jitterbuffer ref so it can't outlive the pipeline it came from.
    if (_healthMonitor) {
        _healthMonitor->detach();
        _healthMonitor->reset();
    }

    _pipelineController.clear();

    _decodingBranch.setState(BranchState::Off);

    if (auto cb = _decodingBranch.takePendingStop())
        cb(STATUS_OK);

    // Belt-and-suspenders: stop() clears _streaming before teardown, but
    // clear here too so a Starting→teardown path without a Streaming phase
    // still lands in a clean `!_streaming` state.
    _streaming = false;
}

void GstVideoReceiver::stop()
{
    if (_reinvokeIfOffThread([this]() { stop(); }))
        return;

    // Detach the monitor unconditionally — stop()'s early-return paths skip
    // _clearPipelineElements, so this prevents the cached jitter ref from
    // outliving the pipeline it came from.
    if (_healthMonitor) {
        _healthMonitor->detach();
    }

    if (_uri.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Stop called on empty URI — nothing to stop";
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping" << _uri;

    _pipelineController.removeTeeProbe();

    if (_pipelineController.hasPipeline()) {
        if (_streaming) {
            qCDebug(GstVideoReceiverLog) << "Streaming stopped" << _uri;
            _streaming = false;
            emit streamingChanged(false);
        } else {
            qCDebug(GstVideoReceiverLog) << "Streaming did not start" << _uri;
        }

        GstNonFloatingPtr<GstBus> bus = _pipelineController.bus();
        if (bus)
            _pipelineController.drainEos(bus.get(), this);
        else
            qCCritical(GstVideoReceiverLog) << "gst_pipeline_get_bus() failed";

        _pipelineController.setNull();

        emit pipelineStopping();  // GstTeeRecorder listens to finalize its branch

        if (_recordingBranch.hasFileSink()) {
            const bool wasStopping = _recordingBranch.state() == BranchState::Stopping;
            _recordingBranch.shutdown(_pipelineController.pipeline());
            if (wasStopping)
                emit recordingBranchEosDrained(true);
        }

        if (_decodingBranch.decoder()) {
            _decodingBranch.shutdown(_pipelineController.pipeline());
            if (_decoderActive)
                _setDecoderActive(false);
        }

        _clearPipelineElements();  // transitions Stopping → Idle

        // Only announce a receiverStopped() primitive when we actually tore
        // down a running pipeline; the empty-URI short-circuit above had no
        // prior receiverStarted(), so no stop transition to signal.
        emit receiverStopped();
    }

    qCDebug(GstVideoReceiverLog) << "Stopped" << _uri;
}

void GstVideoReceiver::startDecoding()
{
    if (_reinvokeIfOffThread([this]() { startDecoding(); }))
        return;

    if (!_decodingBranch.hasSink()) {
        qCDebug(GstVideoReceiverLog) << "No video sink yet — deferring decode start until sink is registered" << _uri;
        _decodingBranch.setDecodingPending(true);
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Starting decoding" << _uri;

    resetBridgeStats();

    if (_decoderActive || _decodingBranch.state() != BranchState::Off) {
        qCDebug(GstVideoReceiverLog) << "Already decoding!" << _uri;
        return;
    }

    _decodingBranch.setState(BranchState::Starting);

    GstNonFloatingPtr<GstPad> pad(gst_element_get_static_pad(_decodingBranch.sinkElement(), "sink"));
    if (!pad) {
        qCCritical(GstVideoReceiverLog) << "Unable to find sink pad of video sink" << _uri;
        _decodingBranch.setState(BranchState::Off);
        return;
    }

    _decodingBranch.resetVideoFrameTime();

    _decodingBranch.setVideoSinkProbeGuard(GstPadProbeGuard(
        pad.get(), gst_pad_add_probe(pad.get(), GST_PAD_PROBE_TYPE_BUFFER, _videoSinkProbe, this, nullptr)));

    if (!_streaming) {
        return;
    }

    if (!_activateDecodingBranch()) {
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Decoding started" << _uri;
}

bool GstVideoReceiver::_activateDecodingBranch()
{
    _decodingBranch.ensureSinkInPipeline(_pipelineController.pipeline());

    if (!_decodingBranch.addDecoder(
            _pipelineController.pipeline(), _pipelineController.decoderValve(), [this](GstPad* p) { _onNewDecoderPad(p); }, G_CALLBACK(_onNewPad),
            this)) {
        qCCritical(GstVideoReceiverLog) << "_addDecoder() failed" << _uri;
        _decodingBranch.shutdown(_pipelineController.pipeline());
        if (_decoderActive)
            _setDecoderActive(false);
        return false;
    }

    g_object_set(_pipelineController.decoderValve(), "drop", FALSE, nullptr);
    _decodingBranch.setState(BranchState::Active);
    // Mark decoding active now that the branch is live — without this,
    // _decoderActive is false until the first frame reaches the sink probe,
    // and stopDecoding() rejects as STATUS_INVALID_STATE in the window before
    // any frame has arrived. _setDecoderActive short-circuits on no-change so the
    // subsequent _noteVideoSinkFrame call is a no-op.
    _setDecoderActive(true);
    return true;
}

void GstVideoReceiver::stopDecoding()
{
    if (_reinvokeIfOffThread([this]() { stopDecoding(); }))
        return;

    qCDebug(GstVideoReceiverLog) << "Stopping decoding" << _uri;

    if (!_pipelineController.hasPipeline() || !_decoderActive ||
        _decodingBranch.state() == BranchState::Stopping) {
        qCDebug(GstVideoReceiverLog) << "Not decoding!" << _uri;
        return;
    }

    g_object_set(_pipelineController.decoderValve(), "drop", TRUE, nullptr);

    _decodingBranch.setState(BranchState::Stopping);

    _decodingBranch.setPendingStop(nullptr);

    if (!_unlinkBranch(_pipelineController.decoderValve())) {
        _completeDecoderStop(STATUS_FAIL);
    }
}

void GstVideoReceiver::pause()
{
    _dispatch([this]() {
        if (_pipelineController.hasPipeline()) {
            qCDebug(GstVideoReceiverLog) << "Pausing pipeline for reconnect" << _uri;
            (void)gst_element_set_state(_pipelineController.pipeline(), GST_STATE_PAUSED);
            // Nudge the watchdog's last-frame timestamp so the next tick doesn't
            // immediately re-fire a timeout just because we stopped feeding it.
            if (_healthMonitor) {
                _healthMonitor->noteSourceFrame();
            }
            emit receiverPaused();
        }
    });
}

void GstVideoReceiver::resume()
{
    _dispatch([this]() {
        if (_pipelineController.hasPipeline()) {
            qCDebug(GstVideoReceiverLog) << "Resuming pipeline" << _uri;
            (void)gst_element_set_state(_pipelineController.pipeline(), GST_STATE_PLAYING);
            if (_healthMonitor) {
                _healthMonitor->noteSourceFrame();
            }
            emit receiverResumed();
        }
    });
}

void GstVideoReceiver::setLowLatency(bool lowLatency)
{
    // Base class stores the value and emits lowLatencyChanged.
    VideoReceiver::setLowLatency(lowLatency);

    // Apply to the running pipeline without a restart.
    // _buffer: -1 means low-latency (pipeline uses 0 ms jitter internally via rtspsrc);
    //  0 means normal (let rtspsrc pick its default ~200 ms).
    _dispatch([this, lowLatency]() {
        _buffer = lowLatency ? -1 : 0;
        if (!isStreaming() || !_pipelineController.source())
            return;

        // Walk the source bin for rtspsrc and rtpjitterbuffer and apply latency.
        // rtspsrc is created with name "source" in GstSourceFactory::createRtspSource.
        GstElement* src = gst_bin_get_by_name(GST_BIN(_pipelineController.source()), "source");
        if (src) {
            if (g_object_class_find_property(G_OBJECT_GET_CLASS(src), "latency")) {
                const guint latMs = lowLatency ? 0u : 200u;
                g_object_set(src, "latency", latMs, nullptr);
                qCDebug(GstVideoReceiverLog) << "setLowLatency: rtspsrc latency ->" << latMs << "ms";
            }
            gst_object_unref(src);
        }

        // rtpjitterbuffer may be inside the source bin (UDP/non-RTSP paths).
        // rtpjitterbuffer is unnamed — iterate the source bin to find it.
        GstIterator* it = gst_bin_iterate_recurse(GST_BIN(_pipelineController.source()));
        GValue val = G_VALUE_INIT;
        while (gst_iterator_next(it, &val) == GST_ITERATOR_OK) {
            GstElement* elem = GST_ELEMENT(g_value_get_object(&val));
            GstElementFactory* factory = gst_element_get_factory(elem);
            if (factory) {
                const gchar* name = gst_element_factory_get_longname(factory);
                if (name && g_str_has_prefix(name, "RTP packet jitter-buffer")) {
                    const guint latMs = lowLatency ? 0u : 200u;
                    g_object_set(elem, "latency", latMs, nullptr);
                    qCDebug(GstVideoReceiverLog) << "setLowLatency: rtpjitterbuffer latency ->" << latMs << "ms";
                }
            }
            g_value_reset(&val);
        }
        g_value_unset(&val);
        gst_iterator_free(it);
    });
}

void GstVideoReceiver::_handleEOS()
{
    if (!_pipelineController.hasPipeline()) {
        return;
    }

    if (_endOfStream.load(std::memory_order_acquire)) {
        // Full pipeline EOS — complete any pending branch stops before tearing down
        if (_decodingBranch.state() == BranchState::Stopping) {
            _decodingBranch.shutdown(_pipelineController.pipeline());
            if (_decoderActive)
                _setDecoderActive(false);
            _completeDecoderStop(STATUS_OK);
        }
        if (_recordingBranch.state() == BranchState::Stopping) {
            _recordingBranch.shutdown(_pipelineController.pipeline());
            emit recordingBranchEosDrained(true);
        }
        stop();
        return;
    }

    // Branch-level EOS — check both (not else-if) in case both are stopping
    if (_decodingBranch.state() == BranchState::Stopping) {
        _decodingBranch.shutdown(_pipelineController.pipeline());
        if (_decoderActive)
            _setDecoderActive(false);
        _completeDecoderStop(STATUS_OK);
    }
    if (_recordingBranch.state() == BranchState::Stopping) {
        _recordingBranch.shutdown(_pipelineController.pipeline());
        emit recordingBranchEosDrained(true);
    }
}

void GstVideoReceiver::_onNewSourcePad(GstPad* pad)
{
    // Only accept video pads — skip audio, subtitles, data, etc.
    GstCapsPtr padCaps(gst_pad_query_caps(pad, nullptr));
    if (padCaps) {
        bool isVideo = false;
        for (guint i = 0; i < gst_caps_get_size(padCaps.get()); ++i) {
            const gchar* name = gst_structure_get_name(gst_caps_get_structure(padCaps.get(), i));
            if (name && (g_str_has_prefix(name, "video/") || g_str_has_prefix(name, "image/"))) {
                isVideo = true;
                break;
            }
        }
        if (!isVideo) {
            qCDebug(GstVideoReceiverLog) << "Skipping non-video pad";
            return;
        }
    }

    // If `_onNewPad` already linked this pad to the tee synchronously (see the
    // anti-race comment there), skip the redundant element_link which would
    // otherwise fail "no compatible unlinked pads" for a tee with one sink.
    if (!gst_pad_is_linked(pad)) {
        if (!gst_element_link(_pipelineController.source(), _pipelineController.tee())) {
            qCCritical(GstVideoReceiverLog) << "Unable to link source";
            return;
        }
    }

    if (!_streaming) {
        qCDebug(GstVideoReceiverLog) << "Streaming started" << _uri;
        _streaming = true;
        emit streamingChanged(true);
    }

    if (!_decodingBranch.hasSink())
        return;

    _pipelineController.dumpGraph("pipeline-with-new-source-pad");

    if (_activateDecodingBranch())
        qCDebug(GstVideoReceiverLog) << "Decoding started" << _uri;
    else
        qCCritical(GstVideoReceiverLog) << "_activateDecodingBranch() failed";
}

void GstVideoReceiver::_onNewDecoderPad(GstPad* pad)
{
    qCDebug(GstVideoReceiverLog) << "_onNewDecoderPad" << _uri;

    _pipelineController.dumpGraph("pipeline-with-new-decoder-pad");

    const QMediaMetaData decoderMeta = _decodingBranch.logSelectedCodec();
    setDecoderInfo(_decodingBranch.isHwDecoding(), _decodingBranch.activeDecoderName(), decoderMeta);

    if (!_decodingBranch.addVideoSink(_pipelineController.pipeline(), pad, _pipelineController.decoderValve())) {
        qCCritical(GstVideoReceiverLog) << "_addVideoSink() failed";
    }
    // VideoStream learns the video size from VideoFrameDelivery::videoSizeChanged
    // when the first decoded frame lands at the appsink — no separate receiver
    // emission needed here.
}

bool GstVideoReceiver::_unlinkBranch(GstElement* from)
{
    // RAII-wrap both pad refs so every early return releases them correctly.
    GstNonFloatingPtr<GstPad> src(gst_element_get_static_pad(from, "src"));
    if (!src) {
        qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
        return false;
    }

    GstNonFloatingPtr<GstPad> sink(gst_pad_get_peer(src.get()));
    if (!sink) {
        qCCritical(GstVideoReceiverLog) << "gst_pad_get_peer() failed";
        return false;
    }

    if (!gst_pad_unlink(src.get(), sink.get())) {
        qCCritical(GstVideoReceiverLog) << "gst_pad_unlink() failed";
        return false;
    }

    // Drop src early — we only need sink for the EOS event below.
    src.reset();

    const gboolean ret = gst_pad_send_event(sink.get(), gst_event_new_eos());

    if (!ret) {
        qCCritical(GstVideoReceiverLog) << "Branch EOS was NOT sent";
        return false;
    }

    qCDebug(GstVideoReceiverLog) << "Branch EOS was sent";

    return true;
}

void GstVideoReceiver::_completeDecoderStop(STATUS status)
{
    if (auto cb = _decodingBranch.takePendingStop())
        cb(status);
}

void GstVideoReceiver::startRecordingBranch(const QString& path, QMediaFormat::FileFormat format)
{
    if (_reinvokeIfOffThread([this, path, format]() { startRecordingBranch(path, format); }))
        return;

    if (!_pipelineController.hasPipeline() || !_pipelineController.recorderValve() || !_pipelineController.tee()) {
        qCWarning(GstVideoReceiverLog) << "startRecordingBranch: pipeline elements not available";
        emit receiverError(ErrorCategory::Fatal, QStringLiteral("Recording pipeline unavailable"));
        emit recordingBranchEosDrained(false);
        return;
    }

    if (_recordingBranch.state() != BranchState::Off) {
        qCWarning(GstVideoReceiverLog) << "startRecordingBranch: branch already active";
        emit receiverError(ErrorCategory::Fatal, QStringLiteral("Recording branch already active"));
        emit recordingBranchEosDrained(false);
        return;
    }

    const STATUS status = _recordingBranch.start(
        _pipelineController.pipeline(), _pipelineController.recorderValve(), _pipelineController.tee(), path, format,
        [this](const QString& startedPath) { emit recordingBranchStarted(startedPath); });

    if (status != STATUS_OK) {
        emit receiverError(ErrorCategory::Fatal, QStringLiteral("Recording branch start failed"));
        emit recordingBranchEosDrained(false);
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Recording branch started";
}

void GstVideoReceiver::stopRecordingBranch()
{
    if (_reinvokeIfOffThread([this]() { stopRecordingBranch(); }))
        return;

    if (!_pipelineController.hasPipeline() || !_pipelineController.recorderValve()) {
        qCWarning(GstVideoReceiverLog) << "stopRecordingBranch: pipeline or valve not available";
        emit recordingBranchEosDrained(false);
        return;
    }

    if (_recordingBranch.state() == BranchState::Off) {
        qCDebug(GstVideoReceiverLog) << "stopRecordingBranch: no active branch";
        emit recordingBranchEosDrained(true);
        return;
    }

    const STATUS status =
        _recordingBranch.stop(_pipelineController.recorderValve(), [this](GstElement* from) { return _unlinkBranch(from); });

    if (status != STATUS_OK) {
        qCCritical(GstVideoReceiverLog) << "stopRecordingBranch: failed to send branch EOS";
        _recordingBranch.shutdown(_pipelineController.pipeline());
        emit recordingBranchEosDrained(false);
        return;
    }

    // Successful EOS send: completion arrives via _handleEOS → recordingBranchEosDrained.
    qCDebug(GstVideoReceiverLog) << "Recording branch EOS sent";
}

bool GstVideoReceiver::_needDispatch() const
{
    return QThread::currentThread() != _thread;
}

bool GstVideoReceiver::_tryDecoderFallback(GstMessage* msg)
{
    const auto result = _fallbackController.evaluate(msg, _decodingBranch);
    if (!result.shouldRetry)
        return false;

    // evaluate() has already marked `_decodingBranch._hwDecoderFailed = true`.
    // The flag is branch-scoped; when the pipeline restarts below, the new
    // decodebin3/uridecodebin3 will pick up `force-sw-decoders=TRUE` from
    // GstDecodingBranch::addDecoder / GstPipelineFactory::build — no global
    // registry-rank mutation, so sibling streams are unaffected.
    qCWarning(GstVideoReceiverLog) << "HW decoder" << result.decoderName
                                   << "failed mid-stream — restarting with scoped SW fallback";

    _dispatch([this]() {
        qCDebug(GstVideoReceiverLog) << "Restarting pipeline (scoped SW fallback)";
        stop();
        QTimer::singleShot(100, this, [this]() { _dispatch([this]() { start(_timeout); }); });
    });

    return true;
}
