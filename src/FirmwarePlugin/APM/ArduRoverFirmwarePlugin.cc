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
///     @author Pritam Ghanghas <pritam.ghanghas@gmail.com>

#include "ArduRoverFirmwarePlugin.h"
#include "Generic/GenericFirmwarePlugin.h"

APMRoverMode::APMRoverMode(uint32_t mode, bool settable)
    : APMCustomMode(mode, settable)
{
    QMap<uint32_t,QString> enumToString;
    enumToString.insert(MANUAL,         QStringLiteral("Manual"));
    enumToString.insert(LEARNING,       QStringLiteral("Learning"));
    enumToString.insert(STEERING,       QStringLiteral("Steering"));
    enumToString.insert(HOLD,           QStringLiteral("Hold"));
    enumToString.insert(AUTO,           QStringLiteral("Auto"));
    enumToString.insert(RTL,            QStringLiteral("RTL"));
    enumToString.insert(GUIDED,         QStringLiteral("Guided"));
    enumToString.insert(INITIALIZING,   QStringLiteral("Initializing"));

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
