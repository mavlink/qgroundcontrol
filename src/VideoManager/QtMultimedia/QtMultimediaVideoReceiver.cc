#include "QtMultimediaVideoReceiver.h"

#include <algorithm>

#include <QtCore/QBuffer>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QPlaybackOptions>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "QGCLoggingCategory.h"
#include "VideoStreamStats.h"

QGC_LOGGING_CATEGORY(QtMultimediaVideoReceiverLog, "Video.QtMultimediaVideoReceiver")

namespace {

constexpr int kSampleIntervalMs = 1000;
/// Before a session's first frame, on top of the timeout: FFmpeg probes a stream for up to 5 s (its default
/// analyzeduration) before it decodes, and then waits for a keyframe.
constexpr qint64 kFirstFrameGraceMs = 5000;

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
{
    _reconnectTimer.setSingleShot(true);
    (void) connect(&_reconnectTimer, &QTimer::timeout, this, &QtMultimediaVideoReceiver::_start);
    _sampleTimer.setInterval(kSampleIntervalMs);
    (void) connect(&_sampleTimer, &QTimer::timeout, this, &QtMultimediaVideoReceiver::_sample);
}

QtMultimediaVideoReceiver::~QtMultimediaVideoReceiver()
{
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
    return Source{url, {}};
}

void QtMultimediaVideoReceiver::onConfigChanged()
{
    // autoReconnect alone applies at once: a reconnect it no longer allows is dropped.
    if (_player || _reconnectTimer.isActive()) {
        Config previous = _sessionConfig;
        previous.autoReconnect = config().autoReconnect;
        if (previous == config()) {
            if (!config().autoReconnect && _reconnectTimer.isActive()) {
                _reconnectTimer.stop();
                setState(State::Stopped);
            }
            return;
        }
    }
    _startOver();
}

void QtMultimediaVideoReceiver::onActiveChanged()
{
    _startOver();
}

void QtMultimediaVideoReceiver::onRestartRequested()
{
    _startOver();
}

void QtMultimediaVideoReceiver::_startOver()
{
    _backoff.reset();
    _streamedSessions = 0;
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

void QtMultimediaVideoReceiver::onStartRecording(const QString& file, FileFormat)
{
    qCWarning(QtMultimediaVideoReceiverLog)
        << "Not recording" << name() << "to" << file << "- the Qt Multimedia backend doesn't record";
    emit errorOccurred(tr("Unable to record video: the Qt Multimedia video backend doesn't record streams."));
}

void QtMultimediaVideoReceiver::onStopRecording() {}

void QtMultimediaVideoReceiver::_start()
{
    _reconnectTimer.stop();
    _stopSession();
    if (!active() || config().uri.isEmpty()) {
        setState(State::Stopped);
        return;
    }
    if (!playerSource(config().uri)) {
        qCWarning(QtMultimediaVideoReceiverLog) << name() << "can't play" << config().uri;
        setState(State::Stopped);
        emit errorOccurred(tr("Unable to play the video stream %1.").arg(config().uri));
        return;
    }
    setState(State::Connecting);
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
    if (_player || !active() || !source) {
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
    _sinceStreaming.invalidate();
    setDecoding(false);
    setStats(VideoStreamStats());
}

void QtMultimediaVideoReceiver::_fail(const QString& reason)
{
    if (!_player) {
        return;
    }
    _stopSession();
    if (config().autoReconnect) {
        const auto delay = _backoff.next();
        qCDebug(QtMultimediaVideoReceiverLog) << name() << "failed, reconnecting in" << delay << ":" << reason;
        setState(State::Reconnecting);
        _reconnectTimer.start(delay);
    } else {
        qCDebug(QtMultimediaVideoReceiverLog) << name() << "failed:" << reason;
        setState(State::Stopped);
        emit errorOccurred(tr("Video stream failed: %1").arg(reason));
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
            setState(State::Streaming);
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
    if (!_sinceStreaming.isValid()) {
        _sinceStreaming.start();
        ++_streamedSessions;
        // The stream recovered: a later failure starts the backoff over.
        _backoff.reset();
    }
    setState(State::Streaming);
    setDecoding(true);
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
    stats.uptimeMs = _sinceStreaming.isValid() ? _sinceStreaming.elapsed() : 0;
    stats.reconnects = std::max(0, _streamedSessions - 1);
    VideoStreamStatsMeter::Counters counters;
    counters.framesDisplayed = _frames;
    if (_meter.update(counters, stats)) {
        // The player doesn't tell how many bytes it received.
        stats.bitrateKbps = -1;
        setStats(stats);
    }
}
