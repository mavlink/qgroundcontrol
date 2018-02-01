/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "QGCCameraManager.h"

//-----------------------------------------------------------------------------
class YuneecCameraManager : public QGCCameraManager
{
    Q_OBJECT
public:
    YuneecCameraManager(Vehicle* vehicle);
protected:
#if defined (__planner__)
    void    _requestCameraInfo      (int compID) override;
    void    _handleHeartbeat        (const mavlink_message_t& message) override;
#endif

};
