#pragma once

#include "MAVLinkMessageType.h"
#include "WaitStateBase.h"

/// \brief Base class for states that wait for a MAVLink message from the vehicle
///
/// While the state waits, handleMessage() is given every message the vehicle receives. In a machine without a
/// vehicle the state fails when it is entered.
///
class VehicleMessageWaitState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(VehicleMessageWaitState)

protected:
    using WaitStateBase::WaitStateBase;

    void connectWaitSignal() override;
    void disconnectWaitSignal() override;

    virtual void handleMessage(const mavlink_message_t& message) = 0;
};
