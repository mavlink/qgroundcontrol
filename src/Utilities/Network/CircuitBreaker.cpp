/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CircuitBreaker.h"

QGC_LOGGING_CATEGORY(CircuitBreakerLog, "qgc.qtlocationplugin.circuitbreaker")

CircuitBreaker::CircuitBreaker(QObject *parent)
    : QObject(parent)
{
}

bool CircuitBreaker::allowRequest()
{
    QMutexLocker locker(&_mutex);

    switch (_state) {
    case State::Closed:
        return true;

    case State::Open:
        attemptReset();
        return _state != State::Open;

    case State::HalfOpen:
        return true;
    }

    return false;
}

void CircuitBreaker::recordSuccess()
{
    QMutexLocker locker(&_mutex);

    _failureCount = 0;

    if (_state == State::HalfOpen) {
        _successCount++;
        if (_successCount >= _successThreshold) {
            setState(State::Closed);
            qCDebug(CircuitBreakerLog) << "Circuit closed after successful recovery";
        }
    }
}

void CircuitBreaker::recordFailure()
{
    QMutexLocker locker(&_mutex);

    _failureCount++;

    if (_state == State::Closed && _failureCount >= _failureThreshold) {
        setState(State::Open);
        _openedTime = QDateTime::currentDateTime();
        qCWarning(CircuitBreakerLog) << "Circuit opened after" << _failureCount << "failures";
        emit circuitOpened();
    } else if (_state == State::HalfOpen) {
        setState(State::Open);
        _openedTime = QDateTime::currentDateTime();
        qCWarning(CircuitBreakerLog) << "Circuit reopened after failure in half-open state";
        emit circuitOpened();
    }
}

void CircuitBreaker::attemptReset()
{
    if (_state != State::Open) {
        return;
    }

    const qint64 elapsed = _openedTime.msecsTo(QDateTime::currentDateTime());
    if (elapsed >= _timeoutMs) {
        setState(State::HalfOpen);
        _successCount = 0;
        qCDebug(CircuitBreakerLog) << "Circuit entering half-open state after timeout";
    }
}

void CircuitBreaker::setState(State newState)
{
    if (_state != newState) {
        _state = newState;
        emit stateChanged(newState);

        if (newState == State::Closed) {
            emit circuitClosed();
        }
    }
}

QString CircuitBreaker::stateString() const
{
    switch (_state) {
    case State::Closed: return "Closed";
    case State::Open: return "Open";
    case State::HalfOpen: return "HalfOpen";
    }
    return "Unknown";
}

void CircuitBreaker::reset()
{
    QMutexLocker locker(&_mutex);
    _state = State::Closed;
    _failureCount = 0;
    _successCount = 0;
    _openedTime = QDateTime();
    emit circuitClosed();
}
