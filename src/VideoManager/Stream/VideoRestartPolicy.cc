#include "VideoRestartPolicy.h"

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VideoRestartPolicyLog, "Video.RestartPolicy")

VideoRestartPolicy::VideoRestartPolicy(QObject* timerContext)
{
    _timer.setSingleShot(true);
    _timer.setTimerType(Qt::CoarseTimer);
    if (timerContext) {
        _timer.moveToThread(timerContext->thread());
        _timer.setParent(timerContext);
    }
    QObject::connect(&_timer, &QTimer::timeout, timerContext, [this]() {
        if (auto fn = std::exchange(_pendingFn, nullptr))
            fn();
    });
}

void VideoRestartPolicy::noteSourceChanged()
{
    ++_generation;
    cancelPending();
}

void VideoRestartPolicy::noteStartRequested()
{
    _wantRunning = true;
}

void VideoRestartPolicy::noteStopRequested()
{
    ++_generation;
    _wantRunning = false;
    _restartPending = false;
    cancelPending();
}

void VideoRestartPolicy::noteReceiverFullyStarted()
{
    _attempts = 0;
    cancelPending();
}

VideoRestartPolicy::Decision VideoRestartPolicy::decide(Event event,
                                                        VideoReceiver::ReceiverState state,
                                                        const VideoSourceResolver::VideoSource& source,
                                                        bool hasUri) const
{
    switch (event) {
        case Event::UserRestart:
            if (!hasUri)
                return {state == VideoReceiver::ReceiverState::Stopped ? Action::Ignore : Action::StopNow, 0};
            if (state == VideoReceiver::ReceiverState::Stopped)
                return {Action::StartNow, 0};
            if (state == VideoReceiver::ReceiverState::Stopping)
                return {Action::MarkRestartPending, 0};
            return {Action::StopNow, 0};
        case Event::ReceiverTimeout:
            if (source.isRtsp() && _canSoftReconnect())
                return {Action::SoftReconnect, _peekBackoffMs()};
            return {Action::HardRestart, 0};
        case Event::ReceiverStopped:
            if (_restartPending || (_wantRunning && hasUri))
                return {Action::StartAfterBackoff, _peekBackoffMs()};
            return {Action::Ignore, 0};
        case Event::ReconnectRequested:
            return hasUri ? Decision{Action::StartAfterBackoff, _peekBackoffMs()} : Decision{};
    }
    return {};
}

void VideoRestartPolicy::requestRestart(VideoReceiver::ReceiverState state,
                                        bool hasUri,
                                        std::function<void()> start,
                                        std::function<void()> stop)
{
    static const VideoSourceResolver::VideoSource emptySource;
    const Decision decision = decide(Event::UserRestart, state, emptySource, hasUri);
    switch (decision.action) {
        case Action::StopNow:
            _wantRunning = hasUri;
            _restartPending = hasUri;
            stop();
            break;
        case Action::MarkRestartPending:
            _wantRunning = true;
            _restartPending = true;
            break;
        case Action::StartNow:
            start();
            break;
        case Action::Ignore:
        case Action::SoftReconnect:
        case Action::HardRestart:
        case Action::StartAfterBackoff:
            break;
    }
}

void VideoRestartPolicy::handleReceiverTimeout(const QString& name,
                                               const VideoSourceResolver::SourceDescriptor& source,
                                               std::function<void()> pause,
                                               std::function<void()> resume,
                                               std::function<void()> hardRestart)
{
    const Decision decision = decide(Event::ReceiverTimeout, VideoReceiver::ReceiverState::Running, source, source.isValid());
    if (decision.action == Action::SoftReconnect) {
        const int attempt = _attempts + 1;
        const int delayMs = _consumeBackoffMs();
        const quint64 generation = _generation;
        qCDebug(VideoRestartPolicyLog) << name << "RTSP reconnect attempt" << attempt
                                       << "backoff" << delayMs << "ms" << source.uri;
        pause();
        _scheduleRestart(delayMs, generation, [this, generation, resume = std::move(resume)]() {
            if (_isCurrentGeneration(generation) && _wantRunning)
                resume();
        });
        return;
    }

    qCDebug(VideoRestartPolicyLog) << name << "stream timeout - full restart" << source.uri;
    hardRestart();
}

void VideoRestartPolicy::handleReceiverFullyStopped(const QString& name,
                                                    const QString& uri,
                                                    bool hasUri,
                                                    std::function<void()> start)
{
    const Decision decision = decide(Event::ReceiverStopped, VideoReceiver::ReceiverState::Stopped, {}, hasUri);
    if (_restartPending) {
        _restartPending = false;
        const int delayMs = _consumeBackoffMs();
        const quint64 generation = _generation;
        qCDebug(VideoRestartPolicyLog) << "Restart (pending)" << name << uri << "backoff:" << delayMs << "ms";
        _scheduleRestart(delayMs, generation, std::move(start));
    } else if (decision.action == Action::StartAfterBackoff) {
        const int delayMs = _consumeBackoffMs();
        const quint64 generation = _generation;
        qCDebug(VideoRestartPolicyLog) << "Auto-reconnect" << name << uri << "backoff:" << delayMs << "ms (attempt"
                                       << _attempts << ")";
        _scheduleRestart(delayMs, generation, std::move(start));
    } else {
        qCDebug(VideoRestartPolicyLog) << name << "stopped (intentional or no valid URI)";
    }
}

void VideoRestartPolicy::handleReconnectRequested(const QString& name,
                                                  bool hasUri,
                                                  std::function<void()> requestStart)
{
    const Decision decision = decide(Event::ReconnectRequested, VideoReceiver::ReceiverState::Running, {}, hasUri);
    if (decision.action != Action::StartAfterBackoff)
        return;

    const int delayMs = _consumeBackoffMs();
    const quint64 generation = _generation;
    qCDebug(VideoRestartPolicyLog) << name << "reconnect requested, backoff:" << delayMs << "ms";
    _scheduleRestart(delayMs, generation, [hasUri, requestStart = std::move(requestStart)]() {
        if (hasUri)
            requestStart();
    });
}

void VideoRestartPolicy::cancelPending()
{
    _timer.stop();
    _pendingFn = nullptr;
}

int VideoRestartPolicy::_peekBackoffMs() const
{
    return std::min(kMaxBackoffMs, kBaseBackoffMs << _attempts);
}

int VideoRestartPolicy::_consumeBackoffMs()
{
    const int delay = _peekBackoffMs();
    ++_attempts;
    return delay;
}

void VideoRestartPolicy::_scheduleRestart(int delayMs, quint64 generation, std::function<void()> fn)
{
    _timer.stop();
    _pendingFn = [this, generation, fn = std::move(fn)]() {
        if (_isCurrentGeneration(generation))
            fn();
    };
    _timer.start(std::max(0, delayMs));
}
