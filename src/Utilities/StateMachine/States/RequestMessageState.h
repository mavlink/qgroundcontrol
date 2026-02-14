#pragma once

#include "WaitStateBase.h"
#include "Vehicle.h"

#include <cstdint>
#include <functional>

/// Requests a MAVLink message from the vehicle using MAV_CMD_REQUEST_MESSAGE.
/// Emits messageReceived() with the decoded message on success.
/// Emits completed()/advance() on success, timeout()/timedOut() on timeout, error() on failure.
///
/// Note: Vehicle::requestMessage() has its own internal timeout, but this state
/// adds an additional safety timeout at the state machine level.
class RequestMessageState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(RequestMessageState)

public:
    using MessageHandler = std::function<void(Vehicle* vehicle, const mavlink_message_t& message)>;

    /// @param parent Parent state
    /// @param messageId MAVLink message ID to request
    /// @param messageHandler Optional handler called with the received message before advance() is emitted
    /// @param compId Component ID to request from (default: MAV_COMP_ID_AUTOPILOT1)
    /// @param timeoutMsecs Timeout in milliseconds (default: 5000, 0 = no timeout)
    /// @param param1-param5 Optional parameters for the request
    RequestMessageState(QState* parent,
                        uint32_t messageId,
                        MessageHandler messageHandler = MessageHandler(),
                        int compId = MAV_COMP_ID_AUTOPILOT1,
                        int timeoutMsecs = 5000,
                        float param1 = 0.0f,
                        float param2 = 0.0f,
                        float param3 = 0.0f,
                        float param4 = 0.0f,
                        float param5 = 0.0f);

    /// @return The failure code from the last request attempt
    Vehicle::RequestMessageResultHandlerFailureCode_t failureCode() const { return _failureCode; }

    /// @return The MAV_RESULT from the last request attempt
    MAV_RESULT commandResult() const { return _commandResult; }

signals:
    /// Emitted when the message is successfully received
    void messageReceived(const mavlink_message_t& message);

protected:
    void connectWaitSignal() override {}
    void disconnectWaitSignal() override {}
    void onWaitEntered() override;
    void onWaitTimeout() override;

private:
    static void _resultHandler(void* resultHandlerData,
                               MAV_RESULT commandResult,
                               Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                               const mavlink_message_t& message);

    uint32_t        _messageId;
    MessageHandler  _messageHandler;
    int             _compId;
    float           _param1;
    float           _param2;
    float           _param3;
    float           _param4;
    float           _param5;

    Vehicle::RequestMessageResultHandlerFailureCode_t _failureCode = Vehicle::RequestMessageNoFailure;
    MAV_RESULT _commandResult = MAV_RESULT_ACCEPTED;
};
