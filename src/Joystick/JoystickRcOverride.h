//
// Created by zdanek on 08.07.2022.
//

#pragma once

#include "Vehicle.h"

class JoystickRcOverride
{
public:
    JoystickRcOverride(const uint8_t rcChannel,
                       const uint16_t loPwmValue,
                       const uint16_t hiPwmValue,
                       bool latch);
    void send(Vehicle* vehicle, bool buttonDown);
    uint8_t channel() const;

private:
    uint8_t rcChannel;
    uint16_t loPwmValue;
    uint16_t hiPwmValue;
    bool latchMode;
    bool latchButtonDown = false;
};

