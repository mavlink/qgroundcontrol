/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick                              2.11
import QtQuick.Controls                     1.2

import QGroundControl                       1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0

//-------------------------------------------------------------------------
//-- Armed Indicator
Rectangle {
    anchors.top:                    parent.top
    anchors.bottom:                 parent.bottom
    width:                          labelRow.width + (ScreenTools.defaultFontPixelWidth * 6)
    color:                          qgcPal.windowShade

    property bool _armed:           activeVehicle ? activeVehicle.armed : false

    Row {
        id:                         labelRow
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.centerIn:           parent
        QGCLabel {
            id:                     labelText
            text:                   _armed ? qsTr("Armed") : qsTr("Disarmed")
            color:                  qgcPal.text
            font.pointSize:         ScreenTools.defaultFontPointSize
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            height:                 ScreenTools.defaultFontPixelHeight * 0.5
            width:                  height
            radius:                 height * 0.5
            color:                  _armed ? qgcPal.colorGreen : qgcPal.colorRed
            border.color:           qgcPal.window
            border.width:           1
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    QGCMouseArea {
        fillItem: parent
        onClicked: _armed ? mainWindow.disarmVehicle() : mainWindow.armVehicle()
    }
}
