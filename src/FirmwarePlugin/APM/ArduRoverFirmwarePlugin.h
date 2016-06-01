/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
