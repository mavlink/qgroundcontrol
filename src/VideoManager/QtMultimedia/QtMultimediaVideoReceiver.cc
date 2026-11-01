#include "QtMultimediaVideoReceiver.h"

#include <algorithm>

#include <QtCore/QBuffer>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QPlaybackOptions>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameInput>
#include <QtMultimedia/QVideoSink>

#include "QGCLoggingCategory.h"
#include "QtVideoRecorder.h"
#include "SharedVideoFrame.h"
#include "VideoStreamStats.h"

QGC_LOGGING_CATEGORY(QtMultimediaVideoReceiverLog, "Video.QtMultimediaVideoReceiver")

namespace {

constexpr int kSampleIntervalMs = 1000;
/// Before a session's first frame, on top of the timeout: FFmpeg probes a stream for up to 5 s (its default
/// analyzeduration) before it decodes, and then waits for a keyframe.
constexpr qint64 kFirstFrameGraceMs = 5000;
/// How long the destructor waits for a recording's file to be finished.
constexpr int kRecordingFinishTimeoutMs = 5000;
/// The encoder's time base for a recording without a fixed frame rate is as coarse as a millisecond.
constexpr qint64 kMinFrameStepUs = 1000;
/// A larger step in the stream's times is a jump, as after a reconnect.
constexpr qint64 kMaxStreamStepUs = 1000000;

constexpr const char* kRtspTransportEnv = "QT_FFMPEG_RTSP_TRANSPORT";

/// Qt's FFmpeg player takes the RTSP transport from the environment alone, as it loads a source. A stream opening at
/// the same time may get the other's, which costs it no more than FFmpeg's own fallback from UDP to TCP.
void setRtspTransport(bool overTcp)
{
    static const QByteArray userTransport = qgetenv(kRtspTransportEnv);
    if (overTcp) {
        (void) qputenv(kRtspTransportEnv, QByteArrayLiteral("tcp"));
    } else if (userTransport.isNull()) {
        (void) qunsetenv(kRtspTransportEnv);
    } else {
        (void) qputenv(kRtspTransportEnv, userTransport);
    }
}

/// An SDP description of a stream of @p encoding RTP packets, payload type 96, arriving at @p port.
QByteArray rtpSdp(const QString& host, int port, QByteArrayView encoding)
{
    const QByteArray address = (host.isEmpty() ? QStringLiteral("0.0.0.0") : host).toLatin1();
    return QByteArrayLiteral("v=0\r\n") + "o=- 0 0 IN IP4 " + address + "\r\ns=QGroundControl\r\nc=IN IP4 " + address +
           "\r\nt=0 0\r\nm=video " + QByteArray::number(port) + " RTP/AVP 96\r\na=rtpmap:96 " + encoding + "/90000\r\n";
}

}  // namespace

QtMultimediaVideoReceiver::QtMultimediaVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
    , _supervisor(new StreamSupervisor(this))
    , _recorder(new QtVideoRecorder(this))
{
    (void) connect(_supervisor, &StreamSupervisor::stateChanged, this, &QtMultimediaVideoReceiver::setState);
    (void) connect(_recorder, &QtVideoRecorder::recordingChanged, this, &QtMultimediaVideoReceiver::setRecording);
    (void) connect(_recorder, &QtVideoRecorder::recordingStarted, this, &QtMultimediaVideoReceiver::recordingStarted);
    (void) connect(_recorder, &QtVideoRecorder::recordingFinished, this, &QtMultimediaVideoReceiver::recordingFinished);
    // Each file's times start at its first frame, also after a format that failed to encode.
    (void) connect(_recorder->mediaRecorder(), &QMediaRecorder::recorderStateChanged, this,
                   [this](QMediaRecorder::RecorderState state) {
                       if (state == QMediaRecorder::RecordingState) {
                           _sinceRecordingStarted.invalidate();
                           _lastRecordedUs = -1;
                           _lastStreamUs = -1;
                       }
                   });
    (void) connect(_supervisor, &StreamSupervisor::reconnectDue, this, &QtMultimediaVideoReceiver::_start);
    _sampleTimer.setInterval(kSampleIntervalMs);
    (void) connect(&_sampleTimer, &QTimer::timeout, this, &QtMultimediaVideoReceiver::_sample);
}

