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

import Auterion.Widgets             1.0

Item {
    id:         _root
    width:      background.width
    height:     background.height

    property var  _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property bool _armed:           _activeVehicle ? _activeVehicle.armed : false

    QGCPalette { id: qgcPal }

    AuterionTextBackground {
        id:                         background
        contentWidth:               labelRow.width
        contentHeight:              labelText.height * 2
        opacity:                    0.75
        anchors.verticalCenter:     parent.verticalCenter
    }
    Row {
        id:                         labelRow
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.centerIn:           parent
        QGCLabel {
            id:                     labelText
            text:                   _armed ? qsTr("Armed") : qsTr("Disarmed")
            color:                  "#FFF"
            font.pointSize:         ScreenTools.defaultFontPointSize
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            height:                 Math.round(ScreenTools.defaultFontPixelHeight * 0.75)
            width:                  height
            radius:                 height * 0.5
            color:                  Qt.rgba(0,0,0,0)
            border.color:           "#FFF"
            border.width:           1
            anchors.verticalCenter: parent.verticalCenter
            Rectangle {
                height:                 Math.round(parent.height * 0.5)
                width:                  height
                radius:                 height * 0.5
                color:                  _armed ? qgcPal.colorGreen : qgcPal.colorRed
                anchors.centerIn:       parent
            }
        }
    }
    QGCMouseArea {
        fillItem: parent
        onClicked: _armed ? toolBar.disarmVehicle() : toolBar.armVehicle()
    }
}
