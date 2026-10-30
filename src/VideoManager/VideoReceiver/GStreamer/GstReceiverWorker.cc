#include "GstReceiverWorker.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include "GstSourceFactory.h"
#include "QGCNetworkHelper.h"
#include "gstqgc/gstqgcqvideosink.h"

namespace {

constexpr std::chrono::milliseconds kWatchdogInterval{1000};
// The live GPU path counters are logged every this many watchdog ticks.
constexpr int kHwStatsLogIntervalTicks = 10;

/// "<base>_part<part>.<ext>" for @p file "<base>.<ext>".
QString partFile(const QString& file, int part)
{
    const QString suffix = QStringLiteral("_part%1").arg(part);
    const qsizetype dot = file.lastIndexOf(u'.');
    const qsizetype separator = std::max(file.lastIndexOf(u'/'), file.lastIndexOf(u'\\'));
    return (dot > separator) ? (file.left(dot) + suffix + file.mid(dot)) : (file + suffix);
}

}  // namespace

GstReceiverWorker::GstReceiverWorker(QObject* parent)
    : QObject(parent)
    , _watchdogTimer(new QTimer(this))
    , _reconnectTimer(new QTimer(this))
{
    _watchdogTimer->setInterval(kWatchdogInterval);
    _reconnectTimer->setSingleShot(true);
    // Backoff delays are asserted by tests; a coarse timer may fire up to 5% early.
    _reconnectTimer->setTimerType(Qt::PreciseTimer);
    (void) connect(_watchdogTimer, &QTimer::timeout, this, [this]() { _onWatchdogTick(); });
    (void) connect(_reconnectTimer, &QTimer::timeout, this, [this]() {
        qCDebug(GstVideoReceiverLog) << "Reconnecting (attempt" << _watchdog.reconnectAttempts() << ")"
                                     << _redactedUri();
        _reconcile();
    });
}

GstReceiverWorker::~GstReceiverWorker()
{
    _stopSession();
}

QString GstReceiverWorker::Recording::currentFile() const
{
    return (part == 1) ? file : partFile(file, part);
}

void GstReceiverWorker::setActive(bool active, const QString& name, const VideoReceiver::Config& config)
{
    _active = active;
    _config = config;
    _halted = false;
    _reconnectTimer->stop();
    _watchdog.resetBackoff();
    if (active) {
        _dotDumper.setReceiverName(name);
    } else {
        _endRecording();
    }
    _reconcile();
}

void GstReceiverWorker::setConfig(const VideoReceiver::Config& config)
{
    VideoReceiver::Config sameStream = std::exchange(_config, config);
    sameStream.autoReconnect = config.autoReconnect;
    if (!_active) {
        return;
    }

    if (sameStream != config) {
        _restartStream();
        return;
    }

    // Only autoReconnect changed, which applies at once.
    if (!config.autoReconnect && _reconnectTimer->isActive()) {
        _halt(_lastFailure, false);
    } else if (config.autoReconnect && _halted && !_haltedPermanently) {
        _halted = false;
        _reconcile();
    }
}

void GstReceiverWorker::restart(const std::shared_ptr<GstElement>& videoSink)
{
    _videoSink = videoSink;
    if (_active) {
        _restartStream();
    }
}

void GstReceiverWorker::setVideoSink(const std::shared_ptr<GstElement>& videoSink)
{
    _videoSink = videoSink;
    if (_session) {
        _session->setVideoSink(_videoSink.get());
    }
}

void GstReceiverWorker::startRecording(const QString& file, VideoReceiver::FileFormat format)
{
    if (!_active) {
        return;
    }
    if (_halted) {
        const QString message =
            QCoreApplication::translate("GstVideoReceiver", "Unable to record video: the video stream has stopped.");
        emit errorOccurred(message);
        return;
    }

    _recording = Recording{.file = file, .format = format};
    emit recordingChanged(true);
    if (_session) {
        _session->setRecording(_recordingRequest());
    }
}

void GstReceiverWorker::stopRecording()
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
    _active = false;
    _reconnectTimer->stop();
    _stopSession();
    thread()->quit();
}

void GstReceiverWorker::_reconcile()
{
    if (!_active || _config.uri.isEmpty()) {
        _reconnectTimer->stop();
        _stopSession();
        _setState(VideoReceiver::State::Stopped);
        return;
    }
    if (_session || _halted || _reconnectTimer->isActive()) {
        return;
    }
    _startSession();
}