QtMultimediaVideoReceiver::~QtMultimediaVideoReceiver()
{
    _closing = true;
    _sampleTimer.stop();
    // Qt writes the file on a thread of its own, which the application's exit would cut short.
    _recorder->stopAndWait(kRecordingFinishTimeoutMs);
    // As _stopSession(): unloaded first, so that its destruction doesn't wait out a network read.
    if (_player) {
        _player->setSource(QUrl());
    }
}

std::optional<QtMultimediaVideoReceiver::Source> QtMultimediaVideoReceiver::playerSource(const QString& uri)
{
    const QUrl url(uri);
    if (uri.isEmpty() || !url.isValid() || url.scheme().isEmpty()) {
        return std::nullopt;
    }
    const QString scheme = url.scheme().toLower();
    if ((scheme == QStringLiteral("udp")) || (scheme == QStringLiteral("udp265"))) {
        if (url.port() <= 0) {
            return std::nullopt;
        }
        const QByteArrayView encoding = (scheme == QStringLiteral("udp265")) ? "H265" : "H264";
        return Source{QUrl(QStringLiteral("stream.sdp")), rtpSdp(url.host(), url.port(), encoding)};
    }
    if (scheme == QStringLiteral("mpegts")) {
        QUrl udp(url);
        udp.setScheme(QStringLiteral("udp"));
        return Source{udp, {}};
    }
    if (scheme == QStringLiteral("rtspt")) {
        QUrl rtsp(url);
        rtsp.setScheme(QStringLiteral("rtsp"));
        return Source{rtsp, {}, true};
    }
    return Source{url, {}};
}

void QtMultimediaVideoReceiver::onConfigChanged()
{
    // autoReconnect alone applies at once: a reconnect it no longer allows is dropped.
    if (_player || _supervisor->reconnectPending()) {
        Config previous = _sessionConfig;
        previous.autoReconnect = config().autoReconnect;
        if (previous == config()) {
            (void) _supervisor->autoReconnectChanged(config().autoReconnect);
            return;
        }
    }
    _startOver();
}

void QtMultimediaVideoReceiver::onActiveChanged()
{
    if (!active()) {
        _recorder->stop();
    }
    _startOver();
}

void QtMultimediaVideoReceiver::onRestartRequested()
{
    _startOver();
}

void QtMultimediaVideoReceiver::_startOver()
{
    _supervisor->startOver();
    _start();
}

void QtMultimediaVideoReceiver::onVideoOutputChanged()
{
    // Taking its output from a playing player waits out its network read on this thread, as stopping it would (Qt
    // joins the demuxer's thread uncancelled): the session starts over without one instead.
    if (_player && _player->videoSink() && !videoSink()) {
        _startOver();
    }
    _bindVideoSink();
    // The watchdog counts from now: no frame reached an output while there was none.
    _sinceFrame.restart();
}

void QtMultimediaVideoReceiver::onStartRecording(const QString& file, FileFormat format)
{
    if (_recorder->active()) {
        qCWarning(QtMultimediaVideoReceiverLog) << "Not recording" << name() << "to" << file << "- recording already";
        return;
    }
    if (!_recordingSession) {
        _recordingSession = new QMediaCaptureSession(this);
        _frameInput = new QVideoFrameInput(this);
        _recordingSession->setVideoFrameInput(_frameInput);
        _recordingSession->setRecorder(_recorder->mediaRecorder());
    }
    qCDebug(QtMultimediaVideoReceiverLog) << name() << "recording to" << file;
    _recorder->start(file, QtVideoRecorder::container(format));
}

void QtMultimediaVideoReceiver::onStopRecording()
{
    _recorder->stop();
}

void QtMultimediaVideoReceiver::_start()
{
    _stopSession();
    if (!active() || config().uri.isEmpty()) {
        _supervisor->stop();
        return;
    }
    if (!playerSource(config().uri)) {
        qCWarning(QtMultimediaVideoReceiverLog) << name() << "can't play" << config().uri;
        _supervisor->stop();
        emit errorOccurred(tr("Unable to play the video stream %1.").arg(config().uri));
        return;
    }
    _supervisor->sessionStarted();
    // Queued after the replaced player's unloading (_stopSession()): Windows doesn't let the new player bind a UDP port
    // the old one still holds.
    if (!_openPending) {
        _openPending = true;
        QMetaObject::invokeMethod(this, &QtMultimediaVideoReceiver::_open, Qt::QueuedConnection);
    }
}

