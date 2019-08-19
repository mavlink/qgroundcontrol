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
    //-- Check encoding
    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        auto* pCamera = qobject_cast<CustomCameraControl*>(_activeVehicle->dynamicCameras()->currentCameraInstance());
        if(pCamera) {
            Fact *fact = pCamera->videoEncoding();
            if (fact) {
                _videoReceiver->setVideoDecoder(static_cast<VideoReceiver::VideoEncoding>(fact->rawValue().toInt()));
            }
        }
    }
    VideoManager::_updateSettings();
}

