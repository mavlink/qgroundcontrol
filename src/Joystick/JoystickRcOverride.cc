//
// Created by zdanek on 08.07.2022.
//

#include "JoystickRcOverride.h"

#include "Vehicle.h"

JoystickRcOverride::JoystickRcOverride(const uint8_t rcChannel,
                                       const uint16_t loPwmValue,
                                       const uint16_t hiPwmValue,
                                       bool latch)
    : rcChannel(rcChannel)
    , loPwmValue(loPwmValue)
    , hiPwmValue(hiPwmValue)
    , latchMode(latch)
{}

void JoystickRcOverride::send(Vehicle *vehicle, bool buttonDown)
{
    uint16_t pwmValue = buttonDown ? hiPwmValue : loPwmValue;

    if (latchMode) {
        if (buttonDown) {
            latchButtonDown = !latchButtonDown;
            pwmValue = latchButtonDown ? hiPwmValue : loPwmValue;
        } else {
            return;
        }
    }
    vehicle->rcChannelOverride(rcChannel, pwmValue);
}

uint8_t JoystickRcOverride::channel() const
{
    return rcChannel;
};