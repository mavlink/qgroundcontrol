/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
