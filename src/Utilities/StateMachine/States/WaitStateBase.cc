#include "WaitStateBase.h"

#include <QtStateMachine/QStateMachine>

WaitStateBase::WaitStateBase(const QString& stateName, QState* parent, int timeoutMsecs)
    : QGCState(stateName, parent)
    , _timeoutMsecs(timeoutMsecs > 0 ? timeoutMsecs : 0)
{
    _timeoutTimer.setSingleShot(true);
    connect(&_timeoutTimer, &QTimer::timeout, this, &WaitStateBase::_onTimeout);
    connect(this, &QState::entered, this, &WaitStateBase::_onEntered);
    connect(this, &QState::exited, this, &WaitStateBase::_stopWaiting);
}

void WaitStateBase::_onEntered()
{
    _completed = false;
    _waiting = true;
    // The machine is not known before the state is entered
    const QStateMachine* const qtMachine = QAbstractState::machine();  // clazy:exclude=skipped-base-method
    if (qtMachine) {
        connect(qtMachine, &QStateMachine::stopped, this, &WaitStateBase::_stopWaiting, Qt::UniqueConnection);
    }
    connectWaitSignal();
    onWaitEntered();
    if (!_completed) {
        _startTimeoutTimer();
    }
}

void WaitStateBase::_startTimeoutTimer()
{
    if (_timeoutMsecs > 0) {
        _timeoutTimer.start(_timeoutMsecs);
    }
}

void WaitStateBase::_stopWaiting()
{
    // Once for each time the state is entered, whichever of exited() and the stopped() of the machine comes first
    if (!_waiting) {
        return;
    }
    _waiting = false;

    _timeoutTimer.stop();
    disconnectWaitSignal();
    onWaitExited();
}

void WaitStateBase::_onTimeout()
{
    if (_completed) {
        return;
    }

    qCWarning(QGCStateMachineLog) << "Timeout" << stateName() << "after" << _timeoutTimer.interval() << "ms";

    disconnectWaitSignal();
    onWaitTimeout();
}

void WaitStateBase::onWaitTimeout()
{
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
    onWaitRestarted();
    if (!_completed) {
        _startTimeoutTimer();
    }
}
