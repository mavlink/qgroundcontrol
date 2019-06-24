/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "QGCApplication.h"
#include "CustomCameraManager.h"

//-----------------------------------------------------------------------------
CustomCameraManager::CustomCameraManager(Vehicle *vehicle)
    : QGCCameraManager(vehicle)
{
}
