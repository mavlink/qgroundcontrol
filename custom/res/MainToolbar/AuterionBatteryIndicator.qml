/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                              2.11
import QtQuick.Controls                     2.4
import QtQuick.Layouts                      1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

import Auterion.Widgets                     1.0

//-------------------------------------------------------------------------
//-- Battery Indicator
Item {
    id:                     _root
    width:                  batteryIndicatorRow.width
    anchors.top:            parent.top
    anchors.bottom:         parent.bottom

    property var    batterySummary:     activeVehicle ? activeVehicle.batterySummary : null
    property var    battery1:           activeVehicle ? activeVehicle.battery        : null
    property var    battery2:           activeVehicle ? activeVehicle.battery2       : null
    property bool   hasSecondBattery:   battery2 && battery2.voltage.value !== -1

    readonly property real _textFontSize: ScreenTools.defaultFontPointSize * (ScreenTools.isMobile ? 1.5 : 1.25)

    function lowestBattery() {
        if(activeVehicle) {
            if(batterySummary && (batterySummary.voltage.value !== -1 || batterySummary.percentRemaining.value > 0.1)) {
                return batterySummary;
            } else if(hasSecondBattery) {
                if(activeVehicle.battery2.percentRemaining.value < activeVehicle.battery.percentRemaining.value) {
                    return activeVehicle.battery2
                }
            }
            return activeVehicle.battery
        }
        return null
    }

    function getBatteryColor(battery) {
        if(battery) {
            if(battery.percentRemaining.value > 75) {
                return qgcPal.text
            }
            if(battery.percentRemaining.value > 50) {
                return qgcPal.colorOrange
            }
            if(battery.percentRemaining.value > 0.1) {
                return qgcPal.colorRed
            }
        }
        return qgcPal.colorGrey
    }

    function getBatteryPercentageText(battery) {
        if(battery) {
            if(battery.percentRemaining.value > 98.9) {
                return "100%"
            }
            if(battery.percentRemaining.value > 0.1) {
                return battery.percentRemaining.valueString + battery.percentRemaining.units
            }
            if(battery.voltage.value >= 0) {
                return battery.voltage.valueString + battery.voltage.units
            }
        }
        return "N/A"
    }

    Row {
        id:             batteryIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        opacity:        (activeVehicle && activeVehicle.batterySummary.voltage.value >= 0) ? 1 : 0.5
        spacing:        ScreenTools.defaultFontPixelWidth
        QGCColoredImage {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              height
            sourceSize.width:   width
            source:             "/auterion/img/menu_battery.svg"
            color:              qgcPal.text
            fillMode:           Image.PreserveAspectFit
            Rectangle {
                color:              getBatteryColor(lowestBattery())
                anchors.left:       parent.left
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.25
                height:             parent.height * 0.35
                width:              activeVehicle ? (activeVehicle.batterySummary.percentRemaining.value / 100) * parent.width * 0.75 : 0
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        QGCLabel {
            text:                   getBatteryPercentageText(lowestBattery())
            font.pointSize:         ScreenTools.smallFontPointSize
            color:                  getBatteryColor(lowestBattery())
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked: {
            mainWindow.showPopUp(_root, allBatteryInfoComponent)
        }
    }

    Component {
        id: allBatteryInfoComponent

        Rectangle {
            width:  battCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: battCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 1.0
            color:  qgcPal.window

            Column {
                id:                 battCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              batteryInfoComponent.width
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             battLabel
                    text:           qsTr("Battery Status")
                    font.pointSize: _root._textFontSize
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Loader {
                    sourceComponent: batteryInfoComponent

                    property string batteryName: "Battery 1"
                    property var batteryObj: activeVehicle.battery
                    property var textFontSize: _root._textFontSize
                }

                Loader {
                    sourceComponent: batteryInfoComponent

                    visible: hasSecondBattery

                    property string batteryName: "Battery 2"
                    property var batteryObj: activeVehicle.battery2
                    property var textFontSize: _root._textFontSize
                }
            }
        }
    }

    //
    // Individual battery detailed information loaded in allBatteryInfoComponent
    //
    Component {
        id: batteryInfoComponent

        GridLayout {
            id:                 battGrid
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            columnSpacing:      ScreenTools.defaultFontPixelWidth
            columns:            2
            anchors.horizontalCenter: parent.horizontalCenter

            // Needs to be defined in Loader
            // property string batteryName: qsTr("...")
            // property var batteryObj: null
            // property var textFontSize: ...

            // Row Battery name
            QGCLabel {
                id: batteryLabel
                text: batteryName
                font.pointSize: textFontSize
                Layout.alignment: Qt.AlignVCenter
            }
            QGCColoredImage {
                height:         batteryLabel.height
                width:          height
                sourceSize.width:   width
                source:         "/auterion/img/menu_battery.svg"
                color:          qgcPal.text
                fillMode:       Image.PreserveAspectFit
                Rectangle {
                    color:              getBatteryColor(batteryObj)
                    anchors.left:       parent.left
                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.125
                    height:             parent.height * 0.35
                    width:              batteryObj ? (batteryObj.percentRemaining.value / 100) * parent.width * 0.875 : 0
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Row Voltage
            QGCLabel {
                text: qsTr("Voltage:")
                font.pointSize: textFontSize
            }
            QGCLabel {
                text: (batteryObj && batteryObj.voltage.value !== -1) ? (batteryObj.voltage.valueString + " " + batteryObj.voltage.units) : "N/A"
                font.pointSize: textFontSize
            }
            // Row Percentage
            QGCLabel {
                text: qsTr("Percentage:")
                font.pointSize: textFontSize
            }
            QGCLabel {
                text: (batteryObj && batteryObj.percentRemaining.value > 0.1) ? getBatteryPercentageText(batteryObj) : "N/A"
                font.pointSize: textFontSize
            }
            // Row Accumulated Consumption
            QGCLabel {
                text: qsTr("Accumulated Consumption:")
                font.pointSize: textFontSize
            }
            QGCLabel {
                text: (batteryObj && batteryObj.mahConsumed.value !== -1) ? (batteryObj.mahConsumed.valueString + " " + batteryObj.mahConsumed.units) : "N/A"
                font.pointSize: textFontSize
            }
        }
    }
}
