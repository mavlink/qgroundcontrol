/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- Battery Indicator
Item {
    width:                  batteryIndicatorRow.width
    anchors.top:            parent.top
    anchors.bottom:         parent.bottom
    anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
    anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.25

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    function getBatteryColor() {
        if(_activeVehicle) {
            if(_activeVehicle.battery.percentRemaining.value > 75) {
                return qgcPal.text
            }
            if(_activeVehicle.battery.percentRemaining.value > 50) {
                return qgcPal.colorOrange
            }
            if(_activeVehicle.battery.percentRemaining.value > 0.1) {
                return qgcPal.colorRed
            }
        }
        return qgcPal.colorGrey
    }

    function getBatteryPercentageText() {
        if(_activeVehicle) {
            if(_activeVehicle.battery.percentRemaining.value > 98.9) {
                return "100%"
            }
            if(_activeVehicle.battery.percentRemaining.value > 0.1) {
                return _activeVehicle.battery.percentRemaining.valueString + _activeVehicle.battery.percentRemaining.units
            }
            if(_activeVehicle.battery.voltage.value >= 0) {
                return _activeVehicle.battery.voltage.valueString + _activeVehicle.battery.voltage.units
            }
        }
        return "N/A"
    }

    Component {
        id: batteryInfo

        Rectangle {
            width:  battCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: battCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  Qt.rgba(0,0,0,0.75)

            Column {
                id:                 battCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(battGrid.width, battLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             battLabel
                    text:           qsTr("Battery Status")
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
                if((x + width) > toolBar.width) {
                    x = toolBar.width - width - ScreenTools.defaultFontPixelWidth
                }
            }
        }
    }

    Row {
        id:             batteryIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        opacity:        (_activeVehicle && _activeVehicle.battery.voltage.value >= 0) ? 1 : 0.5
        spacing:        ScreenTools.defaultFontPixelWidth
        Image {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              height
            sourceSize.width:   width
            source:             "/auterion/img/menu_battery.svg"
            fillMode:           Image.PreserveAspectFit
            Rectangle {
                color:              "#FFFFFF"
                anchors.left:       parent.left
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.25
                height:             parent.height * 0.35
                width:              _activeVehicle ? (_activeVehicle.battery.percentRemaining.value / 100) * parent.width * 0.75 : 0
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        QGCLabel {
            text:                   getBatteryPercentageText()
            font.pointSize:         ScreenTools.smallFontPointSize
            color:                  getBatteryColor()
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showPopUp(batteryInfo, mapToItem(toolBar, x, y).x + (width / 2))
    }
}
