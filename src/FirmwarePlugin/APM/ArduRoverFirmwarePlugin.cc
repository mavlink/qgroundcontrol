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

#include "ArduRoverFirmwarePlugin.h"

APMRoverMode::APMRoverMode(uint32_t mode, bool settable)
    : APMCustomMode(mode, settable)
{
    QMap<uint32_t,QString> enumToString;
    enumToString.insert(MANUAL,         "Manual");
    enumToString.insert(LEARNING,       "Learning");
    enumToString.insert(STEERING,       "Steering");
    enumToString.insert(HOLD,           "Hold");
    enumToString.insert(AUTO,           "Auto");
    enumToString.insert(RTL,            "RTL");
    enumToString.insert(GUIDED,         "Guided");
    enumToString.insert(INITIALIZING,   "Initializing");

    setEnumToStringMapping(enumToString);
}

ArduRoverFirmwarePlugin::ArduRoverFirmwarePlugin(void)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMRoverMode(APMRoverMode::MANUAL       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::LEARNING     ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::STEERING     ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::HOLD         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::AUTO         ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::RTL          ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::GUIDED       ,true);
    supportedFlightModes << APMRoverMode(APMRoverMode::INITIALIZING ,false);
    setSupportedModes(supportedFlightModes);
}
