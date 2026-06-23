#include "WaitForSignalState.h"
#include "QGCLoggingCategory.h"

void WaitForSignalState::connectWaitSignal()
{
    if (_connectFunc) {
        _connectFunc();
    }
}

void WaitForSignalState::disconnectWaitSignal()
{
    if (_signalConnection) {
        disconnect(_signalConnection);
        _signalConnection = {};
    }
}

void WaitForSignalState::_onSignalReceived()
{
    qCDebug(QGCStateMachineLog) << "Signal received" << stateName();
    waitComplete();
}
