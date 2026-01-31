#pragma once

#include "WaitStateBase.h"

#include <QtCore/QMetaObject>

#include <functional>

/// Waits for a signal from a QObject before advancing
/// Can optionally timeout if the signal is not received within a specified time
class WaitForSignalState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(WaitForSignalState)

public:
    /// @param stateName Name for this state (for logging)
    /// @param parent Parent state
    /// @param sender The QObject to wait for a signal from
    /// @param signal Pointer to member signal
    /// @param timeoutMsecs Timeout in milliseconds, 0 for no timeout
    template<typename Func>
    WaitForSignalState(const QString& stateName, QState* parent,
                       typename QtPrivate::FunctionPointer<Func>::Object* sender,
                       Func signal, int timeoutMsecs = 0)
        : WaitStateBase(stateName, parent, timeoutMsecs)
    {
        // Store the connection setup as a lambda to be called on entry
        // Use a lambda to handle signals with any parameter types
        _connectFunc = [this, sender, signal]() {
            _signalConnection = connect(sender, signal, this, [this]() {
                _onSignalReceived();
            });
        };
    }

protected:
    void connectWaitSignal() override;
    void disconnectWaitSignal() override;

private:
    void _onSignalReceived();

    QMetaObject::Connection _signalConnection;
    std::function<void()> _connectFunc;
};
