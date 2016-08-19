/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief MAVLink header file for QGroundControl
 *   @author Lorenz Meier <pixhawk@switched.com>
 */

#ifndef QGCMAVLINK_H
#define QGCMAVLINK_H

#include <mavlink.h>

class QGCMAVLink {
public:
    static bool isFixedWing(MAV_TYPE mavType);
    static bool isRover(MAV_TYPE mavType);
    static bool isSub(MAV_TYPE mavType);
    static bool isMultiRotor(MAV_TYPE mavType);
    static bool isVTOL(MAV_TYPE mavType);
};

#endif // QGCMAVLINK_H

