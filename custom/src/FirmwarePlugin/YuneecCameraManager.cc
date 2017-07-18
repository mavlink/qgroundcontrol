/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCApplication.h"
#include "YuneecCameraManager.h"

//-----------------------------------------------------------------------------
YuneecCameraManager::YuneecCameraManager(Vehicle *vehicle)
    : QGCCameraManager(vehicle)
{
}

//-----------------------------------------------------------------------------
QString
YuneecCameraManager::controllerSource()
{
    return QStringLiteral("");
}
