#include "GPSConnectionSupervisor.h"

GPSConnectionSupervisor::GPSConnectionSupervisor(ExponentialBackoff backoff, std::optional<int> maxAttempts)
    : _backoff(backoff)
    , _maxAttempts(maxAttempts)
{}

void GPSConnectionSupervisor::start(bool gateClosed)
{
    _backoff.reset();
    _retryDelay.reset();
    _phase = gateClosed ? Phase::Waiting : Phase::Opening;
}

void GPSConnectionSupervisor::stop()
{
    _retryDelay.reset();
    _phase = Phase::Off;
}

void GPSConnectionSupervisor::reopen()
{
    if (_phase == Phase::Up) {
        _phase = Phase::Opening;
    }
}

void GPSConnectionSupervisor::connected()
{
    if (_phase == Phase::Opening) {
        _phase = Phase::Up;
    }
}

void GPSConnectionSupervisor::served()
{
    _backoff.reset();
}

void GPSConnectionSupervisor::failed(Delay retryAfter, bool gateClosed)
{
    if (_phase != Phase::Opening && _phase != Phase::Up) {
        return;
    }
    if (!gateClosed && _maxAttempts && _backoff.attempts() >= *_maxAttempts) {
        _phase = Phase::Stopped;
        return;
    }
    _phase = Phase::Waiting;
    if (!gateClosed) {
        _retryDelay = _backoff.peek(retryAfter);
    }
}

void GPSConnectionSupervisor::halt()
{
    if (_phase != Phase::Off) {
        _retryDelay.reset();
        _phase = Phase::Stopped;
    }
}

void GPSConnectionSupervisor::delayEnded(bool gateClosed)
{
    if (!_retryDelay) {
        return;
    }
    _retryDelay.reset();
    if (!gateClosed) {
        // Only now is the attempt one of the budget's, so a closed gate has nothing to give back.
        _backoff.next();
        _phase = Phase::Opening;
    }
}

void GPSConnectionSupervisor::gateOpened()
{
    if (_phase == Phase::Waiting && !_retryDelay) {
        _phase = Phase::Opening;
    }
}
