#include "FFmpegVideoReceiver.h"

#include <utility>

#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>
#include <QtMultimedia/QVideoSink>

#include "QGCLoggingCategory.h"
#include "VideoStreamStats.h"
#include "VideoStreamStatsMeter.h"
#include "VideoStreamUri.h"

QGC_LOGGING_CATEGORY(FFmpegVideoReceiverLog, "Video.FFmpegVideoReceiver")

FFmpegVideoReceiver::FFmpegVideoReceiver(QObject* parent)
    : SessionVideoReceiver(FFmpegVideoReceiverLog, PacketsNoted::Yes, parent)
    , _recorder(std::make_unique<FFmpegStreamRecorder>(static_cast<FFmpegStreamRecorder::Listener&>(*this)))
{}

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
    // No frame reached an output while there was none.
    _watchdog.excuseSinkSilence(StreamWatchdog::now());
}

void FFmpegVideoReceiver::beginRecording(const QString& file, FileFormat format,
                                         [[maybe_unused]] const RecordingEncoding& encoding)
{
    qCDebug(FFmpegVideoReceiverLog) << name() << "recording to" << file;
    _recording.emplace(file, format);
    setRecording(true);
    _updateRecorder();
}

void FFmpegVideoReceiver::_updateRecorder()
{
    if (_recording) {
        _recorder->record(_recording->currentFile(), _recording->format(), _session ? _session->id() : 0);
    }
}

void FFmpegVideoReceiver::endRecording()
{
    if (!_recording) {
        return;
    }
    _recording.reset();
    _recorder->stop();
    setRecording(false);
}

void FFmpegVideoReceiver::startOver()
{
    _decoderFailureReported = false;
    SessionVideoReceiver::startOver();
}

void FFmpegVideoReceiver::openWhenReady()
{
    _openPending = !_endingSessions.empty();
    if (!_openPending) {
        open();
    }
}

void FFmpegVideoReceiver::openSession(const VideoStreamUri::FFmpegSource& source)
{
    const FFmpegStreamSession::Settings settings{
        .source = source,
        .redactedUri = redactedUri(),
        .networkTimeout = _sessionConfig.timeout,
        .lowLatency = _sessionConfig.lowLatency,
        .rtpJitterLatencyMs = _sessionConfig.rtpJitterLatencyMs,
        .softwareDecoderThreads = _sessionConfig.softwareDecoderThreads,
        // Off when the software decoder or the CPU video path is forced: frames decoded in software are in memory.
        .hardwareDecoding = displayConfig().gpuZeroCopy,
        .squarePixels = !displayConfig().disablePixelAspectRatio,
    };
    FFmpegStreamSession::Listener& listener = *this;
    _session = std::make_unique<FFmpegStreamSession>(++_lastSessionId, settings, listener, *_recorder, _watchdog);
    _session->setDecoding(videoSink() != nullptr);
    _updateRecorder();
    _session->start();
}

void FFmpegVideoReceiver::closeSession()
{
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
    _framesShown = 0;
    _decoderInfo = {};
    _codec.clear();
    _sessionUndecodable = false;
}

bool FFmpegVideoReceiver::pictureDue() const
{
    // Once the stream arrives, unless it can't be decoded.
    return videoSink() && !_sessionUndecodable && _session && (_session->counters().packets > 0);
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
    (void) _watchdog.noteSinkFrame(StreamWatchdog::now());
    setVideoSize(presentedSize(frame));
    setDecoding(true);
}

void FFmpegVideoReceiver::sampleSession()
{
    const FFmpegStreamSession::Counters counters = _session->counters();
    if (counters.packets == 0) {
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
        fail(failure ? failure->message : tr("the stream ended"), failure && failure->permanent);
    }
    for (auto it = _endingSessions.begin(); it != _endingSessions.end(); ++it) {
        if ((*it)->id() == id) {
            // Its thread has returned from its last call, or is about to: this waits for it.
            (void) _endingSessions.erase(it);
            break;
        }
    }
    if (_openPending && _endingSessions.empty()) {
        _openPending = false;
        open();
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
                    endRecording();
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
                endRecording();
            }
            emit errorOccurred(tr("Video recording failed: %1").arg(error));
        },
        Qt::QueuedConnection);
}