void GstReceiverWorker::_restartStream()
{
    _reconnectTimer->stop();
    _halted = false;
    _watchdog.resetBackoff();
    _stopSession();
    _reconcile();
}

void GstReceiverWorker::_startSession()
{
    const PipelineSession::Settings settings{
        .uri = _config.uri,
        .redactedUri = _redactedUri(),
        .lowLatency = _config.lowLatency,
        .rtpJitterLatencyMs = _config.rtpJitterLatencyMs,
    };
    _session = new PipelineSession(settings, *this, _watchdog, _dotDumper, this);
    ++_sessionSerial;
    _watchdog.start(_config.timeout, GStreamer::SourceFactory::connectGracePeriod(_config.uri));

    if (!_session->start()) {
        const PipelineSession::Failure failure = _session->failure();
        _stopSession();
        _recover(failure);
        return;
    }

    _setState(VideoReceiver::State::Connecting);
    _session->setVideoSink(_videoSink.get());
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
    PipelineSession* const session = std::exchange(_session, nullptr);
    session->shutdown();
    // Deleted from the event loop: this may run inside one of its calls. Its leftover work is dropped with it.
    session->deleteLater();

    if (_recording && _recording->opened) {
        ++_recording->part;
        _recording->opened = false;
    }
}

void GstReceiverWorker::_recover(const PipelineSession::Failure& failure)
{
    _lastFailure = failure.message;
    if (failure.permanent || !_config.autoReconnect) {
        _halt(failure.message, failure.permanent);
        return;
    }

    const std::chrono::seconds delay = _watchdog.nextReconnectDelay();
    qCDebug(GstVideoReceiverLog) << "Scheduling reconnect #" << _watchdog.reconnectAttempts() << "in" << delay.count()
                                 << "s after" << failure.reason << _redactedUri();
    _reconnectTimer->start(delay);
    _setState(VideoReceiver::State::Reconnecting);
}

void GstReceiverWorker::_halt(const QString& message, bool permanent)
{
    _halted = true;
    _haltedPermanently = permanent;
    _reconnectTimer->stop();
    qCWarning(GstVideoReceiverLog) << "Video stream stopped:" << message;
    _endRecording();
    _setState(VideoReceiver::State::Stopped);
    emit errorOccurred(message);
}

void GstReceiverWorker::_endRecording()
{
    if (!_recording) {
        return;
    }
    _recording.reset();
    emit recordingChanged(false);
}

std::optional<RecordingBranch::Request> GstReceiverWorker::_recordingRequest() const
{
    if (!_recording) {
        return std::nullopt;
    }
    return RecordingBranch::Request{.file = _recording->currentFile(), .format = _recording->format};
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

void GstReceiverWorker::_setState(VideoReceiver::State state)
{
    if (state == _state) {
        return;
    }
    _state = state;
    emit stateChanged(state);
}

QString GstReceiverWorker::_redactedUri() const
{
    return QGCNetworkHelper::redactedUrlForLogging(_config.uri);
}

void GstReceiverWorker::onSessionStreaming()
{
    _setState(VideoReceiver::State::Streaming);
}

void GstReceiverWorker::onSessionFailed(const PipelineSession::Failure& failure)
{
    qCDebug(GstVideoReceiverLog) << "Stream failed:" << failure.reason << _redactedUri();
    // Later, from the event loop: the session may be in the middle of a call.
    const quint64 serial = _sessionSerial;
    (void) QMetaObject::invokeMethod(
        this,
        [this, serial, failure]() {
            if (_session && (serial == _sessionSerial)) {
                _stopSession();
                _recover(failure);
            }
        },
        Qt::QueuedConnection);
}

void GstReceiverWorker::onDecodingChanged(bool decoding)
{
    emit decodingChanged(decoding);
}

void GstReceiverWorker::onDecoderNameChanged(const QString& name)
{
    emit decoderNameChanged(name);
}

void GstReceiverWorker::onDecoderFailing()
{
    onSessionFailed({.reason = QStringLiteral("decoder failing"),
                     .message = QCoreApplication::translate("GstVideoReceiver", "The video decoder keeps failing."),
                     .permanent = false});
}

void GstReceiverWorker::onRecordingFileOpened(const QString& file)
{
    if (_recording && (file == _recording->currentFile())) {
        _recording->opened = true;
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
