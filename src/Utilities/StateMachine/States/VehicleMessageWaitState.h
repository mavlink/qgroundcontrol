#pragma once

#include <cstdint>
#include <functional>

#include "MAVLinkMessageType.h"
#include "WaitStateBase.h"

/// \brief Base class for states that wait for a MAVLink message from the vehicle
///
/// While the state waits, handleMessage() is given every message the vehicle receives. In a machine without a
/// vehicle the state fails when it is entered.
///
/// A state with a request sends it to the vehicle when it is entered and each time it waits again. After a
/// timeout it waits again by itself, as often as the retry count of the request allows: timedOut() is emitted
/// when the last attempt has timed out. A request that cannot be sent fails the state.
///
class VehicleMessageWaitState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(VehicleMessageWaitState)

public:
    using MessageEncoder = std::function<void(quint32 systemId, uint8_t channel, mavlink_message_t* message)>;

    /// @param encoder Function which encodes the message that asks the vehicle for what the state waits for
    /// @param retryCount Number of times the request is sent again after a timeout
    void setRequest(MessageEncoder encoder, int retryCount);

protected:
    using WaitStateBase::WaitStateBase;

    void connectWaitSignal() override;
    void disconnectWaitSignal() override;
    void onWaitEntered() override;
    void onWaitRestarted() override;
    void onWaitTimeout() override;

    virtual void handleMessage(const mavlink_message_t& message) = 0;

private:
    void _sendRequest();

    MessageEncoder _requestEncoder;
    int _requestRetryCount = 0;
    int _requestRetriesDone = 0;
};
