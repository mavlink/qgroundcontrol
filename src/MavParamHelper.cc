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

#include "MavParamHelper.h"

void MavParamVariantToUnion(const QVariant& varParam, MAV_PARAM_TYPE type, mavlink_param_union_t& paramUnion)
{
    bool cvtOk;
    int intParam;
    
    switch (type) {
        case MAV_PARAM_TYPE_UINT8:
        case MAV_PARAM_TYPE_UINT16:
        case MAV_PARAM_TYPE_UINT32:
        case MAV_PARAM_TYPE_INT8:
        case MAV_PARAM_TYPE_INT16:
        case MAV_PARAM_TYPE_INT32:
            intParam = varParam.toInt(&cvtOk);
            Q_ASSERT(cvtOk);
            switch (type) {
                case MAV_PARAM_TYPE_UINT8:
                    Q_ASSERT(!(intParam & 0xFFFFFF00));
                    paramUnion.param_uint8 = (uint8_t)intParam;
                    break;
                    
                case MAV_PARAM_TYPE_UINT16:
                    Q_ASSERT(!(intParam & 0x7FFF0000));
                    paramUnion.param_uint16 = (uint16_t)intParam;
                    break;
                    
                case MAV_PARAM_TYPE_UINT32:
                    Q_ASSERT(intParam >= 0);
                    paramUnion.param_uint32 = (uint32_t)intParam;
                    break;
                    
                case MAV_PARAM_TYPE_INT8:
                    Q_ASSERT(!(intParam & 0x7FFFFF00));
                    paramUnion.param_int8 = (int8_t)intParam;
                    break;
                    
                case MAV_PARAM_TYPE_INT16:
                    Q_ASSERT(!(intParam & 0x7FFF0000));
                    paramUnion.param_int16 = (uint16_t)intParam;
                    break;
                    
                case MAV_PARAM_TYPE_INT32:
                    paramUnion.param_int32 = (int32_t)intParam;
                    break;
                    
                default:
                    // We shouldn't get here
                    Q_ASSERT(false);
                    break;
            }
            break;
            
        case MAV_PARAM_TYPE_REAL32:
            paramUnion.param_float = varParam.toFloat(&cvtOk);
            Q_ASSERT(cvtOk);
            break;
            
        default:
            // Type we don't handle
            Q_ASSERT(false);
            break;
    }
                      
    paramUnion.type = type;
}

QVariant MavParamUnionToVariant(const mavlink_param_union_t& paramUnion)
{
    switch (paramUnion.type) {
        case MAV_PARAM_TYPE_UINT8:
            return QVariant::fromValue(paramUnion.param_uint8);
            
        case MAV_PARAM_TYPE_UINT16:
            return QVariant::fromValue(paramUnion.param_uint16);
            break;
            
        case MAV_PARAM_TYPE_UINT32:
            return QVariant::fromValue(paramUnion.param_uint32);
            break;
            
        case MAV_PARAM_TYPE_INT8:
            return QVariant::fromValue(paramUnion.param_int8);
            break;
            
        case MAV_PARAM_TYPE_INT16:
            return QVariant::fromValue(paramUnion.param_int16);
            break;
            
        case MAV_PARAM_TYPE_INT32:
            return QVariant::fromValue(paramUnion.param_int32);
            break;
            
        case MAV_PARAM_TYPE_REAL32:
            return QVariant::fromValue(paramUnion.param_float);
            break;
            
        default:
            // Type we don't handle
            Q_ASSERT(false);
            return QVariant();
    }
}