void QtMultimediaVideoReceiver::_open()
{
    _openPending = false;
    // Stopped or restarted meanwhile: that _start() reported or opens it.
    const std::optional<Source> source = playerSource(config().uri);
    if (_player || !active() || !source || _closing) {
        return;
    }

    _sessionConfig = config();
    _player = new QMediaPlayer(this);
    QPlaybackOptions options;
    options.setNetworkTimeout(config().timeout);
    if (config().lowLatency) {
        options.setPlaybackIntent(QPlaybackOptions::PlaybackIntent::LowLatencyStreaming);
    }
    _player->setPlaybackOptions(options);
    (void) connect(_player, &QMediaPlayer::mediaStatusChanged, this, &QtMultimediaVideoReceiver::_onMediaStatusChanged);
    (void) connect(_player, &QMediaPlayer::errorOccurred, this,
                   [this](QMediaPlayer::Error, const QString& errorString) { _fail(errorString); });
    _bindVideoSink();
    if (source->url.scheme().compare(QStringLiteral("rtsp"), Qt::CaseInsensitive) == 0) {
        setRtspTransport(source->rtspOverTcp);
    }
    if (source->sdp.isEmpty()) {
        _player->setSource(source->url);
    } else {
        auto* const sdp = new QBuffer(_player);
        sdp->setData(source->sdp);
        (void) sdp->open(QIODevice::ReadOnly);
        _player->setSourceDevice(sdp, source->url);
    }
    qCDebug(QtMultimediaVideoReceiverLog) << name() << "playing" << config().uri;
    _sinceFrame.start();
    _sampleTimer.start();
    _player->play();
}

void QtMultimediaVideoReceiver::_stopSession()
{
    _sampleTimer.stop();
    if (_player) {
        QMediaPlayer* const player = std::exchange(_player, nullptr);
        (void) disconnect(player, nullptr, this, nullptr);
        // Unloaded before it goes: that cancels a network read first, which stopping it or detaching its sink would
        // wait out on this thread, for up to the network timeout per socket. Later, as this may run in one of its
        // signals. The output keeps the last frame (VideoOutput.KeepLastFrame).
        QMetaObject::invokeMethod(
            player,
            [player] {
                player->setSource(QUrl());
                delete player;
            },
            Qt::QueuedConnection);
    }
    _meter.reset();
    _frames = 0;
    _gpuFrames = false;
    setDecoding(false);
    setStats(VideoStreamStats());
}

void QtMultimediaVideoReceiver::_fail(const QString& reason)
{
    if (!_player) {
        return;
    }
    _stopSession();
    const StreamSupervisor::FailureOutcome outcome =
        _supervisor->sessionFailed(reason, /*permanent=*/false, config().autoReconnect);
    if (outcome.halted) {
        qCDebug(QtMultimediaVideoReceiverLog) << name() << "failed:" << reason;
        emit errorOccurred(tr("Video stream failed: %1").arg(reason));
    } else {
        qCDebug(QtMultimediaVideoReceiverLog)
            << name() << "failed, reconnecting in" << outcome.reconnectDelay << ":" << reason;
    }
}

void QtMultimediaVideoReceiver::_bindVideoSink()
{
    QVideoSink* const sink = videoSink();
    if (sink != _videoSink) {
        (void) disconnect(_frameConnection);
        _videoSink = sink;
        if (sink) {
            _frameConnection =
                connect(sink, &QVideoSink::videoFrameChanged, this, &QtMultimediaVideoReceiver::_onVideoFrame);
        }
    }
    if (_player) {
        _player->setVideoSink(sink);
    }
    if (!sink) {
        setDecoding(false);
    }
}

