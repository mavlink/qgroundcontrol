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

#ifndef APMFirmwarePlugin_H
#define APMFirmwarePlugin_H

#include "FirmwarePlugin.h"
#include "QGCLoggingCategory.h"
#include "ardupilotmega/ardupilotmega.h"

Q_DECLARE_LOGGING_CATEGORY(APMFirmwarePluginLog)

class APMFirmwareVersion
{
public:
    APMFirmwareVersion(const QString &versionText = "");
    bool isValid() const;
    bool isBeta() const;
    bool isDev() const;
    bool operator<(const APMFirmwareVersion& other) const;
    QString versionString() const { return _versionString; }
    QString vehicleType() const { return _vehicleType; }
    int majorNumber() const { return _major; }
    int minorNumber() const { return _minor; }
    int patchNumber() const { return _patch; }

private:
    void _parseVersion(const QString &versionText);
    QString _versionString;
    QString _vehicleType;
    int     _major;
    int     _minor;
    int     _patch;
};

/// This is the base class for all stack specific APM firmware plugins
class APMFirmwarePlugin : public FirmwarePlugin
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
    virtual void adjustMavlinkMessage(mavlink_message_t* message);
    virtual void initializeVehicle(Vehicle* vehicle);
    virtual const mavlink_message_info_t* mavlinkMessageInfo(void);
    
protected:
    /// All access to singleton is through stack specific implementation
    APMFirmwarePlugin(QObject* parent = NULL);
    
private:
    void _adjustSeverity(mavlink_message_t* message) const;
    static bool _isTextSeverityAdjustmentNeeded(const APMFirmwareVersion& firmwareVersion);

    APMFirmwareVersion _firmwareVersion;
    bool               _textSeverityAdjustmentNeeded;

};

#endif
