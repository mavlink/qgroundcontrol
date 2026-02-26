#include "EventQueuedState.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

EventQueuedState::EventQueuedState(const QString& stateName, QState* parent,
                                   const QString& eventName, int timeoutMsecs)
    : WaitStateBase(stateName, parent, timeoutMsecs)
{
    _expectedEvents.insert(eventName);
}

EventQueuedState::EventQueuedState(const QString& stateName, QState* parent,
                                   const QSet<QString>& eventNames, int timeoutMsecs)
    : WaitStateBase(stateName, parent, timeoutMsecs)
    , _expectedEvents(eventNames)
{
}

void EventQueuedState::addExpectedEvent(const QString& eventName)
{
    _expectedEvents.insert(eventName);
}

void EventQueuedState::removeExpectedEvent(const QString& eventName)
{
    _expectedEvents.remove(eventName);
}

void EventQueuedState::connectWaitSignal()
{
    if (machine()) {
        _eventConnection = connect(machine(), &QGCStateMachine::machineEvent,
                                   this, &EventQueuedState::_onMachineEvent);
    }
}

void EventQueuedState::disconnectWaitSignal()
{
    if (_eventConnection) {
        disconnect(_eventConnection);
        _eventConnection = {};
    }
}

void EventQueuedState::onWaitEntered()
{
    WaitStateBase::onWaitEntered();
    _receivedEvent.clear();

    qCDebug(QGCStateMachineLog) << stateName() << "waiting for events:"
                                 << _expectedEvents.values().join(", ");
}

void EventQueuedState::_onMachineEvent(const QString& eventName)
{
    if (_expectedEvents.contains(eventName)) {
        qCDebug(QGCStateMachineLog) << stateName() << "received expected event:" << eventName;

        _receivedEvent = eventName;
        emit eventReceived(eventName);
        waitComplete();
    }
}
