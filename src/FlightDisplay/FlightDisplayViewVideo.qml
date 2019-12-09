/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                          2.11
import QtQuick.Controls                 2.4

import QGroundControl                   1.0
import QGroundControl.FlightDisplay     1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.QgcQtGStreamer    1.0
import QGroundControl.SettingsManager       1.0
import org.freedesktop.gstreamer.GLVideoItem 1.0

Item {
    id:     root
    clip:   true
    property double _ar:                QGroundControl.videoManager.aspectRatio
    property bool   _showGrid:          QGroundControl.settingsManager.videoSettings.gridLines.rawValue > 0
    property var    _dynamicCameras:    activeVehicle ? activeVehicle.dynamicCameras : null
    property bool   _connected:         activeVehicle ? !activeVehicle.connectionLost : false
    property int    _curCameraIndex:    _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property bool   _isCamera:          _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _camera:            _isCamera ? _dynamicCameras.cameras.get(_curCameraIndex) : null
    property bool   _hasZoom:           _camera && _camera.hasZoom
    property int    _fitMode:           QGroundControl.settingsManager.videoSettings.videoFit.rawValue

    property alias videoReceiver: videoSurface.videoReceiver
    property double _thermalHeightFactor: 0.85 //-- TODO

    Rectangle {
        anchors.fill:   parent
        color:          "black"
        visible:    true  //    _videoReceiver && _videoReceiver.videoRunning

        //-- Main Video controller.
        VideoSurface {
            id: videoSurface
            videoItem: video
            videoReceiver: QGroundControl.videoManager.videoReceiver
        }

        // Main Video.
        GstGLVideoItem {
            id: video
            anchors.centerIn: parent
            width: parent.width
            height: parent.height
            anchors.fill: parent
        }

        QGCPipable {
            id: videoControls
            anchors.fill: parent
            onPlay: {
                videoSurface.startVideo()
            }

            onPause: {
                videoSurface.pauseVideo()
            }
        }
    }
}
