/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCApplication.h"
#include "AuterionCameraManager.h"

//-----------------------------------------------------------------------------
AuterionCameraManager::AuterionCameraManager(Vehicle *vehicle)
    : QGCCameraManager(vehicle)
{
}
