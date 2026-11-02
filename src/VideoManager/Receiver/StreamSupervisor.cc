#include "StreamSupervisor.h"

#include <algorithm>

StreamSupervisor::StreamSupervisor(QObject* parent, ExponentialBackoff backoff)
    : QObject(parent)
    , _backoff(backoff)
    , _reconnectTimer(this)
{
    _reconnectTimer.setSingleShot(true);
    // Backoff delays are asserted by tests; a coarse timer may fire up to 5% early.
    _reconnectTimer.setTimerType(Qt::PreciseTimer);
    (void) connect(&_reconnectTimer, &QTimer::timeout, this, &StreamSupervisor::reconnectDue);
}

VideoReceiver::State StreamSupervisor::stateFor(Phase phase)
{
    switch (phase) {
        case Phase::Connecting:
            return VideoReceiver::State::Connecting;
        case Phase::Streaming:
            return VideoReceiver::State::Streaming;
        case Phase::Reconnecting:
            return VideoReceiver::State::Reconnecting;
        case Phase::Idle:
        case Phase::Halted:
            break;
    }
    return VideoReceiver::State::Stopped;
}

int StreamSupervisor::reconnects() const
{
    return std::max(0, _streamedSessions - 1);
}

std::chrono::milliseconds StreamSupervisor::uptime() const
{
    return std::chrono::milliseconds(_sinceStreaming.isValid() ? _sinceStreaming.elapsed() : 0);
}

void StreamSupervisor::startOver()
{
    _reconnectTimer.stop();
    _backoff.reset();
    _streamedSessions = 0;
    _haltedPermanently = false;
    if (_phase == Phase::Halted) {
        _setPhase(Phase::Idle);
    }
}

void StreamSupervisor::stop()
{
    _reconnectTimer.stop();
    _endSession();
    _haltedPermanently = false;
    _setPhase(Phase::Idle);
}

void StreamSupervisor::sessionStarted()
{
    _reconnectTimer.stop();
    _endSession();
    _setPhase(Phase::Connecting);
}

void StreamSupervisor::sessionStreaming()
{
    if (!_sessionStreamed) {
        _sessionStreamed = true;
        ++_streamedSessions;
        _sinceStreaming.start();
    }
    _setPhase(Phase::Streaming);
}

StreamSupervisor::FailureOutcome StreamSupervisor::sessionFailed(const QString& message, bool permanent,
                                                                 bool autoReconnect)
{
    _lastFailure = message;
    if (_sessionStreamed) {
        _backoff.reset();
    }
    _endSession();

    if (permanent || !autoReconnect) {
        _reconnectTimer.stop();
        _haltedPermanently = permanent;
        _setPhase(Phase::Halted);
        return {.halted = true};
    }

    const std::chrono::milliseconds delay = _backoff.next();
    _reconnectTimer.start(delay);
    _setPhase(Phase::Reconnecting);
    return {.halted = false, .reconnectDelay = delay};
}

StreamSupervisor::AutoReconnectChange StreamSupervisor::autoReconnectChanged(bool autoReconnect)
{
    if (!autoReconnect && _reconnectTimer.isActive()) {
        _reconnectTimer.stop();
        _haltedPermanently = false;
        _setPhase(Phase::Halted);
        return AutoReconnectChange::Halted;
    }
    if (autoReconnect && (_phase == Phase::Halted) && !_haltedPermanently) {
        _setPhase(Phase::Idle);
        return AutoReconnectChange::Resumable;
    }
    return AutoReconnectChange::None;
}

void StreamSupervisor::_setPhase(Phase phase)
{
    if (phase == _phase) {
        return;
    }
    const VideoReceiver::State previous = state();
    _phase = phase;
    if (state() != previous) {
        emit stateChanged(state());
    }
}

void StreamSupervisor::_endSession()
{
    _sessionStreamed = false;
    _sinceStreaming.invalidate();
}
