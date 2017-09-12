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
import TyphoonHQuickInterface.Widgets       1.0

//-------------------------------------------------------------------------
//-- RC Wignal/Battery Indicator
Item {
    width:          rcRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property real   _batteryLevel:  TyphoonHQuickInterface.rcBattery
    property color  _batteryColor:  _batteryLevel > 0.0 ? Qt.hsva(0.333333 * _batteryLevel, 1, 1, 1) : qgcPal.buttonText

    function getBatteryPercentageText() {
        if(_batteryLevel > 0.98) {
            return "100%"
        }
        if(_batteryLevel > 0.01) {
            return (_batteryLevel * 100).toFixed(0) + '%'
        }
        return "N/A"
    }

    function getBatteryImage() {
        if(_batteryLevel > 0.0) {
            if(_batteryLevel > 0.8) {
                return "/typhoonh/img/rc_battery_charge_100.svg"
            }
            if(_batteryLevel > 0.6) {
                return "/typhoonh/img/rc_battery_charge_80.svg"
            }
            if(_batteryLevel > 0.4) {
                return "/typhoonh/img/rc_battery_charge_60.svg"
            }
            if(_batteryLevel > 0.2) {
                return "/typhoonh/img/rc_battery_charge_40.svg"
            }
            if(_batteryLevel > 0.01) {
                return "/typhoonh/img/rc_battery_charge_20.svg"
            }
        }
        return ""
    }

    function handleRssiWarning() {
        if(_activeVehicle) {
            if(!TyphoonHQuickInterface.rcActive) {
                if(!rcAnimation.running) {
                    rcAnimation.start()
                }
            } else {
                if(rcAnimation.running) {
                    rcAnimation.stop()
                    rcIcon.color = qgcPal.buttonText
                }
            }
        } else {
            rcAnimation.stop()
            rcIcon.color = qgcPal.buttonText
        }
    }

    Connections {
        target: TyphoonHQuickInterface
        onRcActiveChanged: {
            handleRssiWarning()
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        onActiveVehicleAvailableChanged: {
            handleRssiWarning()
        }
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
                    text:           qsTr("ST16 Status")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 rcrssiGrid
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    columns:            2
                    anchors.horizontalCenter: parent.horizontalCenter

                    QGCLabel { text: qsTr("ST16 RSSI:") }
                    QGCLabel { text: _activeVehicle ? (_activeVehicle.rcRSSI > 100 ? 'N/A' : _activeVehicle.rcRSSI + "%") : 'N/A' }
                    QGCLabel { text: qsTr("ST16 Battery:") }
                    QGCLabel { text: getBatteryPercentageText() }
                    QGCLabel { text: qsTr("ST16 GPS Sat Count:") }
                    QGCLabel { text: TyphoonHQuickInterface.gpsCount.toFixed(0) }
                  //QGCLabel { text: qsTr("RC GPS Accuracy:") }
                  //QGCLabel { text: TyphoonHQuickInterface.gpsAccuracy.toFixed(1) }
                  //QGCLabel { text: qsTr("RC Ground Speed:") }
                  //QGCLabel { text: TyphoonHQuickInterface.speed.toFixed(1) }
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
            id:                 rcIcon
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              height
            sourceSize.height:  height
            source:             "/typhoonh/img/rc.svg"
            fillMode:           Image.PreserveAspectFit
            opacity:            _activeVehicle ? 1 : 0.5
            color:              qgcPal.buttonText
            SequentialAnimation on color {
                id:         rcAnimation
                running:    false
                loops:      Animation.Infinite
                ColorAnimation { from: qgcPal.colorRed; to: qgcPal.text;     duration: 750 }
                ColorAnimation { from: qgcPal.text;     to: qgcPal.colorRed; duration: 750 }
            }
        }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            SignalStrength {
                size:                   rcRow.height * 0.4
                percent:                _activeVehicle ? ((_activeVehicle.rcRSSI > 100) ? 0 : _activeVehicle.rcRSSI) : 0
                opacity:                _activeVehicle ? 1 : 0.5
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Row {
                height:             rcRow.height * 0.6
                spacing:            ScreenTools.defaultFontPixelWidth
                QGCColoredImage {
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    source:             "/typhoonh/img/rc_battery_back.svg"
                    fillMode:           Image.PreserveAspectFit
                    width:              parent.height
                    height:             parent.height
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
                }
                QGCLabel {
                    text:                   getBatteryPercentageText()
                    color:                  qgcPal.buttonText
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
