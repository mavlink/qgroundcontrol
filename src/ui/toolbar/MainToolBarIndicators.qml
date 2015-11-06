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

import QtQuick 2.5
import QtQuick.Controls 1.2
import QtGraphicalEffects 1.0
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.1

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Row {
    spacing:  tbSpacing * 2

    function getSatStrength(count) {
        if (count < 1)
            return 0
        if (count < 4)
            return 20
        if (count < 6)
            return 40
        if (count < 8)
            return 60
        if (count < 10)
            return 80
        return 100
    }

    function getMessageColor() {
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

    function getBatteryVoltageText() {
        if (activeVehicle.batteryVoltage > 0) {
            //-- TODO: Need number of cells so I can show cell voltage instead of total voltage
            //if (battNumCells && battNumCells.value) {
            //    return (activeVehicle.batteryVoltage / battNumCells.value).toFixed(2) + 'V'
            //} else {
                return activeVehicle.batteryVoltage.toFixed(1) + 'V'
            //}
        }
        return 'N/A';
    }

    function getBatteryPercentageText() {
        if(activeVehicle.batteryPercent > 98.9) {
            return "100%"
        }
        if(activeVehicle.batteryPercent > 0.1) {
            return activeVehicle.batteryPercent.toFixed(0) + "%"
        }
        return "N/A"
    }

    function getBatteryColor() {
        if(activeVehicle.batteryPercent > 75) {
            return colorGreen
        }
        if(activeVehicle.batteryPercent > 50) {
            return colorOrange
        }
        if(activeVehicle.batteryPercent > 0.1) {
            return colorRed
        }
        return colorGrey
    }

    //-------------------------------------------------------------------------
    //-- Message Indicator
    Item {
        id:         messages
        width:      mainWindow.tbCellHeight
        height:     mainWindow.tbCellHeight
        visible:    activeVehicle.messageCount
        anchors.verticalCenter: parent.verticalCenter

        Item {
            id:                 criticalMessage
            anchors.fill:       parent
            visible:            activeVehicle.messageCount > 0 && isMessageImportant
            Image {
                source:         "/qmlimages/Yield.svg"
                height:         mainWindow.tbButtonWidth
                fillMode:       Image.PreserveAspectFit
                mipmap:         true
                smooth:         true
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
            Image {
                id:             messageIcon
                source:         "/qmlimages/Megaphone.svg"
                height:         mainWindow.tbCellHeight * 0.5
                fillMode:       Image.PreserveAspectFit
                mipmap:         true
                smooth:         true
                visible:        false
                anchors.verticalCenter:   parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            ColorOverlay {
                anchors.fill:   messageIcon
                source:         messageIcon
                color:          getMessageColor()
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                var p = mapToItem(toolBar, mouseX, mouseY);
                _controller.onEnterMessageArea(p.x, p.y);
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
            Image {
                id:             gpsIcon
                source:         "/qmlimages/Gps.svg"
                fillMode:       Image.PreserveAspectFit
                mipmap:         true
                smooth:         true
                width:          mainWindow.tbCellHeight * 0.65
                height:         mainWindow.tbCellHeight * 0.5
                opacity:        activeVehicle.satelliteCount < 1 ? 0.5 : 1
                anchors.verticalCenter: parent.verticalCenter
            }
            SignalStrength {
                size:           mainWindow.tbCellHeight * 0.5
                percent:        getSatStrength(activeVehicle.satelliteCount)
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        QGCLabel {
            text:           activeVehicle.satelliteCount
            font.pixelSize: tbFontSmall
            color:          colorWhite
            opacity:        activeVehicle.satelliteCount < 1 ? 0.5 : 1
            anchors.top:    parent.top
            anchors.leftMargin: gpsIcon.width
            anchors.left:   parent.left
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
            Image {
                source:         "/qmlimages/RC.svg"
                fillMode:       Image.PreserveAspectFit
                mipmap:         true
                smooth:         true
                width:          mainWindow.tbCellHeight * 0.65
                height:         mainWindow.tbCellHeight * 0.5
                opacity:        _controller.remoteRSSI < 1 ? 0.5 : 1
                anchors.verticalCenter: parent.verticalCenter
            }
            SignalStrength {
                size:           mainWindow.tbCellHeight * 0.5
                percent:        _controller.remoteRSSI
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Battery Indicator
    Item {
        id: batteryStatus
        width:  battRow.width * 1.1
        height: mainWindow.tbCellHeight
        opacity: (activeVehicle.batteryVoltage > 0) ? 1 : 0.5
        Row {
            id:         battRow
            height:     mainWindow.tbCellHeight
            spacing:    tbSpacing
            anchors.horizontalCenter: parent.horizontalCenter
            Column {
                spacing:            tbSpacing * 0.5
                anchors.verticalCenter: parent.verticalCenter
                Image {
                    id:             batIcon
                    source:         "/qmlimages/Battery.svg"
                    fillMode:       Image.PreserveAspectFit
                    mipmap:         true
                    smooth:         true
                    height:         batPercent.height * 0.85
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                QGCLabel {
                    text:           (activeVehicle.batteryConsumed > 0) ? activeVehicle.batteryConsumed.toFixed(0) + 'mAh' : 'N/A';
                    font.pixelSize: tbFontSmall
                    color:          getBatteryColor()
                    visible:        QGroundControl.isAdvancedMode
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                QGCLabel {
                    id:             batPercent
                    text:           getBatteryPercentageText()
                    font.pixelSize: tbFontLarge
                    color:          getBatteryColor()
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                QGCLabel {
                    text:           getBatteryVoltageText()
                    font.pixelSize: tbFontNormal
                    color:          getBatteryColor()
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Vehicle Selector
    QGCButton {
        width:      ScreenTools.defaultFontPixelSize * 12
        height:     mainWindow.tbButtonWidth
        text:       "Vehicle " + activeVehicle.id
        visible:    vehicleMenuItems.length > 0
        anchors.verticalCenter: parent.verticalCenter

        menu: vehicleMenu

        Menu {
            id: vehicleMenu
        }

        Component {
            id: vehicleMenuItemComponent

            MenuItem {
                checkable:      true
                checked:        vehicle.active
                onTriggered:    multiVehicleManager.activeVehicle = vehicle

                property int vehicleId: Number(text.split(" ")[1])
                property var vehicle:   multiVehicleManager.getVehicleById(vehicleId)
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
            for (var i=0; i<multiVehicleManager.vehicles.count; i++) {
                var vehicle = multiVehicleManager.vehicles.get(i)
                var menuItem = vehicleMenuItemComponent.createObject(null, { "text": "Vehicle " + vehicle.id })
                vehicleMenuItems.push(menuItem)
                vehicleMenu.insertItem(i, menuItem)
            }
        }

        Component.onCompleted: updateVehicleMenu()

        Connections {
            target:         multiVehicleManager.vehicles
            onCountChanged: parent.updateVehicleMenu
        }
    }

    //-------------------------------------------------------------------------
    //-- Mode Selector

    Item {
        width:  selectorRow.width * 1.1
        height: mainWindow.tbCellHeight
        anchors.verticalCenter: parent.verticalCenter
        Row {
            id:                 selectorRow
            spacing:            tbSpacing
            anchors.verticalCenter:   parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            Image {
                width:          mainWindow.tbCellHeight * 0.65
                height:         mainWindow.tbCellHeight * 0.65
                fillMode:       Image.PreserveAspectFit
                mipmap:         true
                smooth:         true
                source:         "/qmlimages/Quad.svg"
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:           activeVehicle.flightMode
                font.pixelSize: tbFontLarge
                color:          colorWhite
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Menu {
            id: flightModesMenu
        }

        Component {
            id: flightModeMenuItemComponent

            MenuItem {
                checkable:      true
                checked:        activeVehicle.flightMode === text
                onTriggered:    activeVehicle.flightMode = text
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
            target:                 multiVehicleManager
            onActiveVehicleChanged: parent.updateFlightModesMenu
        }

        MouseArea {
            visible: activeVehicle.flightModeSetAvailable
            anchors.fill:   parent
            onClicked: {
                flightModesMenu.popup()
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Arm/Disarm

    Item {
        width:  armCol.width * 1.1
        height: mainWindow.tbCellHeight
        anchors.verticalCenter: parent.verticalCenter
        Row {
            id:                 armCol
            spacing:            tbSpacing * 0.5
            anchors.verticalCenter: parent.verticalCenter
            Image {
                width:          mainWindow.tbCellHeight * 0.5
                height:         mainWindow.tbCellHeight * 0.5
                fillMode:       Image.PreserveAspectFit
                mipmap:         true
                smooth:         true
                source:         activeVehicle.armed ? "/qmlimages/Disarmed.svg" : "/qmlimages/Armed.svg"
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:           activeVehicle.armed ? "Armed" : "Disarmed"
                font.pixelSize: tbFontLarge
                color:          colorWhite
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        MouseArea {
            anchors.fill:   parent
            onClicked: {
                armDialog.visible = true
            }
        }
        MessageDialog {
            id:         armDialog
            visible:    false
            icon:       StandardIcon.Warning
            standardButtons: StandardButton.Yes | StandardButton.No
            title:      activeVehicle.armed ? "Disarming Vehicle" : "Arming Vehicle"
            text:       activeVehicle.armed ? "Do you want to disarm? This will cut power to all motors." : "Do you want to arm? This will enable all motors."
            onYes: {
                activeVehicle.armed = !activeVehicle.armed
                armDialog.visible = false
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
                    text: _controller.telemetryRRSSI + 'dB'
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
                    text: _controller.telemetryLRSSI + 'dB'
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
