#include "GstReceiverWorker.h"

#include <chrono>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include "GstSourceFactory.h"
#include "GstVideoReceiver.h"
#include "QGCNetworkHelper.h"
#include "gstqgc/GstQgcSinkStats.h"

namespace {

constexpr std::chrono::milliseconds kWatchdogInterval{1000};
// The live GPU path counters are logged every this many watchdog ticks.
constexpr int kHwStatsLogIntervalTicks = 10;

}  // namespace

GstReceiverWorker::GstReceiverWorker(QObject* parent)
    : QObject(parent)
    , _watchdogTimer(new QTimer(this))
    , _supervisor(new StreamSupervisor(this))
    , _statsSampler(*_supervisor)
{
    _watchdogTimer->setInterval(kWatchdogInterval);
    (void) connect(_watchdogTimer, &QTimer::timeout, this, [this]() { _onWatchdogTick(); });
    (void) connect(_supervisor, &StreamSupervisor::stateChanged, this,
                   [this](VideoReceiver::State state) { _updateStatus(&Status::state, state); });
    (void) connect(_supervisor, &StreamSupervisor::reconnectDue, this, [this]() {
        qCDebug(GstVideoReceiverLog) << "Reconnecting (attempt" << _supervisor->reconnectAttempts() << ")"
                                     << _redactedUri();
        _reconcile();
    });
}

GstReceiverWorker::~GstReceiverWorker()
{
    _stopSession();
}

void GstReceiverWorker::apply(const Request& request)
{
    const Request previous = std::exchange(_requested, request);

    // A start, stop or restart takes the config and the video sink as they are now.
    if (request.active != previous.active) {
        _onActiveChanged();
    } else if (request.restarts != previous.restarts) {
        if (_requested.active) {
            _restartStream();
        }
    } else {
        if (request.config != previous.config) {
            _onConfigChanged(previous.config);
        }
        if ((request.videoSink != previous.videoSink) && _session) {
            _session->setVideoSink(_requested.videoSink.get());
        }
    }

    const quint64 recordingId = request.recording ? request.recording->id : 0;
    const quint64 previousRecordingId = previous.recording ? previous.recording->id : 0;
    if (recordingId != previousRecordingId) {
        if (request.recording) {
            _startRecording(*request.recording);
        } else {
            _stopRecording();
        }
    }
}

void GstReceiverWorker::_onActiveChanged()
{
    _supervisor->startOver();
    if (_requested.active) {
        _dotDumper.setReceiverName(_requested.name);
    } else {
        _endRecording();
    }
    _reconcile();
}

void GstReceiverWorker::_onConfigChanged(const VideoReceiver::Config& previous)
{
    if (!_requested.active) {
        return;
    }
    const VideoReceiver::Config& config = _requested.config;
    VideoReceiver::Config sameStream = previous;
    sameStream.autoReconnect = config.autoReconnect;
    if (sameStream != config) {
        _restartStream();
        return;
    }

    // Only autoReconnect changed, which applies at once.
    switch (_supervisor->autoReconnectChanged(config.autoReconnect)) {
        case StreamSupervisor::AutoReconnectChange::Halted:
            _onHalted(_supervisor->lastFailure());
            break;
        case StreamSupervisor::AutoReconnectChange::Resumable:
            _reconcile();
            break;
        case StreamSupervisor::AutoReconnectChange::None:
            break;
    }
}

void GstReceiverWorker::_startRecording(const RecordingRequest& request)
{
    if (!_requested.active) {
        return;
    }
    if (_supervisor->halted()) {
        const QString message =
            QCoreApplication::translate("GstVideoReceiver", "Unable to record video: the video stream has stopped.");
        emit errorOccurred(message);
        return;
    }

    _recording.emplace(request.file, request.format);
    _updateStatus(&Status::recording, true);
    if (_session) {
        _session->setRecording(_recordingRequest());
    }
}

void GstReceiverWorker::_stopRecording()
{
    if (!_recording) {
        return;
    }
    _endRecording();
    if (_session) {
        _session->setRecording(std::nullopt);
    }
}

void GstReceiverWorker::dumpPipelineGraph(const QString& tag) const
{
    if (!_session) {
        qCDebug(GstVideoReceiverLog) << "No running pipeline to dump";
        return;
    }
    _session->snapshot(tag);
}

void GstReceiverWorker::shutdown()
{
    _requested.active = false;
    _stopSession();
    _supervisor->stop();
    thread()->quit();
}

void GstReceiverWorker::_reconcile()
{
    if (!_requested.active || _requested.config.uri.isEmpty()) {
        _stopSession();
        _supervisor->stop();
        return;
    }
    if (_session || _supervisor->halted() || _supervisor->reconnectPending()) {
        return;
    }
    _startSession();
}

void GstReceiverWorker::_restartStream()
{
    _supervisor->startOver();
    _stopSession();
    _reconcile();
}

void GstReceiverWorker::_startSession()
{
    const PipelineSession::Settings settings{
        .uri = _requested.config.uri,
        .redactedUri = _redactedUri(),
        .lowLatency = _requested.config.lowLatency,
        .rtpJitterLatencyMs = _requested.config.rtpJitterLatencyMs,
    };
    _session = new PipelineSession(settings, *this, _watchdog, _dotDumper, this);
    _watchdog.start(_requested.config.timeout, GStreamer::SourceFactory::connectGracePeriod(_requested.config.uri));

    if (!_session->start()) {
        const PipelineSession::Failure failure = _session->failure();
        _stopSession();
        _recover(failure);
        return;
    }

    _supervisor->sessionStarted();
    _session->setVideoSink(_requested.videoSink.get());
    _session->setRecording(_recordingRequest());
    _statsTicks = 0;
    _watchdogTimer->start();
}

