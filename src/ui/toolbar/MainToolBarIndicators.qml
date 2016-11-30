/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.5
import QtQuick.Controls         1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {
    property var  _activeVehicle:        QGroundControl.multiVehicleManager.activeVehicle
    property bool _communicationLost:   _activeVehicle ? _activeVehicle.connectionLost : false

    function getMessageColor() {
        if (_activeVehicle) {
            if (_activeVehicle.messageTypeNone)
                return colorGrey
            if (_activeVehicle.messageTypeNormal)
                return colorBlue;
            if (_activeVehicle.messageTypeWarning)
                return colorOrange;
            if (_activeVehicle.messageTypeError)
                return colorRed;
            // Cannot be so make make it obnoxious to show error
            console.log("Invalid vehicle message type")
            return "purple";
        }
        //-- It can only get here when closing (vehicle gone while window active)
        return "white";
    }

    function getBatteryVoltageText() {
        if (_activeVehicle.battery.voltage.value >= 0) {
            return _activeVehicle.battery.voltage.valueString + _activeVehicle.battery.voltage.units
        }
        return 'N/A';
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

    Row {
        id:                 indicatorRow
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        spacing:            ScreenTools.defaultFontPixelWidth * 1.5
        visible:            !_communicationLost

        QGCPalette { id: qgcPal }

        //-------------------------------------------------------------------------
        //-- Message Indicator
        Item {
            id:         messages
            width:      mainWindow.tbCellHeight
            height:     mainWindow.tbCellHeight
            visible:    _activeVehicle && _activeVehicle.messageCount
            anchors.verticalCenter: parent.verticalCenter
            Item {
                id:                 criticalMessage
                anchors.fill:       parent
                visible:            _activeVehicle && _activeVehicle.messageCount > 0 && isMessageImportant
                Image {
                    source:             "/qmlimages/Yield.svg"
                    height:             mainWindow.tbCellHeight * 0.75
                    sourceSize.height:  height
                    fillMode:           Image.PreserveAspectFit
                    cache:              false
                    visible:            isMessageImportant
                    anchors.verticalCenter:   parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            Item {
                anchors.fill:       parent
                visible:            !criticalMessage.visible
                QGCColoredImage {
                    id:         messageIcon
                    source:     "/qmlimages/Megaphone.svg"
                    height:     mainWindow.tbCellHeight * 0.5
                    width:      height
                    sourceSize.height: height
                    fillMode:   Image.PreserveAspectFit
                    color:      getMessageColor()
                    anchors.verticalCenter:   parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    mainWindow.showMessageArea()
                }
            }
        }

        //-------------------------------------------------------------------------
        //-- GPS Indicator
        Item {
            id:     satelitte
            width:  (gpsValuesColumn.x + gpsValuesColumn.width) * 1.1
            height: mainWindow.tbCellHeight

            QGCColoredImage {
                id:             gpsIcon
                source:         "/qmlimages/Gps.svg"
                fillMode:       Image.PreserveAspectFit
                width:          mainWindow.tbCellHeight * 0.65
                height:         mainWindow.tbCellHeight * 0.5
                sourceSize.height: height
                opacity:        (_activeVehicle && _activeVehicle.gps.count.value >= 0) ? 1 : 0.5
                color:          qgcPal.buttonText
                anchors.verticalCenter: parent.verticalCenter
            }

            Column {
                id:                     gpsValuesColumn
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
                anchors.left:           gpsIcon.right

                QGCLabel {
                    anchors.horizontalCenter: hdopValue.horizontalCenter
                    visible:    _activeVehicle && !isNaN(_activeVehicle.gps.hdop.value)
                    color:      qgcPal.buttonText
                    text:       _activeVehicle ? _activeVehicle.gps.count.valueString : ""
                }

                QGCLabel {
                    id:         hdopValue
                    visible:    _activeVehicle && !isNaN(_activeVehicle.gps.hdop.value)
                    color:      qgcPal.buttonText
                    text:       _activeVehicle ? _activeVehicle.gps.hdop.value.toFixed(1) : ""
                }
            } // Column

            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    var centerX = mapToItem(toolBar, x, y).x + (width / 2)
                    mainWindow.showPopUp(gpsInfo, centerX)
                }
            }
        }

        //-------------------------------------------------------------------------
        //-- RC RSSI
        Item {
            id:     rcRssi
            width:  rssiRow.width * 1.1
            height: mainWindow.tbCellHeight
            visible: _activeVehicle ? _activeVehicle.supportsRadio : true
            Row {
                id:         rssiRow
                height:     parent.height
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCColoredImage {
                    width:          mainWindow.tbCellHeight * 0.65
                    height:         width
                    sourceSize.height: height
                    source:         "/qmlimages/RC.svg"
                    fillMode:       Image.PreserveAspectFit
                    opacity:        _activeVehicle ? (((_activeVehicle.rcRSSI < 0) || (_activeVehicle.rcRSSI > 100)) ? 0.5 : 1) : 0.5
                    color:          qgcPal.buttonText
                    anchors.verticalCenter: parent.verticalCenter
                }
                SignalStrength {
                    size:       mainWindow.tbCellHeight * 0.5
                    percent:    _activeVehicle ? ((_activeVehicle.rcRSSI > 100) ? 0 : _activeVehicle.rcRSSI) : 0
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    var centerX = mapToItem(toolBar, x, y).x + (width / 2)
                    mainWindow.showPopUp(rcRSSIInfo, centerX)
                }
            }
        }

        //-------------------------------------------------------------------------
        //-- Telemetry RSSI
        Item {
            id:         telemRssi
            width:      telemIcon.width
            height:     mainWindow.tbCellHeight
            visible:    _controller.telemetryLRSSI < 0
            QGCColoredImage {
                id:         telemIcon
                height:     parent.height * 0.5
                sourceSize.height: height
                width:      height * 1.5
                source:     "/qmlimages/TelemRSSI.svg"
                fillMode:   Image.PreserveAspectFit
                color:      qgcPal.buttonText
                anchors.verticalCenter: parent.verticalCenter
            }
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    var centerX = mapToItem(toolBar, x, y).x + (width / 2)
                    mainWindow.showPopUp(telemRSSIInfo, centerX)
                }
            }
        }

        //-------------------------------------------------------------------------
        //-- Battery Indicator
        Item {
            id:         batteryStatus
            width:      battRow.width * 1.1
            height:     mainWindow.tbCellHeight
            opacity:    (_activeVehicle && _activeVehicle.battery.voltage.value >= 0) ? 1 : 0.5
            Row {
                id:     battRow
                height: mainWindow.tbCellHeight
                anchors.horizontalCenter: parent.horizontalCenter
                QGCColoredImage {
                    height:     mainWindow.tbCellHeight * 0.65
                    width:      height
                    sourceSize.width: width
                    source:     "/qmlimages/Battery.svg"
                    fillMode:   Image.PreserveAspectFit
                    color:      qgcPal.text
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCLabel {
                    text:           getBatteryPercentageText()
                    font.pointSize: ScreenTools.mediumFontPointSize
                    color:          getBatteryColor()
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                anchors.fill:   parent
                onClicked:      mainWindow.showPopUp(batteryInfo, mapToItem(toolBar, x, y).x + (width / 2))
            }
        }

        //-------------------------------------------------------------------------
        //-- Mode Selector
        QGCLabel {
            id:                     flightModeSelector
            text:                   _activeVehicle ? _activeVehicle.flightMode : qsTr("N/A", "No data to display")
            font.pointSize:         ScreenTools.mediumFontPointSize
            color:                  qgcPal.buttonText
            anchors.verticalCenter: parent.verticalCenter

            Menu {
                id: flightModesMenu
            }

            Component {
                id: flightModeMenuItemComponent

                MenuItem {
                    onTriggered: _activeVehicle.flightMode = text
                }
            }

            property var flightModesMenuItems: []

            function updateFlightModesMenu() {
                if (_activeVehicle && _activeVehicle.flightModeSetAvailable) {
                    // Remove old menu items
                    for (var i = 0; i < flightModesMenuItems.length; i++) {
                        flightModesMenu.removeItem(flightModesMenuItems[i])
                    }
                    flightModesMenuItems.length = 0
                    // Add new items
                    for (var i = 0; i < _activeVehicle.flightModes.length; i++) {
                        var menuItem = flightModeMenuItemComponent.createObject(null, { "text": _activeVehicle.flightModes[i] })
                        flightModesMenuItems.push(menuItem)
                        flightModesMenu.insertItem(i, menuItem)
                    }
                }
            }

            Component.onCompleted: flightModeSelector.updateFlightModesMenu()

            Connections {
                target:                 QGroundControl.multiVehicleManager
                onActiveVehicleChanged: flightModeSelector.updateFlightModesMenu()
            }

            MouseArea {
                visible:        _activeVehicle && _activeVehicle.flightModeSetAvailable
                anchors.fill:   parent
                onClicked:      flightModesMenu.popup()
            }
        } // QGCLabel - Flight mode selector
    } // Row - Vehicle indicators

    Image {
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        visible:                x > indicatorRow.width && !_communicationLost
        fillMode:               Image.PreserveAspectFit
        source:                 _activeVehicle ? _activeVehicle.brandImage : ""
    }

    Row {
        anchors.fill:       parent
        layoutDirection:    Qt.RightToLeft
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            _communicationLost

        QGCButton {
            id:                     disconnectButton
            anchors.verticalCenter: parent.verticalCenter
            text:                   qsTr("Disconnect")
            primary:                true
            onClicked:              _activeVehicle.disconnectInactiveVehicle()
        }

        QGCLabel {
            id:                     connectionLost
            anchors.verticalCenter: parent.verticalCenter
            text:                   qsTr("COMMUNICATION LOST")
            font.pointSize:         ScreenTools.largeFontPointSize
            font.family:            ScreenTools.demiboldFontFamily
            color:                  colorRed
        }
    } // Row - Communication lost
} // Item
