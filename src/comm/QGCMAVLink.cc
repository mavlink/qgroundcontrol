/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMAVLink.h"

bool QGCMAVLink::isFixedWing(MAV_TYPE mavType)
{
    return mavType == MAV_TYPE_FIXED_WING;
}

bool QGCMAVLink::isRover(MAV_TYPE mavType)
{
    return mavType == MAV_TYPE_GROUND_ROVER;
}

bool QGCMAVLink::isSub(MAV_TYPE mavType)
{
    return mavType == MAV_TYPE_SUBMARINE;
}

bool QGCMAVLink::isMultiRotor(MAV_TYPE mavType)
{
    switch (mavType) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
        return true;
    default:
        return false;
    }
}

bool QGCMAVLink::isVTOL(MAV_TYPE mavType)
{
    switch (mavType) {
    case MAV_TYPE_VTOL_DUOROTOR:
    case MAV_TYPE_VTOL_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_RESERVED2:
    case MAV_TYPE_VTOL_RESERVED3:
    case MAV_TYPE_VTOL_RESERVED4:
    case MAV_TYPE_VTOL_RESERVED5:
        return true;
    default:
        return false;
    }
}
