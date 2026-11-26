#include "SessionVideoReceiver.h"

#include <chrono>
#include <optional>

#include <QtCore/QUrl>

#include "QGCNetworkHelper.h"

namespace {

constexpr std::chrono::milliseconds kSessionSampleInterval{1000};
/// On top of the timeout, for a server's first packet and for a session's first picture: FFmpeg may probe a stream for
/// up to 5 s (its default analyzeduration) before it decodes, and decoding starts at the stream's next keyframe, which
/// a camera may send seconds apart.
constexpr std::chrono::milliseconds kSessionFirstFrameGrace{5000};

}  // namespace

SessionVideoReceiver::SessionVideoReceiver(LogCategory log, PacketsNoted packetsNoted, QObject* parent)
    : VideoReceiver(parent)
    , _supervisor(new StreamSupervisor(this))
    , _statsSampler(*_supervisor)
    , _log(log)
    , _packetsNoted(packetsNoted)
{
    (void) connect(_supervisor, &StreamSupervisor::stateChanged, this, &SessionVideoReceiver::setState);
    (void) connect(_supervisor, &StreamSupervisor::reconnectDue, this, &SessionVideoReceiver::start);
    _sampleTimer.setInterval(kSessionSampleInterval);
    (void) connect(&_sampleTimer, &QTimer::timeout, this, &SessionVideoReceiver::_sample);
}

void SessionVideoReceiver::onConfigChanged()
{
    // autoReconnect alone applies at once: a reconnect it no longer allows is dropped.
    if ((hasSession() || _supervisor->reconnectPending()) && config().sameStreamAs(_sessionConfig)) {
        if (_supervisor->autoReconnectChanged(config().autoReconnect) ==
            StreamSupervisor::AutoReconnectChange::Halted) {
            _onHalted(_supervisor->lastFailure());
        }
        return;
    }
    startOver();
}

void SessionVideoReceiver::onActiveChanged()
{
    if (!active()) {
        endRecording();
    }
    startOver();
}

void SessionVideoReceiver::onRestartRequested()
{
    startOver();
}

void SessionVideoReceiver::onReconnectRequested()
{
    _supervisor->reconnectNow();
}

void SessionVideoReceiver::onStartRecording(const QString& file, FileFormat format, const RecordingEncoding& encoding)
{
    if (_supervisor->halted()) {
        emit errorOccurred(tr("Unable to record video: the video stream has stopped."));
        return;
    }
    beginRecording(file, format, encoding);
}

void SessionVideoReceiver::onStopRecording()
{
    endRecording();
}

void SessionVideoReceiver::startOver()
{
    _supervisor->startOver();
    start();
}

void SessionVideoReceiver::start()
{
    _stopSession();
    if (!active() || config().uri.isEmpty()) {
        _supervisor->stop();
        return;
    }
    if (!VideoStreamUri::ffmpegSource(config().uri)) {
        qCWarning(_log) << name() << "can't play" << redactedUri();
        const QString message = tr("Unable to play the video stream %1.").arg(redactedUri());
        (void) _supervisor->sessionFailed(message, StreamSupervisor::Failure::Permanent, config().autoReconnect);
        _onHalted(message);
        return;
    }
    _supervisor->sessionStarted();
    openWhenReady();
}

void SessionVideoReceiver::open()
{
    // Stopped or restarted meanwhile: that start() reported or opens it.
    const std::optional<VideoStreamUri::FFmpegSource> source = VideoStreamUri::ffmpegSource(config().uri);
    if (hasSession() || !active() || !source) {
        return;
    }

    _sessionConfig = config();
    // A UDP port waits for its sender for as long as it takes; a server that sends nothing has failed.
    const bool listens = source->transport && VideoStreamUri::listens(*source->transport);
    const bool serverWatched = (_packetsNoted == PacketsNoted::Yes) && !listens;
    _watchdog.start(_sessionConfig.timeout, serverWatched ? std::optional(kSessionFirstFrameGrace) : std::nullopt,
                    {.timeouts = 1, .firstFrameGrace = kSessionFirstFrameGrace});
    const qint64 now = StreamWatchdog::now();
    _watchdog.startSinkClock(now);
    // Starts the clock of the first packet now, rather than with the first sample.
    (void) _watchdog.check(now, false);
    // Before the session opens its sockets, so its own timeout can't run out sooner than this one.
    _sinceOpened.start();
    qCDebug(_log) << name() << "playing" << redactedUri();
    // Before the session, which may fail as it opens.
    _sampleTimer.start();
    openSession(*source);
}

void SessionVideoReceiver::_stopSession()
{
    _sampleTimer.stop();
    closeSession();
    (void) _statsSampler.reset();
    setDecoding(false);
    setStats(_statsSampler.stats());
}

QString SessionVideoReceiver::redactedUri() const
{
    return QGCNetworkHelper::redactedUrlForLogging(config().uri);
}

bool SessionVideoReceiver::_awaitingSender() const
{
    const std::optional<VideoStreamUri::Transport> transport = VideoStreamUri::transportFor(QUrl(_sessionConfig.uri));
    // A session that ends sooner than its timeout didn't run out of patience: its port is taken, or it can't play.
    return transport && VideoStreamUri::listens(*transport) &&
           (_supervisor->phase() == StreamSupervisor::Phase::Connecting) &&
           (_sinceOpened.durationElapsed() >= _sessionConfig.timeout);
}

void SessionVideoReceiver::fail(const QString& reason, bool permanent)
{
    if (!hasSession()) {
        return;
    }
    if (!permanent && _awaitingSender()) {
        // FFmpeg gives up on a silent port; a backoff would leave it unbound when the stream starts.
        qCDebug(_log) << name() << "no stream yet, listening on:" << reason;
        start();
        return;
    }
    _stopSession();
    const QString message = tr("Video stream failed: %1").arg(reason);
    const StreamSupervisor::FailureOutcome outcome = _supervisor->sessionFailed(
        message, permanent ? StreamSupervisor::Failure::Permanent : StreamSupervisor::Failure::Transient,
        config().autoReconnect);
    if (outcome.halted) {
        qCDebug(_log) << name() << "failed:" << reason;
        _onHalted(message);
    } else {
        qCDebug(_log) << name() << "failed, reconnecting in" << outcome.reconnectDelay << ":" << reason;
    }
}

void SessionVideoReceiver::_onHalted(const QString& message)
{
    endRecording();
    emit errorOccurred(message);
}

void SessionVideoReceiver::_sample()
{
    if (!hasSession()) {
        return;
    }
    const StreamWatchdog::Result stalled = _watchdog.check(StreamWatchdog::now(), pictureDue());
    switch (stalled.stall) {
        case StreamWatchdog::Stall::None:
            break;
        case StreamWatchdog::Stall::Source:
            fail(tr("no video received for %1 s").arg(stalled.silentMs / 1000));
            return;
        case StreamWatchdog::Stall::Decoder:
            fail(tr("no picture for %1 s").arg(stalled.silentMs / 1000));
            return;
    }
    sampleSession();
}
