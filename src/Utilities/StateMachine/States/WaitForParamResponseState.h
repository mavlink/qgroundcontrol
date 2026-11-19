#pragma once

#include <cstdint>
#include <functional>

#include "VehicleMessageWaitState.h"

/// \brief Waits for either PARAM_VALUE (success) or PARAM_ERROR (rejection) from the vehicle.
///
/// On PARAM_VALUE matching predicate: calls waitComplete() (emits advance())
/// On PARAM_ERROR matching predicate: stores error info, calls waitFailed() (emits error())
/// On timeout: timedOut() fires (for retry path)
///
class WaitForParamResponseState : public VehicleMessageWaitState
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

    /// @return The error code of the PARAM_ERROR that failed the state since it was last entered, 0 if none did
    uint8_t lastParamError() const { return _lastParamError; }

    /// @return Human-readable string for lastParamError(), empty if no PARAM_ERROR failed the state
    QString lastParamErrorString() const { return _lastParamErrorString; }

protected:
    void onWaitEntered() override;
    void handleMessage(const mavlink_message_t& message) override;

private:
    static QString _paramErrorToString(uint8_t errorCode);

    Predicate _paramValuePredicate;
    Predicate _paramErrorPredicate;
    uint8_t _lastParamError = 0;
    QString _lastParamErrorString;
};
