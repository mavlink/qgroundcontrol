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
    enumToString.insert(STABILIZE, QStringLiteral("Stabilize"));
    enumToString.insert(ACRO,      QStringLiteral("Acro"));
    enumToString.insert(ALT_HOLD,  QStringLiteral("Alt Hold"));
    enumToString.insert(AUTO,      QStringLiteral("Auto"));
    enumToString.insert(GUIDED,    QStringLiteral("Guided"));
    enumToString.insert(LOITER,    QStringLiteral("Loiter"));
    enumToString.insert(RTL,       QStringLiteral("RTL"));
    enumToString.insert(CIRCLE,    QStringLiteral("Circle"));
    enumToString.insert(POSITION,  QStringLiteral("Position"));
    enumToString.insert(LAND,      QStringLiteral("Land"));
    enumToString.insert(OF_LOITER, QStringLiteral("OF Loiter"));
    enumToString.insert(DRIFT,     QStringLiteral("Drift"));
    enumToString.insert(SPORT,     QStringLiteral("Sport"));
    enumToString.insert(FLIP,      QStringLiteral("Flip"));
    enumToString.insert(AUTOTUNE,  QStringLiteral("Autotune"));
    enumToString.insert(POS_HOLD,  QStringLiteral("Pos Hold"));
    enumToString.insert(BRAKE,     QStringLiteral("Brake"));

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
