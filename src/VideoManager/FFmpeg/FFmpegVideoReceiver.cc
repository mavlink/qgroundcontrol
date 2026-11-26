#include "FFmpegVideoReceiver.h"

#include <chrono>
#include <utility>

#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>
#include <QtCore/QUrl>
#include <QtMultimedia/QVideoSink>

#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "VideoStreamStats.h"
#include "VideoStreamStatsMeter.h"
#include "VideoStreamUri.h"

QGC_LOGGING_CATEGORY(FFmpegVideoReceiverLog, "Video.FFmpegVideoReceiver")

namespace {

constexpr int kFFmpegSampleIntervalMs = 1000;
/// Before a session's first frame, on top of the timeout: decoding starts at the stream's next keyframe, which a camera
/// may send seconds apart.
constexpr qint64 kFFmpegFirstFrameGraceMs = 5000;

qint64 ffmpegMilliseconds(std::chrono::seconds duration)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

}  // namespace

FFmpegVideoReceiver::FFmpegVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
    , _supervisor(new StreamSupervisor(this))
    , _recorder(std::make_unique<FFmpegStreamRecorder>(static_cast<FFmpegStreamRecorder::Listener&>(*this)))
    , _statsSampler(*_supervisor)
{
    (void) connect(_supervisor, &StreamSupervisor::stateChanged, this, &FFmpegVideoReceiver::setState);
    (void) connect(_supervisor, &StreamSupervisor::reconnectDue, this, &FFmpegVideoReceiver::_start);
    _sampleTimer.setInterval(kFFmpegSampleIntervalMs);
    (void) connect(&_sampleTimer, &QTimer::timeout, this, &FFmpegVideoReceiver::_sample);
}

FFmpegVideoReceiver::~FFmpegVideoReceiver()
{
    _sampleTimer.stop();
    if (_session) {
        _session->requestStop();
    }
    for (const std::unique_ptr<FFmpegStreamSession>& session : _endingSessions) {
        session->requestStop();
    }
    // Each waits for its thread. What they post meanwhile is dropped with this object.
    _session.reset();
    _endingSessions.clear();
    // Finishes the open file, which the application's exit would cut short.
    _recorder.reset();
}

void FFmpegVideoReceiver::onConfigChanged()
{
    // autoReconnect alone applies at once: a reconnect it no longer allows is dropped.
    if ((_session || _supervisor->reconnectPending()) && config().sameStreamAs(_sessionConfig)) {
        if (_supervisor->autoReconnectChanged(config().autoReconnect) ==
            StreamSupervisor::AutoReconnectChange::Halted) {
            _onHalted(_supervisor->lastFailure());
        }
        return;
    }
    _startOver();
}

void FFmpegVideoReceiver::onActiveChanged()
{
    if (!active()) {
        _endRecording();
    }
    _startOver();
}

void FFmpegVideoReceiver::onRestartRequested()
{
    _startOver();
}

void FFmpegVideoReceiver::onReconnectRequested()
{
    _supervisor->reconnectNow();
}

void FFmpegVideoReceiver::onVideoOutputChanged()
{
    const bool displaying = videoSink() != nullptr;
    if (_session) {
        _session->setDecoding(displaying);
    }
    if (!displaying) {
        QVideoFrame dropped;
        {
            const QMutexLocker locker(&_frameMutex);
            dropped = std::exchange(_pendingFrame, QVideoFrame());
        }
        setDecoding(false);
    }
    // The watchdog counts from now: no frame reached an output while there was none.
    _sinceFrame.start();
}

void FFmpegVideoReceiver::onStartRecording(const QString& file, FileFormat format,
                                           [[maybe_unused]] const RecordingEncoding& encoding)
{
    if (_supervisor->halted()) {
        emit errorOccurred(tr("Unable to record video: the video stream has stopped."));
        return;
    }
    qCDebug(FFmpegVideoReceiverLog) << name() << "recording to" << file;
    _recording.emplace(file, format);
    setRecording(true);
    _updateRecorder();
}

void FFmpegVideoReceiver::onStopRecording()
{
    _endRecording();
}

void FFmpegVideoReceiver::_updateRecorder()
{
    if (_recording) {
        _recorder->record(_recording->currentFile(), _recording->format(), _session ? _session->id() : 0);
    }
}

