/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtMultimedia             5.5

import QGroundControl           1.0

Rectangle {
    id:                 _root
    width:              parent.width
    height:             parent.height
    color:              Qt.rgba(0,0,0,0.75)
    clip:               true
    anchors.centerIn:   parent

    function adjustAspectRatio()
    {
        //-- Set aspect ratio
        var size = camera.viewfinder.resolution
        if(size.height > 0 && size.width > 0) {
            var ar = size.width / size.height
            _root.height = parent.height * ar
        }
    }

    Camera {
        id:             camera
        deviceId:       QGroundControl.videoManager.videoSourceID
        captureMode:    Camera.CaptureViewfinder
        onDeviceIdChanged: {
            adjustAspectRatio()
        }
        onCameraStateChanged: {
            if(camera.cameraStatus === Camera.ActiveStatus) {
                adjustAspectRatio()
            }
        }
    }
    VideoOutput {
        source:         camera
        anchors.fill:   parent
        fillMode:       VideoOutput.PreserveAspectCrop
        visible:        !QGroundControl.videoManager.isGStreamer
    }
    onVisibleChanged: {
        if(visible)
            camera.start()
        else
            camera.stop()
    }
}
