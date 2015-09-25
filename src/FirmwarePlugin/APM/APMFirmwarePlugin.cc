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

#include "APMFirmwarePlugin.h"
#include "Generic/GenericFirmwarePlugin.h"

#include <QDebug>

IMPLEMENT_QGC_SINGLETON(APMFirmwarePlugin, FirmwarePlugin)

APMFirmwarePlugin::APMFirmwarePlugin(QObject* parent) :
    FirmwarePlugin(parent)
{
    
}

bool APMFirmwarePlugin::isCapable(FirmwareCapabilities capabilities)
{
    Q_UNUSED(capabilities);
    
    // FIXME: No capabilitis yet supported
    
    return false;
}

QList<VehicleComponent*> APMFirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);
    
    return QList<VehicleComponent*>();
}

QStringList APMFirmwarePlugin::flightModes(void)
{
    // FIXME: NYI
    
    qWarning() << "APMFirmwarePlugin::flightModes not supported";
    
    return QStringList();
}

QString APMFirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode)
{
    // FIXME: Nothing more than generic support yet
    return GenericFirmwarePlugin::instance()->flightMode(base_mode, custom_mode);
}

bool APMFirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    Q_UNUSED(flightMode);
    Q_UNUSED(base_mode);
    Q_UNUSED(custom_mode);
    
    qWarning() << "APMFirmwarePlugin::setFlightMode called on base class, not supported";
    
    return false;
}

int APMFirmwarePlugin::manualControlReservedButtonCount(void)
{
    // We don't know whether the firmware is going to used any of these buttons.
    // So reserve them all.
    return -1;
}

void APMFirmwarePlugin::adjustMavlinkMessage(mavlink_message_t* message)
{
    if (message->msgid == MAVLINK_MSG_ID_PARAM_VALUE) {
        mavlink_param_value_t paramValue;
        mavlink_param_union_t paramUnion;
        
        // APM stack passes all parameter values in mavlink_param_union_t.param_float no matter what
        // type they are. Fix that up to correct usage.
        
        mavlink_msg_param_value_decode(message, &paramValue);
        
        switch (paramValue.param_type) {
            case MAV_PARAM_TYPE_UINT8:
                paramUnion.param_uint8 = (uint8_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_INT8:
                paramUnion.param_int8 = (int8_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_UINT16:
                paramUnion.param_uint16 = (uint16_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_INT16:
                paramUnion.param_int16 = (int16_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_UINT32:
                paramUnion.param_uint32 = (uint32_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_INT32:
                paramUnion.param_int32 = (int32_t)paramValue.param_value;
                break;
            case MAV_PARAM_TYPE_REAL32:
                // Already in param_float
                break;
            default:
                qCritical() << "Invalid/Unsupported data type used in parameter:" << paramValue.param_type;
        }
        
        paramValue.param_value = paramUnion.param_float;
        
    } else if (message->msgid == MAVLINK_MSG_ID_PARAM_SET) {
        mavlink_param_set_t     paramSet;
        mavlink_param_union_t   paramUnion;
        
        // APM stack passes all parameter values in mavlink_param_union_t.param_float no matter what
        // type they are. Fix it back to the wrong way on the way out.
        
        mavlink_msg_param_set_decode(message, &paramSet);
        
        paramUnion.param_float = paramSet.param_value;

        switch (paramSet.param_type) {
            case MAV_PARAM_TYPE_UINT8:
                paramSet.param_value = paramUnion.param_uint8;
                break;
            case MAV_PARAM_TYPE_INT8:
                paramSet.param_value = paramUnion.param_int8;
                break;
            case MAV_PARAM_TYPE_UINT16:
                paramSet.param_value = paramUnion.param_uint16;
                break;
            case MAV_PARAM_TYPE_INT16:
                paramSet.param_value = paramUnion.param_int16;
                break;
            case MAV_PARAM_TYPE_UINT32:
                paramSet.param_value = paramUnion.param_uint32;
                break;
            case MAV_PARAM_TYPE_INT32:
                paramSet.param_value = paramUnion.param_int32;
                break;
            case MAV_PARAM_TYPE_REAL32:
                // Already in param_float
                break;
            default:
                qCritical() << "Invalid/Unsupported data type used in parameter:" << paramSet.param_type;
        }
    }

    // FIXME: Need to implement mavlink message severity adjustment
}
