/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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
///     @author Don Gagne <don@thegagnes.com>

#include "ArduCopterFirmwarePlugin.h"
#include "Generic/GenericFirmwarePlugin.h"

APMCopterMode::APMCopterMode(uint32_t mode, bool settable) :
    APMCustomMode(mode, settable)
{
    QMap<uint32_t,QString> enumToString;
    enumToString.insert(STABILIZE, "Stabilize");
    enumToString.insert(ACRO,      "Acro");
    enumToString.insert(ALT_HOLD,  "Alt Hold");
    enumToString.insert(AUTO,      "Auto");
    enumToString.insert(GUIDED,    "Guided");
    enumToString.insert(LOITER,    "Loiter");
    enumToString.insert(RTL,       "RTL");
    enumToString.insert(CIRCLE,    "Circle");
    enumToString.insert(POSITION,  "Position");
    enumToString.insert(LAND,      "Land");
    enumToString.insert(OF_LOITER, "OF Loiter");
    enumToString.insert(DRIFT,     "Drift");
    enumToString.insert(SPORT,     "Sport");
    enumToString.insert(FLIP,      "Flip");
    enumToString.insert(AUTOTUNE,  "Autotune");
    enumToString.insert(POS_HOLD,  "Pos Hold");
    enumToString.insert(BRAKE,     "Brake");

    setEnumToStringMapping(enumToString);
}

ArduCopterFirmwarePlugin::ArduCopterFirmwarePlugin(void)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMCopterMode(APMCopterMode::STABILIZE ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::ACRO      ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::ALT_HOLD  ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::AUTO      ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::GUIDED    ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::LOITER    ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::RTL       ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::CIRCLE    ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::POSITION  ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::LAND      ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::OF_LOITER ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::DRIFT     ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::SPORT     ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::FLIP      ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::AUTOTUNE  ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::POS_HOLD  ,true);
    supportedFlightModes << APMCopterMode(APMCopterMode::BRAKE     ,true);
    setSupportedModes(supportedFlightModes);
}
