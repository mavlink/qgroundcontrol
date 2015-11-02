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
///     @author Pritam Ghanghas <pritam.ghanghas@gmail.com>

#ifndef ArduRoverFirmwarePlugin_H
#define ArduRoverFirmwarePlugin_H

#include "APMFirmwarePlugin.h"

class APMRoverMode : public APMCustomMode
{
public:
    enum Mode {
        MANUAL        = 0,
        RESERVED_1    = 1, // RESERVED FOR FUTURE USE
        LEARNING      = 2,
        STEERING      = 3,
        HOLD          = 4,
        RESERVED_5    = 5, // RESERVED FOR FUTURE USE
        RESERVED_6    = 6, // RESERVED FOR FUTURE USE
        RESERVED_7    = 7, // RESERVED FOR FUTURE USE
        RESERVED_8    = 8, // RESERVED FOR FUTURE USE
        RESERVED_9    = 9, // RESERVED FOR FUTURE USE
        AUTO          = 10,
        RTL           = 11,
        RESERVED_12   = 12, // RESERVED FOR FUTURE USE
        RESERVED_13   = 13, // RESERVED FOR FUTURE USE
        RESERVED_14   = 14, // RESERVED FOR FUTURE USE
        GUIDED        = 15,
        INITIALIZING  = 16,
    };
    static const int modeCount = 17;

    APMRoverMode(uint32_t mode, bool settable);
};

class ArduRoverFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT
    
public:
    ArduRoverFirmwarePlugin(void);
};

#endif
