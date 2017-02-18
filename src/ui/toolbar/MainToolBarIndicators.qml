/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {
    property var  _activeVehicle:        QGroundControl.multiVehicleManager.activeVehicle
    property bool _communicationLost:   _activeVehicle ? _activeVehicle.connectionLost : false

    QGCPalette { id: qgcPal }

    function getBatteryColor() {
        if(_activeVehicle) {
            if(_activeVehicle.battery.percentRemaining.value > 75) {
                return qgcPal.text
            }
            if(_activeVehicle.battery.percentRemaining.value > 50) {
                return colorOrange
            }
            if(_activeVehicle.battery.percentRemaining.value > 0.1) {
                return colorRed
            }
        }
        return colorGrey
    }

    function getRSSIColor(value) {
        if(value >= 0)
            return colorGrey;
        if(value > -60)
            return colorGreen;
        if(value > -90)
            return colorOrange;
        return colorRed;
    }

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

    //---------------------------------------------
    // GPS Info
    Component {
        id: gpsInfo

        Rectangle {
            width:  gpsCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: gpsCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 gpsCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(gpsGrid.width, gpsLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             gpsLabel
                    text:           (_activeVehicle && _activeVehicle.gps.count.value >= 0) ? qsTr("GPS Status") : qsTr("GPS Data Unavailable")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 gpsGrid
                    visible:            (_activeVehicle && _activeVehicle.gps.count.value >= 0)
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2

                    QGCLabel { text: qsTr("GPS Count:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.count.valueString : qsTr("N/A", "No data to display") }
                    QGCLabel { text: qsTr("GPS Lock:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.lock.enumStringValue : qsTr("N/A", "No data to display") }
                    QGCLabel { text: qsTr("HDOP:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.hdop.valueString : qsTr("--.--", "No data to display") }
                    QGCLabel { text: qsTr("VDOP:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.vdop.valueString : qsTr("--.--", "No data to display") }
                    QGCLabel { text: qsTr("Course Over Ground:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.courseOverGround.valueString : qsTr("--.--", "No data to display") }
                }
            }

            Component.onCompleted: {
                var pos = mapFromItem(toolBar, centerX - (width / 2), toolBar.height)
                x = pos.x
                y = pos.y + ScreenTools.defaultFontPixelHeight
            }
        }
    }

    //---------------------------------------------
    // Battery Info
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
            }
        }
    }

    //---------------------------------------------
    // RC RSSI Info
    Component {
        id: rcRSSIInfo

        Rectangle {
            width:  rcrssiCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: rcrssiCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 rcrssiCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(rcrssiGrid.width, rssiLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             rssiLabel
                    text:           _activeVehicle ? (_activeVehicle.rcRSSI != 255 ? qsTr("RC RSSI Status") : qsTr("RC RSSI Data Unavailable")) : qsTr("N/A", "No data available")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 rcrssiGrid
                    visible:            _activeVehicle && _activeVehicle.rcRSSI != 255
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    columns:            2
                    anchors.horizontalCenter: parent.horizontalCenter

                    QGCLabel { text: qsTr("RSSI:") }
                    QGCLabel { text: _activeVehicle ? (_activeVehicle.rcRSSI + "%") : 0 }
                }
            }

            Component.onCompleted: {
                var pos = mapFromItem(toolBar, centerX - (width / 2), toolBar.height)
                x = pos.x
                y = pos.y + ScreenTools.defaultFontPixelHeight
            }
        }
    }

    //---------------------------------------------
    // Telemetry RSSI Info
    Component {
        id: telemRSSIInfo

        Rectangle {
            width:  telemCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: telemCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 telemCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(telemGrid.width, telemLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             telemLabel
                    text:           qsTr("Telemetry RSSI Status")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 telemGrid
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    columns:            2
                    anchors.horizontalCenter: parent.horizontalCenter

                    QGCLabel { text: qsTr("Local RSSI:") }
                    QGCLabel { text: _activeVehicle.telemetryLRSSI + " dBm" }
                    QGCLabel { text: qsTr("Remote RSSI:") }
                    QGCLabel { text: _activeVehicle.telemetryRRSSI + " dBm" }
                    QGCLabel { text: qsTr("RX Errors:") }
                    QGCLabel { text: _activeVehicle.telemetryRXErrors }
                    QGCLabel { text: qsTr("Errors Fixed:") }
                    QGCLabel { text: _activeVehicle.telemetryFixed }
                    QGCLabel { text: qsTr("TX Buffer:") }
                    QGCLabel { text: _activeVehicle.telemetryTXBuffer }
                    QGCLabel { text: qsTr("Local Noise:") }
                    QGCLabel { text: _activeVehicle.telemetryLNoise }
                    QGCLabel { text: qsTr("Remote Noise:") }
                    QGCLabel { text: _activeVehicle.telemetryRNoise }
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
        id:             indicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth * 1.5
        visible:        !_communicationLost

        //-------------------------------------------------------------------------
        //-- Message Indicator
        Item {
            id:             messages
            width:          height
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            visible:        _activeVehicle && _activeVehicle.messageCount

            Image {
                id:                 criticalMessageIcon
                anchors.fill:       parent
                source:             "/qmlimages/Yield.svg"
                sourceSize.height:  height
                fillMode:           Image.PreserveAspectFit
                cache:              false
                visible:            _activeVehicle && _activeVehicle.messageCount > 0 && isMessageImportant
            }

            QGCColoredImage {
                anchors.fill:       parent
                source:             "/qmlimages/Megaphone.svg"
                sourceSize.height:  height
                fillMode:           Image.PreserveAspectFit
                color:              getMessageColor()
                visible:            !criticalMessageIcon.visible
            }

            MouseArea {
                anchors.fill:   parent
                onClicked:      mainWindow.showMessageArea()
            }
        }

        //-------------------------------------------------------------------------
        //-- GPS Indicator
        Item {
            id:             satelitte
            width:          (gpsValuesColumn.x + gpsValuesColumn.width) * 1.1
            anchors.top:    parent.top
            anchors.bottom: parent.bottom

            QGCColoredImage {
                id:                 gpsIcon
                width:              height
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             "/qmlimages/Gps.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                opacity:            (_activeVehicle && _activeVehicle.gps.count.value >= 0) ? 1 : 0.5
                color:              qgcPal.buttonText
            }

            Column {
                id:                     gpsValuesColumn
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
                anchors.left:           gpsIcon.right

                QGCLabel {
                    anchors.horizontalCenter:   hdopValue.horizontalCenter
                    visible:                    _activeVehicle && !isNaN(_activeVehicle.gps.hdop.value)
                    color:                      qgcPal.buttonText
                    text:                       _activeVehicle ? _activeVehicle.gps.count.valueString : ""
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
            id:             rcRssi
            width:          rssiRow.width * 1.1
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            visible:        _activeVehicle ? _activeVehicle.supportsRadio : true

            Row {
                id:             rssiRow
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                spacing:        ScreenTools.defaultFontPixelWidth

                QGCColoredImage {
                    width:              height
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    sourceSize.height:  height
                    source:             "/qmlimages/RC.svg"
                    fillMode:           Image.PreserveAspectFit
                    opacity:            _activeVehicle ? (((_activeVehicle.rcRSSI < 0) || (_activeVehicle.rcRSSI > 100)) ? 0.5 : 1) : 0.5
                    color:              qgcPal.buttonText
                }

                SignalStrength {
                    anchors.verticalCenter: parent.verticalCenter
                    size:                   parent.height * 0.5
                    percent:                _activeVehicle ? ((_activeVehicle.rcRSSI > 100) ? 0 : _activeVehicle.rcRSSI) : 0
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
        QGCColoredImage {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            sourceSize.height:  height
            source:             "/qmlimages/TelemRSSI.svg"
            fillMode:           Image.PreserveAspectFit
            color:              qgcPal.buttonText
            visible:            _activeVehicle ? (_activeVehicle.telemetryLRSSI < 0) : false

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    var centerX = mapToItem(toolBar, x, y).x + (width / 2)
                    mainWindow.showPopUp(telemRSSIInfo, centerX)
                }
            }
        }

        //-------------------------------------------------------------------------
        //-- Battery Indicator
        Item {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          batteryIndicatorRow.width

            Row {
                id:             batteryIndicatorRow
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                opacity:        (_activeVehicle && _activeVehicle.battery.voltage.value >= 0) ? 1 : 0.5

                QGCColoredImage {
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    width:              height
                    sourceSize.width:   width
                    source:             "/qmlimages/Battery.svg"
                    fillMode:           Image.PreserveAspectFit
                    color:              qgcPal.text
                }

                QGCLabel {
                    text:                   getBatteryPercentageText()
                    font.pointSize:         ScreenTools.mediumFontPointSize
                    color:                  getBatteryColor()
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
