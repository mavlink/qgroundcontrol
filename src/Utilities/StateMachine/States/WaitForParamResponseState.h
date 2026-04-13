#pragma once

#include "WaitStateBase.h"
#include "QGCMAVLink.h"

#include <cstdint>
#include <functional>

class Vehicle;

/// Waits for either PARAM_VALUE (success) or PARAM_ERROR (rejection) from the vehicle.
/// On PARAM_VALUE matching predicate: calls waitComplete() (emits advance())
/// On PARAM_ERROR matching predicate: stores error info, calls waitFailed() (emits error())
/// On timeout: existing timeout() signal fires (for retry path)
class WaitForParamResponseState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(WaitForParamResponseState)

public:
    using Predicate = std::function<bool(const mavlink_message_t &message)>;

    /// @param parent Parent state
    /// @param timeoutMsecs Timeout in milliseconds to wait for response, 0 to wait forever
    /// @param paramValuePredicate Predicate to filter PARAM_VALUE messages
    /// @param paramErrorPredicate Predicate to filter PARAM_ERROR messages
    WaitForParamResponseState(QState *parent, int timeoutMsecs,
                              Predicate paramValuePredicate,
                              Predicate paramErrorPredicate);

    /// @return The last PARAM_ERROR error code received
    uint8_t lastParamError() const { return _lastParamError; }

    /// @return Human-readable string for the last PARAM_ERROR received
    QString lastParamErrorString() const { return _lastParamErrorString; }

protected:
    void connectWaitSignal() override;
    void disconnectWaitSignal() override;

private slots:
    void _messageReceived(const mavlink_message_t &message);

private:
    static QString _paramErrorToString(uint8_t errorCode);

    Predicate _paramValuePredicate;
    Predicate _paramErrorPredicate;
    uint8_t _lastParamError = 0;
    QString _lastParamErrorString;
};
