/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#pragma once

#include "vehicle_gps_position.h"
#include "satellite_info.h"
#include <QMetaType>

/**
 ** struct GPSPositionMessage
 * wrapper that can be used for Qt signal/slots
 */
struct GPSPositionMessage
{
    vehicle_gps_position_s position_data;
};

Q_DECLARE_METATYPE(GPSPositionMessage);


struct GPSSatelliteMessage
{
    satellite_info_s satellite_data;
};
Q_DECLARE_METATYPE(GPSSatelliteMessage);
