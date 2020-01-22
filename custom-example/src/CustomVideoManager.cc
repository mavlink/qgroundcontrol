/****************************************************************************
 *
 *   (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CustomVideoManager.h"
#include "MultiVehicleManager.h"
#include "CustomCameraManager.h"
#include "CustomCameraControl.h"

//-----------------------------------------------------------------------------
CustomVideoManager::CustomVideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : VideoManager(app, toolbox)
{
}

//-----------------------------------------------------------------------------
void
CustomVideoManager::_updateSettings()
{
    if(!_videoSettings || !_videoReceiver)
        return;
    VideoManager::_updateSettings();
}

