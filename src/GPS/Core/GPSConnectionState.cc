#include "GPSConnectionState.h"

#include <QtCore/QPointer>

#include <algorithm>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSConnectionStateLog, "GPS.Core.GPSConnectionState")

GPSConnectionState::GPSConnectionState(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
{
    qCDebug(GPSConnectionStateLog) << this;
}

GPSConnectionState::~GPSConnectionState()
{
    qCDebug(GPSConnectionStateLog) << this;
}

bool GPSConnectionState::shouldConnect(bool automatic) const
{
    return !_paused && (_manualRequested || automatic);
}

bool GPSConnectionState::updateIntent(bool automatic)
{
    const QPointer<GPSConnectionState> guard(this);
    const bool active = shouldConnect(automatic);
    if (_active != active) {
        _active = active;
        emit changed();
    }
    return guard && _active;
}

void GPSConnectionState::requestConnect()
{
    _manualRequested = true;
    _paused = false;
    _active = true;
    resetRetry();
    if (_state == Retrying) {
        _state = Disconnected;
    }
    emit changed();
}

void GPSConnectionState::pause()
{
    const bool changed = !_paused || _manualRequested || _active || _state == Retrying;
    _paused = true;
    _manualRequested = false;
    _active = false;
    resetRetry();
    if (_state == Retrying) {
        _state = Disconnected;
    }
    if (changed) {
        emit this->changed();
    }
}

void GPSConnectionState::stop()
{
    const bool changed = _manualRequested || _active || _state == Retrying;
    _manualRequested = false;
    _active = false;
    resetRetry();
    if (_state == Retrying) {
        _state = Disconnected;
    }
    if (changed) {
        emit this->changed();
    }
}

void GPSConnectionState::resetIntent()
{
    const bool changed = _paused || _manualRequested || _active || _state == Retrying;
    _paused = false;
    _manualRequested = false;
    _active = false;
    resetRetry();
    if (_state == Retrying) {
        _state = Disconnected;
    }
    if (changed) {
        emit this->changed();
    }
}

qint64 GPSConnectionState::retryRemainingMs() const
{
    return _retryDeadlineMs < 0 ? -1 : std::max<qint64>(0, _retryDeadlineMs - (_scheduler ? _scheduler->nowMs() : 0));
}

bool GPSConnectionState::canAttempt() const
{
    return _active && (_state == Disconnected || (_state == Retrying && retryRemainingMs() == 0));
}

bool GPSConnectionState::beginAttempt()
{
    if (!canAttempt()) {
        return false;
    }
    if (_state == Retrying) {
        _retryDelayMs = std::min(_retryDelayMs * 2, 30000);
    }
    _retryDeadlineMs = -1;
    const QPointer<GPSConnectionState> guard(this);
    _setState(Connecting);
    return guard && _active && _state == Connecting;
}

bool GPSConnectionState::startAttempt(const std::function<bool()>& start)
{
    if (!start || !canAttempt()) {
        return false;
    }
    const QPointer<GPSConnectionState> guard(this);
    const quint64 admission = _transitionRevision + 1;
    const bool admitted = beginAttempt();
    const bool started = admitted && guard && _transitionRevision == admission && start();
    if (!started && guard && _transitionRevision == admission && _state == Connecting) {
        _setState(Disconnected);
    }
    return started;
}

void GPSConnectionState::configuring()
{
    if (_active && _state == Connecting) {
        _setState(Configuring);
    }
}

void GPSConnectionState::ready()
{
    if (_active && (_state == Connecting || _state == Configuring)) {
        resetRetry();
        _setState(Ready);
    }
}

void GPSConnectionState::failed()
{
    if (_active && _state != Retrying && _state != Stopping) {
        _retryDeadlineMs = (_scheduler ? _scheduler->nowMs() : 0) + _retryDelayMs;
        _setState(Retrying);
    }
}

void GPSConnectionState::stopping()
{
    _setState(Stopping);
}

void GPSConnectionState::stopped()
{
    _setState(Disconnected);
}

void GPSConnectionState::resetRetry()
{
    _retryDeadlineMs = -1;
    _retryDelayMs = 1000;
}

void GPSConnectionState::_setState(State state)
{
    if (_state != state) {
        qCDebug(GPSConnectionStateLog) << this << _state << "->" << state;
        _state = state;
        ++_transitionRevision;
        emit changed();
    }
}
