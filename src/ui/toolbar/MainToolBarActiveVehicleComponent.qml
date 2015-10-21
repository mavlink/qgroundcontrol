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

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Row {
    height:     cellHeight
    spacing:    cellSpacerSize

    Rectangle {
        id: messages
        width: (activeVehicle.messageCount > 99) ? getProportionalDimmension(65) : getProportionalDimmension(60)
        height: cellHeight
        anchors.verticalCenter: parent.verticalCenter
        color:  getMessageColor()
        border.color: "#00000000"
        border.width: 0
        property bool showTriangle: false

        Image {
            id: messageIcon
            source: getMessageIcon();
            height: getProportionalDimmension(16)
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: getProportionalDimmension(8)
        }

        Item {
            id: messageTextRect
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            width: messages.width - messageIcon.width
            QGCLabel {
                id: messageText
                text: (activeVehicle.messageCount > 0) ? activeVehicle.messageCount : ''
                font.pixelSize: ScreenTools.smallFontPixelSize
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                color: colorWhite
            }
        }

        Image {
            id: dropDown
            source: "/qmlimages/arrow-down.png"
            visible: (messages.showTriangle) && (activeVehicle.messageCount > 0)
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.bottomMargin: getProportionalDimmension(3)
            anchors.rightMargin:  getProportionalDimmension(3)
        }

        Timer {
            id: mouseOffTimer
            interval: 2000;
            running: false;
            repeat: false
            onTriggered: {
                messages.showTriangle = false;
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: {
                messages.showTriangle = true;
                mouseOffTimer.start();
            }
            onClicked: {
                var p = mapToItem(toolBarHolder, mouseX, mouseY);
                _controller.onEnterMessageArea(p.x, p.y);
            }
        }

    }

    QGCButton {
        width:                  ScreenTools.defaultFontPixelWidth * 13
        height:                 cellHeight
        anchors.verticalCenter: parent.verticalCenter
        text:                   "Vehicle " + activeVehicle.id

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
            for (var i=0; i<vehicleMenuItems.length; i++) {
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


    Rectangle {
        id: satelitte
        width:  getProportionalDimmension(55)
        height: cellHeight
        anchors.verticalCenter: parent.verticalCenter
        color:  getSatelliteColor();
        border.color: "#00000000"
        border.width: 0

        Image {
            source: "qrc:/res/Gps";
            height: getProportionalDimmension(24)
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: getProportionalDimmension(6)
            mipmap: true
            smooth: true
        }

        QGCLabel {
            id: satelitteText
            text: activeVehicle.satelliteCount >= 0 ? activeVehicle.satelliteCount : 'NA'
            font.pixelSize: activeVehicle.satelliteCount >= 0 ? ScreenTools.defaultFontPixelSize : ScreenTools.smallFontPixelSize
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: getProportionalDimmension(6)
            horizontalAlignment: Text.AlignRight
            color: colorWhite
        }
    }

    Rectangle {
        id: rssiRC
        width:  getProportionalDimmension(55)
        height: cellHeight
        visible: _controller.remoteRSSI <= 100
        anchors.verticalCenter: parent.verticalCenter
        color:  getRSSIColor(_controller.remoteRSSI);
        border.color: "#00000000"
        border.width: 0
        Image {
            source: "qrc:/res/AntennaRC";
            width: cellHeight * 0.7
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
        height: cellHeight
        visible: (_controller.telemetryRRSSI > 0) && (_controller.telemetryLRSSI > 0)
        anchors.verticalCenter: parent.verticalCenter
        color:  getRSSIColor(Math.min(_controller.telemetryRRSSI,_controller.telemetryLRSSI));
        border.color: "#00000000"
        border.width: 0
        Image {
            source: "qrc:/res/AntennaT";
            width: cellHeight * 0.7
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

    Rectangle {
        id: batteryStatus
        width:  activeVehicle.batteryConsumed < 0.0 ? getProportionalDimmension(60) : getProportionalDimmension(80)
        height: cellHeight
        anchors.verticalCenter: parent.verticalCenter
        color:  getBatteryColor();
        border.color: "#00000000"
        border.width: 0
        Image {
            source: getBatteryIcon();
            height: getProportionalDimmension(20)
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: getProportionalDimmension(6)
            mipmap: true
            smooth: true
        }

        QGCLabel {
            visible: batteryStatus.visible && activeVehicle.batteryConsumed < 0.0
            text: (activeVehicle.batteryVoltage > 0) ? activeVehicle.batteryVoltage.toFixed(1) + 'V' : '---';
            font.pixelSize: ScreenTools.smallFontPixelSize
            font.weight: Font.DemiBold
            anchors.right: parent.right
            anchors.rightMargin: getProportionalDimmension(6)
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignRight
            color: colorWhite
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right:          parent.right
            anchors.rightMargin:    getProportionalDimmension(6)
            visible: batteryStatus.visible && activeVehicle.batteryConsumed >= 0.0
            QGCLabel {
                text: (activeVehicle.batteryVoltage > 0) ? activeVehicle.batteryVoltage.toFixed(1) + 'V' : '---';
                width: getProportionalDimmension(30)
                horizontalAlignment: Text.AlignRight
                font.pixelSize: ScreenTools.smallFontPixelSize
                font.weight: Font.DemiBold
                color: colorWhite
            }
            QGCLabel {
                text: (activeVehicle.batteryConsumed > 0) ? activeVehicle.batteryConsumed.toFixed(0) + 'mAh' : '---';
                width: getProportionalDimmension(30)
                horizontalAlignment: Text.AlignRight
                font.pixelSize: ScreenTools.smallFontPixelSize
                font.weight: Font.DemiBold
                color: colorWhite
            }
        }
    }

    QGCButton {
        width:                  ScreenTools.defaultFontPixelWidth * 11
        height:                 cellHeight
        anchors.verticalCenter: parent.verticalCenter
        text:                   activeVehicle.armed ? "Armed" : "Disarmed"

        menu: Menu {
            MenuItem {
                enabled: !activeVehicle.armed
                text: "Arm"

                onTriggered: activeVehicle.armed = true
            }

            MenuItem {
                enabled: activeVehicle.armed
                text: "Disarm"

                onTriggered: activeVehicle.armed = false
            }
        }
    }

    QGCButton {
        width:                  ScreenTools.defaultFontPixelWidth * 16
        height:                 cellHeight
        anchors.verticalCenter: parent.verticalCenter
        text:                   activeVehicle.flightMode

        menu: activeVehicle.flightModeSetAvailable ? flightModesMenu : null

        Menu {
            id: flightModesMenu
        }

        Component {
            id: flightModeMenuItemComponent

            MenuItem {
                checkable:      true
                checked:        activeVehicle.flightMode == text
                onTriggered:    activeVehicle.flightMode = text
            }
        }

        property var flightModesMenuItems: []

        function updateFlightModesMenu() {
            if (activeVehicle.flightModeSetAvailable) {
                // Remove old menu items
                for (var i=0; i<flightModesMenuItems.length; i++) {
                    flightModesMenu.removeItem(flightModesMenuItems[i])
                }
                flightModesMenuItems.length = 0

                // Add new items
                for (var i=0; i<activeVehicle.flightModes.length; i++) {
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
    }

    Rectangle {
        width:                  ScreenTools.defaultFontPixelWidth * 4
        height:                 cellHeight
        anchors.verticalCenter: parent.verticalCenter
        color:                  colorBlue
        border.width:           0
        visible:                activeVehicle.hilMode

        QGCLabel {
            anchors.fill:           parent
            horizontalAlignment:    Text.AlignHCenter
            verticalAlignment:      Text.AlignVCenter
            text:                   "HIL"
        }
    }

} // Row