void FFmpegVideoReceiver::_endRecording()
{
    if (!_recording) {
        return;
    }
    _recording.reset();
    _recorder->stop();
    setRecording(false);
}

void FFmpegVideoReceiver::_startOver()
{
    _supervisor->startOver();
    _decoderFailureReported = false;
    _start();
}

void FFmpegVideoReceiver::_start()
{
    _stopSession();
    _openPending = false;
    if (!active() || config().uri.isEmpty()) {
        _supervisor->stop();
        return;
    }
    if (!VideoStreamUri::ffmpegSource(config().uri)) {
        qCWarning(FFmpegVideoReceiverLog) << name() << "can't play" << _redactedUri();
        const QString message = tr("Unable to play the video stream %1.").arg(_redactedUri());
        (void) _supervisor->sessionFailed(message, StreamSupervisor::Failure::Permanent, config().autoReconnect);
        _onHalted(message);
        return;
    }
    _supervisor->sessionStarted();
    _openPending = true;
    if (_endingSessions.empty()) {
        _open();
    }
}

void FFmpegVideoReceiver::_open()
{
    _openPending = false;
    const std::optional<VideoStreamUri::FFmpegSource> source = VideoStreamUri::ffmpegSource(config().uri);
    if (_session || !active() || !source) {
        return;
    }

    _sessionConfig = config();
    const FFmpegStreamSession::Settings settings{
        .source = *source,
        .redactedUri = _redactedUri(),
        .networkTimeout = _sessionConfig.timeout,
        .lowLatency = _sessionConfig.lowLatency,
        .rtpJitterLatencyMs = _sessionConfig.rtpJitterLatencyMs,
        .softwareDecoderThreads = _sessionConfig.softwareDecoderThreads,
        // Off when the software decoder or the CPU video path is forced: frames decoded in software are in memory.
        .hardwareDecoding = displayConfig().gpuZeroCopy,
        .squarePixels = !displayConfig().disablePixelAspectRatio,
    };
    FFmpegStreamSession::Listener& listener = *this;
    _session = std::make_unique<FFmpegStreamSession>(++_lastSessionId, settings, listener, *_recorder);
    _session->setDecoding(videoSink() != nullptr);
    _updateRecorder();
    qCDebug(FFmpegVideoReceiverLog) << name() << "playing" << _redactedUri();
    _sinceOpened.start();
    _sinceFrame.start();
    _sampleTimer.start();
    _session->start();
}

void FFmpegVideoReceiver::_stopSession()
{
    _sampleTimer.stop();
    if (_session) {
        _session->requestStop();
        _endingSessions.push_back(std::move(_session));
        // The part finishes with its session; the next session records to the next part, if this one got any.
        if (_recording) {
            _recording->nextPart();
            _updateRecorder();
        }
    }
    QVideoFrame dropped;
    {
        const QMutexLocker locker(&_frameMutex);
        dropped = std::exchange(_pendingFrame, QVideoFrame());
        _framesDropped = 0;
    }
    (void) _statsSampler.reset();
    _framesShown = 0;
    _decoderInfo = {};
    _codec.clear();
    _sessionUndecodable = false;
    setDecoding(false);
    setStats(_statsSampler.stats());
}

QString FFmpegVideoReceiver::_redactedUri() const
{
    return QGCNetworkHelper::redactedUrlForLogging(config().uri);
}

bool FFmpegVideoReceiver::_awaitingSender() const
{
    const std::optional<VideoStreamUri::Transport> transport = VideoStreamUri::transportFor(QUrl(_sessionConfig.uri));
    // A session that ends sooner than its timeout didn't run out of patience: its port is taken, or it can't play.
    return transport && VideoStreamUri::listens(*transport) &&
           (_supervisor->phase() == StreamSupervisor::Phase::Connecting) &&
           (_sinceOpened.durationElapsed() >= _sessionConfig.timeout);
}

