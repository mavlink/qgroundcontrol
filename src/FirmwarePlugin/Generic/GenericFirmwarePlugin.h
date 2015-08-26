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

#ifndef GenericFirmwarePlugin_H
#define GenericFirmwarePlugin_H

#include "FirmwarePlugin.h"

class GenericFirmwarePlugin : public FirmwarePlugin
{
    Q_OBJECT

    DECLARE_QGC_SINGLETON(GenericFirmwarePlugin, FirmwarePlugin)
    
public:
    // Overrides from FirmwarePlugin
    
    virtual bool isCapable(FirmwareCapabilities capabilities) { Q_UNUSED(capabilities); return false; }
    virtual QList<VehicleComponent*> componentsForVehicle(AutoPilotPlugin* vehicle);
    virtual QStringList flightModes(void) { return QStringList(); }
    virtual QString flightMode(uint8_t base_mode, uint32_t custom_mode);
    virtual bool setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode);
    
private:
    /// All access to singleton is through AutoPilotPluginManager::instance
    GenericFirmwarePlugin(QObject* parent = NULL);
};

#endif
