#include "WaitStateBase.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

WaitStateBase::WaitStateBase(const QString& stateName, QState* parent, int timeoutMsecs)
    : QGCState(stateName, parent)
    , _timeoutMsecs(timeoutMsecs > 0 ? timeoutMsecs : 0)
{
    _timeoutTimer.setSingleShot(true);
    connect(&_timeoutTimer, &QTimer::timeout, this, &WaitStateBase::_onTimeout);
    connect(this, &QState::entered, this, &WaitStateBase::_onEntered);
    connect(this, &QState::exited, this, &WaitStateBase::_onExited);
}

void WaitStateBase::_onEntered()
{
    _completed = false;
    connectWaitSignal();
    onWaitEntered();

    // Check for runtime timeout override
    int effectiveTimeout = _timeoutMsecs;
    if (machine()) {
        int override = machine()->timeoutOverride(objectName());
        if (override >= 0) {
            effectiveTimeout = override;
            qCDebug(QGCStateMachineLog) << stateName() << "using timeout override:" << effectiveTimeout << "ms";
        }
    }

    if (effectiveTimeout > 0) {
        _timeoutTimer.start(effectiveTimeout);
    }
}

void WaitStateBase::_onExited()
{
    _timeoutTimer.stop();
    disconnectWaitSignal();
    onWaitExited();
}

void WaitStateBase::_onTimeout()
{
    if (_completed) {
        return;
    }

    qCDebug(QGCStateMachineLog) << "Timeout" << stateName();

    // Record timeout for statistics
    if (machine()) {
        machine()->recordTimeout(objectName());
    }

    disconnectWaitSignal();
    onWaitTimeout();
}

void WaitStateBase::onWaitEntered()
{
    // Default implementation does nothing - subclasses can override
}

void WaitStateBase::onWaitExited()
{
    // Default implementation does nothing - subclasses can override
}

void WaitStateBase::onWaitTimeout()
{
    emit timeout();
    emit timedOut();
}

void WaitStateBase::waitComplete()
{
    if (_completed) {
        return;
    }
    _completed = true;

    _timeoutTimer.stop();
    disconnectWaitSignal();

    emit completed();
    emit advance();
}

void WaitStateBase::waitFailed()
{
    if (_completed) {
        return;
    }
    _completed = true;

    _timeoutTimer.stop();
    disconnectWaitSignal();

    emit error();
}

void WaitStateBase::restartWait()
{
    if (_completed) {
        return;
    }

    // _onTimeout() disconnects wait signals. Retry flows that remain in the same
    // state need an explicit rearm of both wait connections and timeout timer.
    disconnectWaitSignal();
    connectWaitSignal();

    int effectiveTimeout = _timeoutMsecs;
    if (machine()) {
        const int override = machine()->timeoutOverride(objectName());
        if (override >= 0) {
            effectiveTimeout = override;
        }
    }

    if (effectiveTimeout > 0) {
        _timeoutTimer.start(effectiveTimeout);
    }
}
