/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.11
import QtQuick.Controls 1.2

import QGroundControl                       1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0

Item {
    id:     _root
    width:  labelBackground.width
    clip:   true

    property var  _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property bool _armed:           _activeVehicle ? _activeVehicle.armed : false

    readonly property real _topBottomMargins: ScreenTools.defaultFontPixelHeight * 0.5

    QGCPalette { id: qgcPal }

    Row {
        id:                         labelBackground
        spacing:                    0
        opacity:                    0.5
        anchors.top:                parent.top
        anchors.topMargin:          _topBottomMargins
        anchors.bottom:             parent.bottom
        anchors.bottomMargin:       _topBottomMargins
        Image {
            id:                     edge
            source:                 "/auterion/img/label_left_edge.svg"
            width:                  height
            antialiasing:           true
            sourceSize.height:      height
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            fillMode:               Image.PreserveAspectFit
        }
        Rectangle {
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            width:                  labelRow.width * 1.15
            color:                  "#000"
        }
        Image {
            source:                 "/auterion/img/label_right_edge.svg"
            width:                  height
            antialiasing:           true
            sourceSize.height:      height
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            fillMode:               Image.PreserveAspectFit
        }
    }
    Row {
        id:                         labelRow
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.centerIn:           parent
        QGCLabel {
            text:                   _armed ? qsTr("Armed") : qsTr("Disarmed")
            color:                  "#FFF"
            font.pointSize:         ScreenTools.smallFontPointSize
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            height:                 ScreenTools.defaultFontPixelHeight * 0.75
            width:                  height
            radius:                 height * 0.5
            color:                  _armed ? qgcPal.colorGreen : qgcPal.colorRed
            border.color:           "#FFF"
            border.width:           1
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    QGCMouseArea {
        fillItem: parent
        onClicked: _armed ? toolBar.disarmVehicle() : toolBar.armVehicle()
    }
}
