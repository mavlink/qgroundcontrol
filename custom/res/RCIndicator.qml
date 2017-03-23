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

import TyphoonHQuickInterface               1.0

//-------------------------------------------------------------------------
//-- RC Wignal/Battery Indicator
Item {
    width:          rcRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    function getBatteryColor() {
        if(TyphoonHQuickInterface.rcBattery > 0.75) {
            return qgcPal.colorGreen
        }
        if(TyphoonHQuickInterface.rcBattery > 0.5) {
            return qgcPal.colorOrange
        }
        if(TyphoonHQuickInterface.rcBattery > 0.1) {
            return qgcPal.colorRed
        }
        return qgcPal.text
    }

    function getBatteryPercentageText() {
        if(TyphoonHQuickInterface.rcBattery > 0.98) {
            return "100%"
        }
        if(TyphoonHQuickInterface.rcBattery > 0.1) {
            return (TyphoonHQuickInterface.rcBattery * 100).toFixed(0) + '%'
        }
        return "N/A"
    }

    function getBatteryIcon() {
        if(TyphoonHQuickInterface.rcBattery > 0.95) {
            return "/typhoonh/battery_100.svg"
        } else if(TyphoonHQuickInterface.rcBattery > 0.75) {
            return "/typhoonh/battery_80.svg"
        } else if(TyphoonHQuickInterface.rcBattery > 0.55) {
            return "/typhoonh/battery_60.svg"
        } else if(TyphoonHQuickInterface.rcBattery > 0.35) {
            return "/typhoonh/battery_40.svg"
        } else if(TyphoonHQuickInterface.rcBattery > 0.15) {
            return "/typhoonh/battery_20.svg"
        }
        return "/typhoonh/battery_0.svg"
    }

    Component {
        id: rcInfo

        Rectangle {
            width:  rcCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: rcCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 rcCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(rcrssiGrid.width, rssiLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             rssiLabel
                    text:           qsTr("RC Status")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 rcrssiGrid
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    columns:            2
                    anchors.horizontalCenter: parent.horizontalCenter

                    QGCLabel { text: qsTr("RSSI:") }
                    QGCLabel { text: _activeVehicle ? (_activeVehicle.rcRSSI + "%") : 'N/A' }
                    QGCLabel { text: qsTr("Battery:") }
                    QGCLabel { text: getBatteryPercentageText() }
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
        id:             rcRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth
        QGCColoredImage {
            anchors.top:        parent.top
            width:              height
            anchors.bottom:     parent.bottom
            sourceSize.height:  height
            source:             "/qmlimages/RC.svg"
            fillMode:           Image.PreserveAspectFit
            opacity:            _activeVehicle ? 1 : 0.5
            color:              qgcPal.buttonText
        }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            SignalStrength {
                size:                   rcRow.height * 0.5
                percent:                _activeVehicle ? ((_activeVehicle.rcRSSI > 100) ? 0 : _activeVehicle.rcRSSI) : 0
                opacity:                _activeVehicle ? 1 : 0.5
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Row {
                height:                 rcRow.height * 0.5
                spacing:                ScreenTools.defaultFontPixelWidth
                Image {
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    width:              height
                    sourceSize.width:   width
                    source:             getBatteryIcon()
                    fillMode:           Image.PreserveAspectFit
                }
                QGCLabel {
                    text:                   getBatteryPercentageText()
                    color:                  getBatteryColor()
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked: {
            var centerX = mapToItem(toolBar, x, y).x + (width / 2)
            mainWindow.showPopUp(rcInfo, centerX)
        }
    }
}
