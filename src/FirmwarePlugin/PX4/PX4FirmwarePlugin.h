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
///     @author Don Gagne <don@thegagnes.com>

#ifndef PX4FirmwarePlugin_H
#define PX4FirmwarePlugin_H

#include "FirmwarePlugin.h"
#include "PX4ParameterMetaData.h"

class PX4FirmwarePlugin : public FirmwarePlugin
{
    Q_OBJECT

public:
    // Overrides from FirmwarePlugin
    virtual bool isCapable(FirmwareCapabilities capabilities);
    virtual QList<VehicleComponent*> componentsForVehicle(AutoPilotPlugin* vehicle);
    virtual QStringList flightModes(void);
    virtual QString flightMode(uint8_t base_mode, uint32_t custom_mode);
    virtual bool setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode);
    virtual int manualControlReservedButtonCount(void);
    virtual void adjustMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message);
    virtual void initializeVehicle(Vehicle* vehicle);
    virtual bool sendHomePositionToVehicle(void);
    virtual void addMetaDataToFact(Fact* fact, MAV_TYPE vehicleType);
    virtual QString getDefaultComponentIdParam(void) const { return QString("SYS_AUTOSTART"); }
    virtual QList<MAV_CMD> supportedMissionCommands(void);

private:
    PX4ParameterMetaData    _parameterMetaData;
};

#endif
