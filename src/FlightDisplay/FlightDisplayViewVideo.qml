/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.4
import QtQuick.Controls             1.3

import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0


Item {
    id: root
    Rectangle {
        id:             noVideo
        anchors.fill:   parent
        color:          Qt.rgba(0,0,0,0.75)
        visible:        !_controller.videoRunning
        QGCLabel {
            text:               qsTr("NO VIDEO")
            font.family:        ScreenTools.demiboldFontFamily
            color:              "white"
            font.pointSize:     _mainIsMap ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
            anchors.centerIn:   parent
        }
    }
    QGCVideoBackground {
        anchors.fill:   parent
        display:        _controller.videoSurface
        receiver:       _controller.videoReceiver
        visible:        _controller.videoRunning
        runVideo:       true
        /* TODO: Come up with a way to make this an option
        QGCAttitudeHUD {
            id:                 attitudeHUD
            visible:            !_mainIsMap
            rollAngle:          _roll
            pitchAngle:         _pitch
            width:              ScreenTools.defaultFontPixelHeight * (30)
            height:             ScreenTools.defaultFontPixelHeight * (30)
            active:             QGroundControl.multiVehicleManager.activeVehicleAvailable
            z:                  QGroundControl.zOrderWidgets
        }
        */
    }
}
