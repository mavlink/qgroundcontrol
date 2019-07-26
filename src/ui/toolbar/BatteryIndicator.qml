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
import QtQuick.Window                       2.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- Battery Indicator
Item {
    id:             _root
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          batteryIndicatorRow.width

    property bool   showIndicator: true

    property var    batterySummary:     activeVehicle ? activeVehicle.batterySummary : null
    property var    battery1:           activeVehicle ? activeVehicle.battery1       : null
    property var    battery2:           activeVehicle ? activeVehicle.battery2       : null
    property bool   hasSecondBattery:   battery2 && battery2.voltage.value !== -1
    property real   maxHeight:          ScreenTools.isMobile ? (Screen.height - ScreenTools.toolbarHeight) * 0.7 : Screen.height

    readonly property real _textFontSize: ScreenTools.defaultFontPointSize

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
            return Qt.darker(qgcPal.colorRed, 1.5)
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
        opacity:        (_root.batterySummary && _root.batterySummary.voltage.value >= 0) ? 1 : 0.5
        QGCColoredImage {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              height
            sourceSize.width:   width
            source:             "/qmlimages/Battery.svg"
            color:              qgcPal.text
            fillMode:           Image.PreserveAspectFit
        }
        QGCLabel {
            text:                   getBatteryPercentageText(_root.batterySummary)
            font.pointSize:         ScreenTools.mediumFontPointSize
            color:                  getBatteryColor(_root.batterySummary)
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
            width: flickableItem.contentWidth
            height: flickableItem.contentHeight > _root.maxHeight ? _root.maxHeight : flickableItem.contentHeight
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color: qgcPal.text

            QGCFlickable {
                id: flickableItem
                anchors.fill: parent
                contentWidth: battCol.width + ScreenTools.defaultFontPixelWidth  * 3;
                contentHeight: battCol.height  + ScreenTools.defaultFontPixelHeight * 2
                Item {
                    anchors.fill: parent

                    Column {
                        id:                 battCol
                        spacing:            ScreenTools.defaultFontPixelHeight * 1
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
                            property var batteryObj: _root.battery1
                            property var textFontSize: _root._textFontSize
                            property bool moreDetails: moreDetails.checked
                        }

                        Loader {
                            sourceComponent: batteryInfoComponent

                            visible: hasSecondBattery

                            property string batteryName: "Battery 2"
                            property var batteryObj: _root.battery2
                            property var textFontSize: _root._textFontSize
                            property bool moreDetails: moreDetails.checked
                        }

                        QGCCheckBox {
                            id: moreDetails
                            radius: ScreenTools.defaultFontPixelHeight * 0.25
                            text: qsTr("More details")
                            checked: QGroundControl.settingsManager.appSettings.showDetailedBatteryInfo.rawValue
                            onCheckedChanged: {
                                QGroundControl.settingsManager.appSettings.showDetailedBatteryInfo.rawValue = checked
                            }

                            textFontPointSize: _root._textFontSize
                        }
                    }
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
            // property bool moreDetails: ...

            // Row: Battery name
            QGCLabel {
                id: batteryLabel
                text: batteryName
                font.pointSize: textFontSize
                Layout.alignment: Qt.AlignVCenter
            }
            QGCColoredImage {
                height:             batteryLabel.height
                width:              height
                sourceSize.width:   width
                source:             "/qmlimages/Battery.svg"
                color:              getBatteryColor(batteryObj)
                fillMode:           Image.PreserveAspectFit
            }

            // Row: Voltage
            QGCLabel {
                text: qsTr("Voltage:")
                font.pointSize: textFontSize
            }
            QGCLabel {
                text: (batteryObj && batteryObj.voltage.value !== -1) ? (batteryObj.voltage.valueString + " " + batteryObj.voltage.units) : "N/A"
                font.pointSize: textFontSize
            }
            // Row: Percentage
            QGCLabel {
                text: qsTr("Percentage:")
                font.pointSize: textFontSize
            }
            QGCLabel {
                text: (batteryObj && batteryObj.percentRemaining.value > 0.1) ? getBatteryPercentageText(batteryObj) : "N/A"
                font.pointSize: textFontSize
            }
            // Row: Accumulated Consumption
            QGCLabel {
                text: qsTr("Accumulated Consumption:")
                font.pointSize: textFontSize
            }
            QGCLabel {
                text: (batteryObj && batteryObj.mahConsumed.value !== -1) ? (batteryObj.mahConsumed.valueString + " " + batteryObj.mahConsumed.units) : "N/A"
                font.pointSize: textFontSize
            }
            //
            // Optional rows
            //
            // Row: Current
            QGCLabel {
                id: currentLabel
                visible: moreDetails && batteryObj && batteryObj.current.value > 0
                text: qsTr("Current:")
                font.pointSize: textFontSize
            }
            QGCLabel {
                visible: currentLabel.visible
                text: visible ? (batteryObj.current.valueString + " " + batteryObj.current.units) : "N/A"
                font.pointSize: textFontSize
            }
            // Row: Temperature
            QGCLabel {
                id: temperatureLabel
                visible: moreDetails && batteryObj && batteryObj.temperature.value > 0
                text: qsTr("Temperature:")
                font.pointSize: textFontSize
            }
            QGCLabel {
                visible: temperatureLabel.visible
                text: visible ? (batteryObj.temperature.valueString + " " + batteryObj.temperature.units) : "N/A"
                font.pointSize: textFontSize
            }
            // Row: Time Remaining
            QGCLabel {
                id: timeRemainingLabel
                visible: moreDetails && batteryObj && batteryObj.timeRemaining.value > 0
                text: qsTr("Time Remaining:")
                font.pointSize: textFontSize
            }
            QGCLabel {
                visible: timeRemainingLabel.visible
                text: visible ? (batteryObj.timeRemaining.value + " " + batteryObj.timeRemaining.units) : "N/A"
                font.pointSize: textFontSize
            }
        }
    }
}
