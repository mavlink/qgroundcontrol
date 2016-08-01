/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Rustom Jehangir <rusty@bluerobotics.com>

#ifndef ArduSubFirmwarePlugin_H
#define ArduSubFirmwarePlugin_H

#include "APMFirmwarePlugin.h"

class APMSubMode : public APMCustomMode
{
public:
    enum Mode {
        STABILIZE         = 0,   // Hold level position
        RESERVED_1        = 1,
        ALT_HOLD          = 2,   // Depth hold
        RESERVED_3        = 3,
        RESERVED_4        = 4,
        RESERVED_5        = 5,
        RESERVED_6        = 6,
        RESERVED_7        = 7,
        RESERVED_8        = 8,
        RESERVED_9        = 9,
        RESERVED_10       = 10,
        RESERVED_11       = 11,
        RESERVED_12       = 12,
        RESERVED_13       = 13,
        RESERVED_14       = 14,
        RESERVED_15       = 15,
        RESERVED_16       = 16,
        RESERVED_17       = 17,
        RESERVED_18       = 18,
        MANUAL            = 19
    };
    static const int modeCount = 20;

    APMSubMode(uint32_t mode, bool settable);
};

class ArduSubFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT

public:
    ArduSubFirmwarePlugin(void);

    // Overrides from FirmwarePlugin
    int manualControlReservedButtonCount(void);

    bool supportsThrottleModeCenterZero(void);

    bool supportsManualControl(void);

    bool supportsRadio(void);

    bool supportsJSButton(void);
};

#endif
