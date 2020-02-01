/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#pragma once

#define MAVLINK_USE_MESSAGE_INFO
#define MAVLINK_EXTERNAL_RX_STATUS  // Single m_mavlink_status instance is in QGCApplication.cc
#include <stddef.h>                 // Hack workaround for Mav 2.0 header problem with respect to offsetof usage
#include <mavlink_types.h>
extern mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];
#include <mavlink.h>

class QGCMAVLink {
public:
    static bool isFixedWing(MAV_TYPE mavType);
    static bool isRover(MAV_TYPE mavType);
    static bool isSub(MAV_TYPE mavType);
    static bool isMultiRotor(MAV_TYPE mavType);
    static bool isVTOL(MAV_TYPE mavType);
};


