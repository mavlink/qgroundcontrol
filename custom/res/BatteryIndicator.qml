/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
import QtQuick.Controls     1.2
import QtQuick.Layouts      1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

import TyphoonHQuickInterface.Widgets       1.0

//-------------------------------------------------------------------------
//-- Battery Indicator
Item {
    id:             _root
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          batRow.width

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property real   _batteryLevel:  _activeVehicle ? _activeVehicle.battery.percentRemaining.value / 100.0 : 0.0
    property color  _batteryColor:  _batteryLevel > 0.0 ? Qt.hsva(0.333333 * _batteryLevel, 1, 1, 1) : qgcPal.buttonText

    function getBatteryPercentageText() {
        if(_activeVehicle) {
            if(_batteryLevel > 0.989) {
                return "100%"
            }
            if(_batteryLevel > 0.01) {
                return _activeVehicle.battery.percentRemaining.valueString + _activeVehicle.battery.percentRemaining.units
            }
        }
        return qsTr("N/A")
    }

    function getBatteryImage() {
        if(_activeVehicle) {
            if(_batteryLevel > 0.8) {
                return "/typhoonh/img/battery_charge_100.svg"
            }
            if(_batteryLevel > 0.6) {
                return "/typhoonh/img/battery_charge_80.svg"
            }
            if(_batteryLevel > 0.4) {
                return "/typhoonh/img/battery_charge_60.svg"
            }
            if(_batteryLevel > 0.2) {
                return "/typhoonh/img/battery_charge_40.svg"
            }
            return "/typhoonh/img/battery_charge_20.svg"
        }
        return ""
    }

    QGCLabel {
        id:                     percentLabel
        text:                   "100%"
        font.pointSize:         ScreenTools.mediumFontPointSize
        visible:                false
    }

    Component {
        id: batteryInfo

        Rectangle {
            width:  battCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: battCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 battCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(battGrid.width, battLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             battLabel
                    text:           qsTr("Vehicle Battery Status")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 battGrid
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    columns:            2
                    anchors.horizontalCenter: parent.horizontalCenter

                    QGCLabel { text: qsTr("Voltage:") }
                    QGCLabel { text: (_activeVehicle && _activeVehicle.battery.voltage.value != -1) ? (_activeVehicle.battery.voltage.valueString + " " + _activeVehicle.battery.voltage.units) : "N/A" }
                    QGCLabel { text: qsTr("Accumulated Consumption:") }
                    QGCLabel { text: (_activeVehicle && _activeVehicle.battery.mahConsumed.value != -1) ? (_activeVehicle.battery.mahConsumed.valueString + " " + _activeVehicle.battery.mahConsumed.units) : "N/A" }
                }
            }

            Component.onCompleted: {
                var pos = mapFromItem(toolBar, centerX - (width / 2), toolBar.height)
                x = pos.x
                y = pos.y + ScreenTools.defaultFontPixelHeight
            }
        }
    }

    Row {
        id:             batRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth
        QGCColoredImage {
            width:              parent.height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             "/typhoonh/img/battery_back.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.height:  height
            opacity:            (activeVehicle && activeVehicle.gps.count.value >= 0) ? 1 : 0.5
            color:              qgcPal.text
            QGCColoredImage {
                anchors.fill:       parent
                source:             getBatteryImage()
                fillMode:           Image.PreserveAspectFit
                sourceSize.width:   width
                color:              _batteryColor
            }
            QGCColoredImage {
                anchors.fill:       parent
                source:             "/typhoonh/img/battery_plane.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
            }
        }
        QGCLabel {
            text:       getBatteryPercentageText()
            width:      percentLabel.width
            color:      qgcPal.buttonText
            font.pointSize: ScreenTools.mediumFontPointSize
            horizontalAlignment:    Text.AlignLeft
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showPopUp(batteryInfo, mapToItem(toolBar, x, y).x + (width / 2))
    }
}
