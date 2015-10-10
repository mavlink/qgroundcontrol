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
#include "QGCMAVLink.h"

#include <QDebug>

IMPLEMENT_QGC_SINGLETON(ArduCopterFirmwarePlugin, ArduCopterFirmwarePlugin)

ArduCopterFirmwarePlugin::ArduCopterFirmwarePlugin(QObject* parent) :
    APMFirmwarePlugin(parent)
{

}

bool ArduCopterFirmwarePlugin::isCapable(FirmwareCapabilities capabilities)
{
    Q_UNUSED(capabilities);
    
    // FIXME: No capabilitis yet supported
    
    return false;
}

QList<VehicleComponent*> ArduCopterFirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);
    
    return QList<VehicleComponent*>();
}

QStringList ArduCopterFirmwarePlugin::flightModes(void)
{
    // FIXME: NYI
    
    qWarning() << "ArduCopterFirmwarePlugin::flightModes not supported";
    
    return QStringList();
}

QString ArduCopterFirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode)
{
    // FIXME: Nothing more than generic support yet
    return GenericFirmwarePlugin::instance()->flightMode(base_mode, custom_mode);
}

bool ArduCopterFirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    Q_UNUSED(flightMode);
    Q_UNUSED(base_mode);
    Q_UNUSED(custom_mode);
    
    qWarning() << "ArduCopterFirmwarePlugin::setFlightMode called on base class, not supported";
    
    return false;
}
