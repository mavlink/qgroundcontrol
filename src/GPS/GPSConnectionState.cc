#include "GPSConnectionState.h"

#include <QtCore/QPointer>

#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSConnectionStateLog, "GPS.GPSConnectionState")

GPSConnectionState::GPSConnectionState(QObject* parent) : QObject(parent)
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

bool GPSConnectionState::canAttempt() const
{
    return _active && (_state == Disconnected || (_state == Retrying && _retryDeadline.hasExpired()));
}

bool GPSConnectionState::beginAttempt()
{
    if (!canAttempt()) {
        return false;
    }
    if (_state == Retrying) {
        _retryDelayMs = std::min(_retryDelayMs * 2, 30000);
    }
    _retryDeadline = QDeadlineTimer::Forever;
    const QPointer<GPSConnectionState> guard(this);
    _setState(Connecting);
    return guard && _active && _state == Connecting;
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
        _retryDeadline.setRemainingTime(_retryDelayMs);
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
    _retryDeadline = QDeadlineTimer::Forever;
    _retryDelayMs = 1000;
}

void GPSConnectionState::_setState(State state)
{
    if (_state != state) {
        qCDebug(GPSConnectionStateLog) << this << _state << "->" << state;
        _state = state;
        emit changed();
    }
}