void QtMultimediaVideoReceiver::_onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
        case QMediaPlayer::BufferedMedia:
            _supervisor->sessionStreaming();
            break;
        case QMediaPlayer::EndOfMedia:
            _fail(tr("the stream ended"));
            break;
        case QMediaPlayer::InvalidMedia:
            _fail(_player ? _player->errorString() : QString());
            break;
        default:
            break;
    }
}

void QtMultimediaVideoReceiver::_onVideoFrame(const QVideoFrame& frame)
{
    // An invalid frame clears the display; frames can also arrive queued from a session that ended.
    if (!_player || !frame.isValid()) {
        return;
    }
    _sinceFrame.restart();
    ++_frames;
    _gpuFrames = (frame.handleType() != QVideoFrame::NoHandle);
    setVideoSize(presentedSize(frame));
    _supervisor->sessionStreaming();
    setDecoding(true);
    _recordFrame(frame);
}

void QtMultimediaVideoReceiver::_recordFrame(const QVideoFrame& frame)
{
    if (!_frameInput || !_recorder->active()) {
        return;
    }
    if (!_sinceRecordingStarted.isValid()) {
        _sinceRecordingStarted.start();
    }
    // A frame of its own: its times are the recording's, not the stream's, and copies of a frame share them.
    QVideoFrame recorded = SharedVideoFrame::withOwnMetadata(frame);
    const qint64 recordedUs = recordingTimeUs(_lastRecordedUs, _lastStreamUs, frame.startTime(),
                                              _sinceRecordingStarted.nsecsElapsed() / 1000);
    recorded.setStartTime(recordedUs);
    recorded.setEndTime(-1);
    // One the encoder can't take yet is dropped, as the display drops a frame it can't show in time.
    if (_frameInput->sendVideoFrame(recorded)) {
        _lastRecordedUs = recordedUs;
        _lastStreamUs = frame.startTime();
    }
}

qint64 QtMultimediaVideoReceiver::recordingTimeUs(qint64 lastRecordedUs, qint64 lastStreamUs, qint64 streamUs,
                                                  qint64 arrivalUs)
{
    if (lastRecordedUs < 0) {
        return arrivalUs;
    }
    // Frames queued while the GUI thread was busy arrive together, so they take the stream's spacing.
    const qint64 streamStepUs = ((streamUs >= 0) && (lastStreamUs >= 0)) ? (streamUs - lastStreamUs) : -1;
    const qint64 timeUs =
        ((streamStepUs > 0) && (streamStepUs < kMaxStreamStepUs)) ? (lastRecordedUs + streamStepUs) : arrivalUs;
    return std::max(timeUs, lastRecordedUs + kMinFrameStepUs);
}

void QtMultimediaVideoReceiver::_sample()
{
    if (!_player) {
        return;
    }
    const qint64 timeoutMs = std::chrono::duration_cast<std::chrono::milliseconds>(config().timeout).count() +
                             ((_frames == 0) ? kFirstFrameGraceMs : 0);
    if (_videoSink && (_sinceFrame.elapsed() > timeoutMs)) {
        _fail(tr("no video for %1 s").arg(timeoutMs / 1000));
        return;
    }
    if (_frames == 0) {
        return;
    }

    VideoStreamStats stats = this->stats();
    stats.decoder = QStringLiteral("Qt Multimedia");
    // Qt's FFmpeg backend hands over GPU frames only from a hardware decoder; a hardware decoder's frames copied to
    // memory read as software.
    stats.hardwareDecoder = _gpuFrames;
    stats.gpuPath = _gpuFrames ? QStringLiteral("GPU") : QStringLiteral("CPU");
    const auto codec = _player->metaData().value(QMediaMetaData::VideoCodec).value<QMediaFormat::VideoCodec>();
    stats.codec =
        (codec == QMediaFormat::VideoCodec::Unspecified) ? QString() : QMediaFormat::videoCodecDescription(codec);
    stats.uptimeMs = _supervisor->uptime().count();
    stats.reconnects = _supervisor->reconnects();
    VideoStreamStatsMeter::Counters counters;
    counters.framesDisplayed = _frames;
    if (_meter.update(counters, stats)) {
        // The player doesn't tell how many bytes it received.
        stats.bitrateKbps = -1;
        setStats(stats);
    }
}
