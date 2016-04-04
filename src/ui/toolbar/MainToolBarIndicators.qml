/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtGraphicalEffects       1.0
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.1

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Row {
    spacing:  tbSpacing * 2

    QGCPalette { id: qgcPal }

    function getSatStrength(hdop) {
        if (hdop <= 1.0)
            return 100
        if (hdop <= 1.4)
            return 75
        if (hdop <= 1.8)
            return 50
        if (hdop <= 3.0)
            return 25
        return 0
    }

    function getMessageColor() {
        if (activeVehicle) {
            if (activeVehicle.messageTypeNone)
                return colorGrey
            if (activeVehicle.messageTypeNormal)
                return colorBlue;
            if (activeVehicle.messageTypeWarning)
                return colorOrange;
            if (activeVehicle.messageTypeError)
                return colorRed;
            // Cannot be so make make it obnoxious to show error
            console.log("Invalid vehicle message type")
            return "purple";
        }
        //-- It can only get here when closing (vehicle gone while window active)
        return "white";
    }

    function getBatteryVoltageText() {
        if (activeVehicle.battery.voltage.value >= 0) {
            return activeVehicle.battery.voltage.valueString + activeVehicle.battery.voltage.units
        }
        return 'N/A';
    }

    function getBatteryPercentageText() {
        if(activeVehicle) {
            if(activeVehicle.battery.percentRemaining.value > 98.9) {
                return "100%"
            }
            if(activeVehicle.battery.percentRemaining.value > 0.1) {
                return activeVehicle.battery.percentRemaining.valueString + activeVehicle.battery.percentRemaining.units
            }
            if(activeVehicle.battery.voltage.value >= 0) {
                return activeVehicle.battery.voltage.valueString + activeVehicle.battery.voltage.units
            }
        }
        return "N/A"
    }

    //-------------------------------------------------------------------------
    //-- Message Indicator
    Item {
        id:         messages
        width:      mainWindow.tbCellHeight
        height:     mainWindow.tbCellHeight
        visible:    activeVehicle && activeVehicle.messageCount
        anchors.verticalCenter: parent.verticalCenter

        Item {
            id:                 criticalMessage
            anchors.fill:       parent
            visible:            activeVehicle && activeVehicle.messageCount > 0 && isMessageImportant

            Image {
                source:         "/qmlimages/Yield.svg"
                height:         mainWindow.tbCellHeight * 0.75
                fillMode:       Image.PreserveAspectFit
                cache:          false
                visible:        isMessageImportant
                anchors.verticalCenter:   parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

            SequentialAnimation {
                id:    loopAnimation
                loops: Animation.Infinite
                NumberAnimation { target: criticalMessage; property: "opacity"; duration: 1000; from: 0.25; to: 1 }
                NumberAnimation { target: criticalMessage; property: "opacity"; duration: 1000; from: 1; to: 0.25 }
            }

            onVisibleChanged: {
                if(messages.visible) {
                    loopAnimation.start()
                } else {
                    loopAnimation.stop()
                }
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
        width:  gpsRow.width * 1.1
        height: mainWindow.tbCellHeight
        Row {
            id:     gpsRow
            height: parent.height

            QGCColoredImage {
                id:             gpsIcon
                source:         "/qmlimages/Gps.svg"
                fillMode:       Image.PreserveAspectFit
                width:          mainWindow.tbCellHeight * 0.65
                height:         mainWindow.tbCellHeight * 0.5
                opacity:        (activeVehicle && activeVehicle.gps.count.value >= 0) ? 1 : 0.5
                color:          qgcPal.buttonText
                anchors.verticalCenter: parent.verticalCenter
            }

            SignalStrength {
                size:           mainWindow.tbCellHeight * 0.5
                percent:        activeVehicle ? getSatStrength(activeVehicle.gps.hdop.value) : ""
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        QGCLabel {
            anchors.top:        parent.top
            anchors.leftMargin: gpsIcon.width
            anchors.left:       parent.left
            visible:            activeVehicle && !isNaN(activeVehicle.gps.hdop.value)
            font.pixelSize:     tbFontSmall
            color:              qgcPal.buttonText
            text:               activeVehicle ? activeVehicle.gps.hdop.valueString : ""
        }

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

        Row {
            id:     rssiRow
            height: parent.height

            QGCColoredImage {
                width:          mainWindow.tbCellHeight * 0.65
                height:         mainWindow.tbCellHeight * 0.5
                source:         "/qmlimages/RC.svg"
                fillMode:       Image.PreserveAspectFit
                opacity:        activeVehicle ? (activeVehicle.rcRSSI < 1 ? 0.5 : 1) : 0.5
                color:          qgcPal.buttonText
                anchors.verticalCenter: parent.verticalCenter
            }

            SignalStrength {
                size:       mainWindow.tbCellHeight * 0.5
                percent:    activeVehicle ? activeVehicle.rcRSSI : 0
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
        opacity:    (activeVehicle && activeVehicle.battery.voltage.value >= 0) ? 1 : 0.5

        Row {
            id:     battRow
            height: mainWindow.tbCellHeight
            anchors.horizontalCenter: parent.horizontalCenter

            QGCColoredImage {
                height:     mainWindow.tbCellHeight * 0.65
                source:     "/qmlimages/Battery.svg"
                fillMode:   Image.PreserveAspectFit
                color:      qgcPal.buttonText
                anchors.verticalCenter: parent.verticalCenter
            }

            QGCLabel {
                text:           getBatteryPercentageText()
                font.pixelSize: tbFontLarge
                color:          getBatteryColor()
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            anchors.fill:   parent
            onClicked: {
                if (activeVehicle) {
                    var centerX = mapToItem(toolBar, x, y).x + (width / 2)
                    mainWindow.showPopUp(batteryInfo, centerX)
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Vehicle Selector
    QGCButton {
        id:                     vehicleSelectorButton
        width:                  ScreenTools.defaultFontPixelSize * 8
        text:                   "Vehicle " + (activeVehicle ? activeVehicle.id : "None")
        visible:                QGroundControl.multiVehicleManager.vehicles.count > 1
        anchors.verticalCenter: parent.verticalCenter

        menu: vehicleMenu

        Menu {
            id: vehicleMenu
        }

        Component {
            id: vehicleMenuItemComponent

            MenuItem {
                checkable:      true
                onTriggered:    QGroundControl.multiVehicleManager.activeVehicle = vehicle

                property int vehicleId: Number(text.split(" ")[1])
                property var vehicle:   QGroundControl.multiVehicleManager.getVehicleById(vehicleId)
            }
        }

        property var vehicleMenuItems: []

        function updateVehicleMenu() {
            // Remove old menu items
            for (var i = 0; i < vehicleMenuItems.length; i++) {
                vehicleMenu.removeItem(vehicleMenuItems[i])
            }
            vehicleMenuItems.length = 0

            // Add new items
            for (var i=0; i<QGroundControl.multiVehicleManager.vehicles.count; i++) {
                var vehicle = QGroundControl.multiVehicleManager.vehicles.get(i)
                var menuItem = vehicleMenuItemComponent.createObject(null, { "text": "Vehicle " + vehicle.id })
                vehicleMenuItems.push(menuItem)
                vehicleMenu.insertItem(i, menuItem)
            }
        }

        Component.onCompleted: updateVehicleMenu()

        Connections {
            target:         QGroundControl.multiVehicleManager.vehicles
            onCountChanged: vehicleSelectorButton.updateVehicleMenu()
        }
    }

    //-------------------------------------------------------------------------
    //-- Mode Selector

    Item {
        id:     flightModeSelector
        width:  selectorRow.width * 1.1
        height: mainWindow.tbCellHeight
        anchors.verticalCenter: parent.verticalCenter

        Row {
            id:                 selectorRow
            spacing:            tbSpacing
            anchors.verticalCenter:   parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter

            QGCLabel {
                text:           activeVehicle ? activeVehicle.flightMode : qsTr("N/A", "No data to display")
                font.pixelSize: tbFontLarge
                color:          qgcPal.buttonText
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Menu {
            id: flightModesMenu
        }

        Component {
            id: flightModeMenuItemComponent

            MenuItem {
                onTriggered: {
                    if(activeVehicle) {
                        activeVehicle.flightMode = text
                    }
                }
            }
        }

        property var flightModesMenuItems: []

        function updateFlightModesMenu() {
            if (activeVehicle.flightModeSetAvailable) {
                // Remove old menu items
                for (var i = 0; i < flightModesMenuItems.length; i++) {
                    flightModesMenu.removeItem(flightModesMenuItems[i])
                }
                flightModesMenuItems.length = 0
                // Add new items
                for (var i = 0; i < activeVehicle.flightModes.length; i++) {
                    var menuItem = flightModeMenuItemComponent.createObject(null, { "text": activeVehicle.flightModes[i] })
                    flightModesMenuItems.push(menuItem)
                    flightModesMenu.insertItem(i, menuItem)
                }
            }
        }

        Component.onCompleted: updateFlightModesMenu()

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: flightModeSelector.updateFlightModesMenu
        }

        MouseArea {
            visible: activeVehicle ? activeVehicle.flightModeSetAvailable : false
            anchors.fill:   parent
            onClicked: {
                flightModesMenu.popup()
            }
        }
    }

/*
    property var colorOrangeText: (qgcPal.globalTheme === QGCPalette.Light) ? "#b75711" : "#ea8225"
    property var colorRedText:    (qgcPal.globalTheme === QGCPalette.Light) ? "#ee1112" : "#ef2526"
    property var colorGreenText:  (qgcPal.globalTheme === QGCPalette.Light) ? "#046b1b" : "#00d930"
    property var colorWhiteText:  (qgcPal.globalTheme === QGCPalette.Light) ? "#343333" : "#f0f0f0"

    function getRSSIColor(value) {
        if(value < 10)
            return colorRed;
        if(value < 50)
            return colorOrange;
        return colorGreen;
    }

    Rectangle {
        id: rssiRC
        width:  getProportionalDimmension(55)
        height: mainWindow.tbCellHeight
        visible: _controller.remoteRSSI <= 100
        anchors.verticalCenter: parent.verticalCenter
        color:  getRSSIColor(_controller.remoteRSSI);
        border.color: "#00000000"
        border.width: 0
        Image {
            source: "qrc:/res/AntennaRC";
            width: mainWindow.tbCellHeight * 0.7
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: getProportionalDimmension(6)
            mipmap: true
            smooth: true
        }
        QGCLabel {
            text: _controller.remoteRSSI
            anchors.right: parent.right
            anchors.rightMargin: getProportionalDimmension(6)
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignRight
            font.pixelSize: ScreenTools.smallFontPixelSize
            font.weight: Font.DemiBold
            color: colorWhite
        }
    }

    Rectangle {
        id: rssiTelemetry
        width:  getProportionalDimmension(80)
        height: mainWindow.tbCellHeight
        visible: (_controller.telemetryRRSSI > 0) && (_controller.telemetryLRSSI > 0)
        anchors.verticalCenter: parent.verticalCenter
        color:  getRSSIColor(Math.min(_controller.telemetryRRSSI,_controller.telemetryLRSSI));
        border.color: "#00000000"
        border.width: 0
        Image {
            source: "qrc:/res/AntennaT";
            width: mainWindow.tbCellHeight * 0.7
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: getProportionalDimmension(6)
            mipmap: true
            smooth: true
        }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right:          parent.right
            anchors.rightMargin:    getProportionalDimmension(6)
            Row {
                anchors.right: parent.right
                QGCLabel {
                    text: 'R '
                    font.pixelSize: ScreenTools.smallFontPixelSize
                    font.weight: Font.DemiBold
                    color: colorWhite
                }
                QGCLabel {
                    text: _controller.telemetryRRSSI + 'dBm'
                    width: getProportionalDimmension(30)
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: ScreenTools.smallFontPixelSize
                    font.weight: Font.DemiBold
                    color: colorWhite
                }
            }
            Row {
                anchors.right: parent.right
                QGCLabel {
                    text: 'L '
                    font.pixelSize: ScreenTools.smallFontPixelSize
                    font.weight: Font.DemiBold
                    color: colorWhite
                }
                QGCLabel {
                    text: _controller.telemetryLRSSI + 'dBm'
                    width: getProportionalDimmension(30)
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: ScreenTools.smallFontPixelSize
                    font.weight: Font.DemiBold
                    color: colorWhite
                }
            }
        }
    }


*/

} // Row


