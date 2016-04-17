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

#include "FirmwarePlugin.h"
#include "QGCApplication.h"

#include <QDebug>

const char* guided_mode_not_supported_by_vehicle = "Guided mode not supported by Vehicle.";

bool FirmwarePlugin::isCapable(FirmwareCapabilities capabilities)
{
    Q_UNUSED(capabilities);
    return false;
}

QList<VehicleComponent*> FirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);

    return QList<VehicleComponent*>();
}

QString FirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    QString flightMode;

    struct Bit2Name {
        uint8_t     baseModeBit;
        const char* name;
    };
    static const struct Bit2Name rgBit2Name[] = {
        { MAV_MODE_FLAG_MANUAL_INPUT_ENABLED,   "Manual" },
        { MAV_MODE_FLAG_STABILIZE_ENABLED,      "Stabilize" },
        { MAV_MODE_FLAG_GUIDED_ENABLED,         "Guided" },
        { MAV_MODE_FLAG_AUTO_ENABLED,           "Auto" },
        { MAV_MODE_FLAG_TEST_ENABLED,           "Test" },
    };

    Q_UNUSED(custom_mode);

    if (base_mode == 0) {
        flightMode = "PreFlight";
    } else if (base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        flightMode = QString("Custom:0x%1").arg(custom_mode, 0, 16);
    } else {
        for (size_t i=0; i<sizeof(rgBit2Name)/sizeof(rgBit2Name[0]); i++) {
            if (base_mode & rgBit2Name[i].baseModeBit) {
                if (i != 0) {
                    flightMode += " ";
                }
                flightMode += rgBit2Name[i].name;
            }
        }
    }

    return flightMode;
}

bool FirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    Q_UNUSED(flightMode);
    Q_UNUSED(base_mode);
    Q_UNUSED(custom_mode);

    qWarning() << "FirmwarePlugin::setFlightMode called on base class, not supported";

    return false;
}

int FirmwarePlugin::manualControlReservedButtonCount(void)
{
    // We don't know whether the firmware is going to used any of these buttons.
    // So reserve them all.
    return -1;
}

bool FirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(message);
    // Generic plugin does no message adjustment
    return true;
}

void FirmwarePlugin::adjustOutgoingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(message);
    // Generic plugin does no message adjustment
}

void FirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);

    // Generic Flight Stack is by definition "generic", so no extra work
}

bool FirmwarePlugin::sendHomePositionToVehicle(void)
{
    // Generic stack does not want home position sent in the first position.
    // Subsequent sequence numbers must be adjusted.
    // This is the mavlink spec default.
    return false;
}

QList<MAV_CMD> FirmwarePlugin::supportedMissionCommands(void)
{
    // Generic supports all commands
    return QList<MAV_CMD>();
}

void FirmwarePlugin::missionCommandOverrides(QString& commonJsonFilename, QString& fixedWingJsonFilename, QString& multiRotorJsonFilename) const
{
    // No overrides
    commonJsonFilename.clear();
    fixedWingJsonFilename.clear();
    multiRotorJsonFilename.clear();
}

void FirmwarePlugin::getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion)
{
    Q_UNUSED(metaDataFile);
    majorVersion = -1;
    minorVersion = -1;
}

bool FirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    return false;
}

void FirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(guidedMode);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

bool FirmwarePlugin::isPaused(const Vehicle* vehicle) const
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    return false;
}

void FirmwarePlugin::pauseVehicle(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeRTL(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double altitudeRel)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(altitudeRel);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(gotoCoord);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeRel)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(altitudeRel);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

const FirmwarePlugin::remapParamNameMajorVersionMap_t& FirmwarePlugin::paramNameRemapMajorVersionMap(void) const
{
    static const remapParamNameMajorVersionMap_t remap;

    return remap;
}

int FirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    Q_UNUSED(majorVersionNumber);
    return 0;
}