void GstReceiverWorker::_stopSession()
{
    if (!_session) {
        return;
    }
    _watchdogTimer->stop();
    if (_statsSampler.reset()) {
        _updateStatus(&Status::stats, _statsSampler.stats());
    }
    PipelineSession* const session = std::exchange(_session, nullptr);
    session->shutdown();
    // Deleted from the event loop: this may run inside one of its calls. Its leftover work is dropped with it.
    session->deleteLater();

    if (_recording) {
        _recording->onSessionEnded();
    }
}

void GstReceiverWorker::_recover(const PipelineSession::Failure& failure)
{
    const StreamSupervisor::FailureOutcome outcome =
        _supervisor->sessionFailed(failure.message, failure.permanent, _requested.config.autoReconnect);
    if (outcome.halted) {
        _onHalted(failure.message);
        return;
    }
    qCDebug(GstVideoReceiverLog) << "Scheduling reconnect #" << _supervisor->reconnectAttempts() << "in"
                                 << outcome.reconnectDelay.count() << "ms after" << failure.reason << _redactedUri();
}

void GstReceiverWorker::_onHalted(const QString& message)
{
    qCWarning(GstVideoReceiverLog) << "Video stream stopped:" << message;
    _endRecording();
    emit errorOccurred(message);
}

void GstReceiverWorker::_endRecording()
{
    if (!_recording) {
        return;
    }
    _recording.reset();
    _updateStatus(&Status::recording, false);
}

std::optional<RecordingBranch::Request> GstReceiverWorker::_recordingRequest() const
{
    if (!_recording) {
        return std::nullopt;
    }
    return RecordingBranch::Request{.file = _recording->currentFile(), .format = _recording->format()};
}

void GstReceiverWorker::_onWatchdogTick()
{
    if (!_session) {
        return;
    }

    if (++_statsTicks >= kHwStatsLogIntervalTicks) {
        _statsTicks = 0;
        if (const GstQgc::PathStats hwStats = _session->sinkPathStats(); hwStats.totalDelivered > 0) {
            qCDebug(GstVideoReceiverLog).noquote() << "GPU path live" << _redactedUri() << hwStats.line;
        }
    }
    _sampleStats();

    const StreamWatchdog::Result result = _watchdog.check(StreamWatchdog::now(), _session->displaying());
    switch (result.stall) {
        case StreamWatchdog::Stall::None:
            return;
        case StreamWatchdog::Stall::Decoder:
            qCDebug(GstVideoReceiverLog) << "Video decoder timeout, no frames for" << result.silentMs << "ms"
                                         << _redactedUri();
            _session->snapshot(QStringLiteral("pipeline-decoder-timeout"));
            _session->restartDecoder("decoder watchdog");
            return;
        case StreamWatchdog::Stall::Source:
            qCDebug(GstVideoReceiverLog) << "Stream timeout, no frames for" << result.silentMs << "ms"
                                         << _redactedUri();
            _session->snapshot(QStringLiteral("pipeline-watchdog-timeout"));
            _stopSession();
            _recover({.reason = QStringLiteral("source watchdog"),
                      .message = QCoreApplication::translate("GstVideoReceiver", "No video received for %1 s.")
                                     .arg(result.silentMs / 1000),
                      .permanent = false});
            return;
    }
}

void GstReceiverWorker::_sampleStats()
{
    // Rates from before the first frame would only measure the connection's setup.
    if (_supervisor->phase() != StreamSupervisor::Phase::Streaming) {
        return;
    }
    VideoStreamStatsMeter::Counters counters;
    VideoStreamStats stats = _statsSampler.stats();
    _session->sampleStats(counters, stats);
    if (_statsSampler.sample(counters, stats, _recording ? &*_recording : nullptr)) {
        _updateStatus(&Status::stats, _statsSampler.stats());
    }
}

QString GstReceiverWorker::_redactedUri() const
{
    return QGCNetworkHelper::redactedUrlForLogging(_requested.config.uri);
}

void GstReceiverWorker::onSessionStreaming()
{
    _supervisor->sessionStreaming();
}

void GstReceiverWorker::onSessionFailed(const PipelineSession::Failure& failure)
{
    qCDebug(GstVideoReceiverLog) << "Stream failed:" << failure.reason << _redactedUri();
    _stopSession();
    _recover(failure);
}

void GstReceiverWorker::onDecodingChanged(bool decoding)
{
    _updateStatus(&Status::decoding, decoding);
}

void GstReceiverWorker::onDecoderNameChanged(const QString& name)
{
    _updateStatus(&Status::decoderName, name);
}

void GstReceiverWorker::onDecoderFailing()
{
    onSessionFailed({.reason = QStringLiteral("decoder failing"),
                     .message = QCoreApplication::translate("GstVideoReceiver", "The video decoder keeps failing."),
                     .permanent = false});
}

void GstReceiverWorker::onRecordingFileOpened(const QString& file)
{
    if (_recording) {
        (void) _recording->onFileOpened(file);
    }
    emit recordingStarted(file);
}

void GstReceiverWorker::onRecordingFileClosed(const QString& file, const QString& error)
{
    emit recordingFinished(file, error);
}

void GstReceiverWorker::onRecordingFailed(const QString& file, const QString& error, bool fileOpened)
{
    if (_recording && (file == _recording->currentFile())) {
        _endRecording();
    }
    // A file that opened already reported the error through recordingFinished().
    if (!fileOpened) {
        const QString message =
            QCoreApplication::translate("GstVideoReceiver", "Video recording failed: %1").arg(error);
        emit errorOccurred(message);
    }
}
