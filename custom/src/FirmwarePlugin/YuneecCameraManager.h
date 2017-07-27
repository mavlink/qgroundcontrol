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
    QString controllerSource        ();
};
