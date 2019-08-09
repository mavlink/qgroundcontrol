/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "QGCCameraManager.h"

//-----------------------------------------------------------------------------
class AuterionCameraManager : public QGCCameraManager
{
    Q_OBJECT
public:
    AuterionCameraManager(Vehicle* vehicle);
};
