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
#if defined (__planner__)
void
YuneecCameraManager::_requestCameraInfo(int compID)
{
    //-- For DataPlanner, we don't want to let it create a camera instance
    Q_UNUSED(compID);
}
#endif

//-----------------------------------------------------------------------------
#if defined (__planner__)
void
YuneecCameraManager::_handleHeartbeat(const mavlink_message_t &message)
{
    //-- For DataPlanner, we don't care about camera heartbeats
    Q_UNUSED(message);
}
#endif
