/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.7
import QtMultimedia             5.5

import QGroundControl           1.0

Rectangle {
    anchors.fill:       parent
    color:              Qt.rgba(0,0,0,0.75)
    Camera {
        id:             camera
        deviceId:       QGroundControl.videoManager.videoSourceID
        captureMode:    Camera.CaptureViewfinder
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
