/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief MAVLink header file for QGroundControl
 *   @author Lorenz Meier <pixhawk@switched.com>
 */

#ifndef QGCMAVLINK_H
#define QGCMAVLINK_H

#include <mavlink_types.h>
#include <mavlink.h>

#ifdef QGC_USE_PIXHAWK_MESSAGES
#include <pixhawk.h>
#endif

#ifdef QGC_USE_SLUGS_MESSAGES
#include <slugs.h>
#endif

#ifdef QGC_USE_UALBERTA_MESSAGES
#include <ualberta.h>
#endif

#ifdef QGC_USE_ARDUPILOTMEGA_MESSAGES
#include <ardupilotmega.h>
#endif


#endif // QGCMAVLINK_H