void FFmpegVideoReceiver::_fail(const QString& reason, bool permanent)
{
    if (!_session) {
        return;
    }
    if (!permanent && _awaitingSender()) {
        qCDebug(FFmpegVideoReceiverLog) << name() << "no stream yet, listening on:" << reason;
        _start();
        return;
    }
    _stopSession();
    const QString message = tr("Video stream failed: %1").arg(reason);
    const StreamSupervisor::FailureOutcome outcome = _supervisor->sessionFailed(
        message, permanent ? StreamSupervisor::Failure::Permanent : StreamSupervisor::Failure::Transient,
        config().autoReconnect);
    if (outcome.halted) {
        qCDebug(FFmpegVideoReceiverLog) << name() << "failed:" << reason;
        _onHalted(message);
    } else {
        qCDebug(FFmpegVideoReceiverLog) << name() << "failed, reconnecting in" << outcome.reconnectDelay << ":"
                                        << reason;
    }
}

void FFmpegVideoReceiver::_onHalted(const QString& message)
{
    _endRecording();
    emit errorOccurred(message);
}

void FFmpegVideoReceiver::_showFrame()
{
    QVideoFrame frame;
    quint64 session = 0;
    {
        const QMutexLocker locker(&_frameMutex);
        frame = std::exchange(_pendingFrame, QVideoFrame());
        session = _pendingFrameSession;
        _frameShowPosted = false;
    }
    QVideoSink* const sink = videoSink();
    if (!frame.isValid() || !_isCurrent(session) || !sink) {
        return;
    }
    sink->setVideoFrame(frame);
    ++_framesShown;
    _sinceFrame.restart();
    setVideoSize(presentedSize(frame));
    setDecoding(true);
}

void FFmpegVideoReceiver::_sample()
{
    if (!_session) {
        return;
    }
    const FFmpegStreamSession::Counters counters = _session->counters();
    const qint64 timeoutMs = ffmpegMilliseconds(_sessionConfig.timeout);
    const std::optional<VideoStreamUri::Transport> transport = _session->settings().source.transport;
    if (counters.packets == 0) {
        // A UDP port waits for its sender for as long as it takes; a server that sends nothing has failed.
        const bool listens = transport && VideoStreamUri::listens(*transport);
        if (!listens && (_sinceOpened.elapsed() > (timeoutMs + kFFmpegFirstFrameGraceMs))) {
            _fail(tr("no video received for %1 s").arg(_sinceOpened.elapsed() / 1000));
        }
        return;
    }
    const qint64 silentMs = FFmpegStreamSession::nowMs() - counters.lastPacketMs;
    if (silentMs > timeoutMs) {
        _fail(tr("no video received for %1 s").arg(silentMs / 1000));
        return;
    }
    const qint64 frameTimeoutMs = timeoutMs + ((_framesShown == 0) ? kFFmpegFirstFrameGraceMs : 0);
    if (videoSink() && !_sessionUndecodable && (_sinceFrame.elapsed() > frameTimeoutMs)) {
        _fail(tr("no picture for %1 s").arg(_sinceFrame.elapsed() / 1000));
        return;
    }
    if (_recording && _recording->partDue()) {
        _recording->nextPart();
        qCDebug(FFmpegVideoReceiverLog) << name() << "recording continues in" << _recording->currentFile();
        _updateRecorder();
    }
    // Rates from before the stream arrived would only measure the connection's setup.
    if (_supervisor->phase() != StreamSupervisor::Phase::Streaming) {
        return;
    }

    VideoStreamStats stats = _statsSampler.stats();
    stats.decoder = _decoderInfo.name;
    stats.hardwareDecoder = _decoderInfo.hardware;
    // Qt decides as it renders whether a frame in GPU memory is shown from there or copied first.
    stats.gpuPath = counters.hardwareFrames ? QStringLiteral("GPU") : QStringLiteral("CPU");
    stats.codec = _codec;
    stats.keyframeIntervalMs = counters.keyframeIntervalMs;
    // FFmpeg counts RTP packets, their losses and jitter in its private RTP state only (RTPDemuxContext), so the RTP
    // fields stay unknown; and there is no jitter buffer whose latency would tell the playout delay.
    VideoStreamStatsMeter::Counters meterCounters;
    meterCounters.framesDisplayed = _framesShown;
    meterCounters.bytesReceived = counters.bytes;
    {
        const QMutexLocker locker(&_frameMutex);
        meterCounters.framesDropped = _framesDropped;
    }
    if (_statsSampler.sample(meterCounters, stats, _recording ? &*_recording : nullptr)) {
        setStats(_statsSampler.stats());
    }
}

