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

#ifndef FirmwarePlugin_H
#define FirmwarePlugin_H

#include "QGCSingleton.h"
#include "QGCMAVLink.h"
#include "VehicleComponent.h"
#include "AutoPilotPlugin.h"

#include <QList>
#include <QString>

/// This is the base class for Firmware specific plugins
///
/// The FirmwarePlugin class is the abstract base class which represents the methods and objects
/// which are specific to a certain Firmware flight stack. This is the only place where
/// flight stack specific code should reside in QGroundControl. The remainder of the
/// QGroundControl source is generic to a common mavlink implementation. The implementation
/// in the base class supports mavlink generic firmware. Override the base clase virtuals
/// to create you firmware specific plugin.

class FirmwarePlugin : public QGCSingleton
{
    Q_OBJECT

public:
    /// Set of optional capabilites which firmware may support
    typedef enum {
        SetFlightModeCapability,
    } FirmwareCapabilities;
    
    /// @return true: Firmware supports all specified capabilites
    virtual bool isCapable(FirmwareCapabilities capabilities) = 0;
    
    /// Returns VehicleComponents for specified Vehicle
    ///     @param vehicle Vehicle  to associate with components
    /// @return List of VehicleComponents for the specified vehicle. Caller owns returned objects and must
    ///         free when no longer needed.
    virtual QList<VehicleComponent*> componentsForVehicle(AutoPilotPlugin* vehicle) = 0;
    
    /// Returns the list of available flight modes
    virtual QStringList flightModes(void) = 0;
    
    /// Returns the name for this flight mode. Flight mode names must be human readable as well as audio speakable.
    ///     @param base_mode Base mode from mavlink HEARTBEAT message
    ///     @param custom_mode Custom mode from mavlink HEARTBEAT message
    virtual QString flightMode(uint8_t base_mode, uint32_t custom_mode) = 0;
    
    /// Sets base_mode and custom_mode to specified flight mode.
    ///     @param[out] base_mode Base mode for SET_MODE mavlink message
    ///     @param[out] custom_mode Custom mode for SET_MODE mavlink message
    virtual bool setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) = 0;
    
protected:
    FirmwarePlugin(QObject* parent = NULL) : QGCSingleton(parent) { }
};

#endif