void FFmpegVideoReceiver::_onSessionEnded(quint64 id, const std::optional<FFmpegStreamSession::Failure>& failure)
{
    if (_isCurrent(id)) {
        _fail(failure ? failure->message : tr("the stream ended"), failure && failure->permanent);
    }
    for (auto it = _endingSessions.begin(); it != _endingSessions.end(); ++it) {
        if ((*it)->id() == id) {
            // Its thread has returned from its last call, or is about to: this waits for it.
            (void) _endingSessions.erase(it);
            break;
        }
    }
    if (_openPending && _endingSessions.empty()) {
        _open();
    }
}

void FFmpegVideoReceiver::onSessionStreaming(quint64 session, const QString& codec)
{
    QMetaObject::invokeMethod(
        this,
        [this, session, codec] {
            if (_isCurrent(session)) {
                _codec = codec;
                _supervisor->sessionStreaming();
            }
        },
        Qt::QueuedConnection);
}

void FFmpegVideoReceiver::onSessionDecoder(quint64 session, const FFmpegStreamSession::DecoderInfo& decoder)
{
    QMetaObject::invokeMethod(
        this,
        [this, session, decoder] {
            if (_isCurrent(session)) {
                _decoderInfo = decoder;
            }
        },
        Qt::QueuedConnection);
}

void FFmpegVideoReceiver::onSessionFrame(quint64 session, QVideoFrame frame)
{
    // Released after the lock: it may hand its decoder buffer back to a pool.
    QVideoFrame replaced;
    bool post = false;
    {
        const QMutexLocker locker(&_frameMutex);
        if (_pendingFrame.isValid() && (_pendingFrameSession == session)) {
            ++_framesDropped;
        }
        replaced = std::exchange(_pendingFrame, std::move(frame));
        _pendingFrameSession = session;
        post = !std::exchange(_frameShowPosted, true);
    }
    if (post) {
        QMetaObject::invokeMethod(this, &FFmpegVideoReceiver::_showFrame, Qt::QueuedConnection);
    }
}

void FFmpegVideoReceiver::onSessionDecoderFailed(quint64 session, const QString& message)
{
    QMetaObject::invokeMethod(
        this,
        [this, session, message] {
            if (!_isCurrent(session)) {
                return;
            }
            _sessionUndecodable = true;
            if (std::exchange(_decoderFailureReported, true)) {
                return;
            }
            emit errorOccurred(tr("Unable to decode the video: %1").arg(message));
        },
        Qt::QueuedConnection);
}

void FFmpegVideoReceiver::onSessionEnded(quint64 session, const std::optional<FFmpegStreamSession::Failure>& failure)
{
    QMetaObject::invokeMethod(
        this, [this, session, failure] { _onSessionEnded(session, failure); }, Qt::QueuedConnection);
}

void FFmpegVideoReceiver::onRecordingFileOpened(const QString& file)
{
    QMetaObject::invokeMethod(
        this,
        [this, file] {
            if (_recording) {
                (void) _recording->onFileOpened(file);
            }
            emit recordingStarted(file);
        },
        Qt::QueuedConnection);
}

void FFmpegVideoReceiver::onRecordingFileClosed(const QString& file, const QString& error)
{
    QMetaObject::invokeMethod(
        this,
        [this, file, error] {
            if (_recording) {
                _recording->onFileClosed(file);
                // A file that failed as it was written ends the recording, as one that can't be opened does.
                if (!error.isEmpty() && (file == _recording->currentFile())) {
                    _endRecording();
                }
            }
            emit recordingFinished(file, error);
        },
        Qt::QueuedConnection);
}

void FFmpegVideoReceiver::onRecordingNeedsFormat(const QString& file, FileFormat format)
{
    QMetaObject::invokeMethod(
        this,
        [this, file, format] {
            if (!_recording || (file != _recording->currentFile())) {
                return;
            }
            _recording->setFormat(format);
            qCDebug(FFmpegVideoReceiverLog)
                << name() << "recording to" << _recording->currentFile() << "instead of" << file;
            _updateRecorder();
        },
        Qt::QueuedConnection);
}

void FFmpegVideoReceiver::onRecordingFailed(const QString& file, const QString& error)
{
    QMetaObject::invokeMethod(
        this,
        [this, file, error] {
            if (_recording && (file == _recording->currentFile())) {
                _endRecording();
            }
            emit errorOccurred(tr("Video recording failed: %1").arg(error));
        },
        Qt::QueuedConnection);
}
