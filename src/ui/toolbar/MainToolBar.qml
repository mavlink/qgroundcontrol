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

/**
 * @file
 *   @brief QGC Main Tool Bar
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0

Rectangle {
    id:             toolBarHolder
    anchors.left:   parent.left
    anchors.right:  parent.right
    height:         toolBarHeight
    color:          qgcPal.window

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var activeVehicle: multiVehicleManager.activeVehicle

    readonly property real toolBarHeight:   ScreenTools.defaultFontPixelHeight * 3
    property int cellSpacerSize:            ScreenTools.isMobile ? getProportionalDimmension(6) : getProportionalDimmension(4)
    readonly property int cellHeight:       toolBarHeight * 0.75

    readonly property real horizontalMargins:   ScreenTools.defaultFontPixelWidth / 2
    readonly property real verticalMargins:     ScreenTools.defaultFontPixelHeight / 4

    readonly property var colorBlue:    "#1a6eaa"
    readonly property var colorGreen:   "#329147"
    readonly property var colorRed:     "#942324"
    readonly property var colorOrange:  "#a76f26"
    readonly property var colorWhite:   "#f0f0f0"

    property var colorOrangeText: (qgcPal.globalTheme === QGCPalette.Light) ? "#b75711" : "#ea8225"
    property var colorRedText:    (qgcPal.globalTheme === QGCPalette.Light) ? "#ee1112" : "#ef2526"
    property var colorGreenText:  (qgcPal.globalTheme === QGCPalette.Light) ? "#046b1b" : "#00d930"
    property var colorWhiteText:  (qgcPal.globalTheme === QGCPalette.Light) ? "#343333" : "#f0f0f0"

    MainToolBarController { id: _controller }

    function showToolbarMessage(message) {
        toolBarMessage.text = message
        if (toolBarMessage.contentHeight > toolBarMessageCloseButton.height) {
            toolBarHolder.height = toolBarHeight + toolBarMessage.contentHeight + (verticalMargins * 2)
        } else {
            toolBarHolder.height = toolBarHeight + toolBarMessageCloseButton.height + (verticalMargins * 2)
        }
        toolBarMessageArea.visible = true
    }

    function getProportionalDimmension(val) {
        return toolBarHeight * val / 40
    }

    function getMessageColor() {
        if (activeVehicle.messageTypeNone)
            return qgcPal.button;
        if (activeVehicle.messageTypeNorma)
            return colorBlue;
        if (activeVehicle.messageTypeWarning)
            return colorOrange;
        if (activeVehicle.messageTypeError)
            return colorRed;
        // Cannot be so make make it obnoxious to show error
        return "purple";
    }

    function getMessageIcon() {
        if (activeVehicle.messageTypeNormal || activeVehicle.messageTypeNone)
            return "qrc:/res/Megaphone";
        else
            return "qrc:/res/Yield";
    }

    function getBatteryIcon() {
        if(activeVehicle.batteryPercent < 20.0)
            return "qrc:/res/Battery_0";
        else if(activeVehicle.batteryPercent < 40.0)
            return "qrc:/res/Battery_20";
        else if(activeVehicle.batteryPercent < 60.0)
            return "qrc:/res/Battery_40";
        else if(activeVehicle.batteryPercent < 80.0)
            return "qrc:/res/Battery_60";
        else if(activeVehicle.batteryPercent < 90.0)
            return "qrc:/res/Battery_80";
        else
            return "qrc:/res/Battery_100";
    }

    function getBatteryColor() {
        if (activeVehicle.batteryPercent > 40.0)
            return colorGreen;
        if(activeVehicle.batteryPercent > 0.01)
            return colorRed;
        // This means there is no battery level data
        return colorBlue;
    }

    function getSatelliteColor() {
        // No GPS data
        if (activeVehicle.satelliteCount < 0)
            return qgcPal.button
        // No Lock
        if(activeVehicle.satelliteLock < 2)
            return colorRed;
        // 2D Lock
        if(activeVehicle.satelliteLock === 2)
            return colorBlue;
        // Lock is 3D or more
        return colorGreen;
    }

    function getRSSIColor(value) {
        if(value < 10)
            return colorRed;
        if(value < 50)
            return colorOrange;
        return colorGreen;
    }

    function showMavStatus() {
         return (multiVehicleManager.activeVehicleAvailable && activeVehicle.heartbeatTimeout === 0 && _controller.connectionCount > 0);
    }

    //-------------------------------------------------------------------------
    //-- Main menu for Mobile Devices
    Menu {
        id: mobileMenu

        ExclusiveGroup { id: mainMenuGroup }

        MenuItem {
            id:             flyViewShowing
            text:           "Fly"
            checkable:      true
            checked:        true
            exclusiveGroup: mainMenuGroup

            onTriggered: {
                checked = true
                _controller.onFlyView();
            }
        }

        MenuItem {
            id:             setupViewShowing
            text:           "Setup"
            checkable:      true
            exclusiveGroup: mainMenuGroup

            onTriggered: {
                checked = true
                _controller.onSetupView();
            }
        }

        MenuItem {
            id:             planViewShowing
            text:           "Plan"
            checkable:      true
            exclusiveGroup: mainMenuGroup

            onTriggered: {
                checked = true
                _controller.onPlanView();
            }
        }

        MenuSeparator { }


        MenuItem {
            text:           "QGroundControl Settings"

            onTriggered: controller.showSettings()
        }
    } // Menu

    /*
    Row {
        id:         toolRow
        height:     cellHeight
        spacing:    getProportionalDimmension(4)
    }
    */

    Loader {
        id:                 desktopToolsLoader
        height:             cellHeight
        x:                  horizontalMargins
        y:                  (toolBarHeight - cellHeight) / 2
        sourceComponent:    ScreenTools.isMobile ? undefined : desktopTools
    }

    //---------------------------------------------------------------------
    //-- Indicators
    Row {
        id:                     row12
        x:                      horizontalMargins + (ScreenTools.isMobile ? 0: desktopToolsLoader.item.width)
        height:                 cellHeight
        spacing:                cellSpacerSize
        anchors.top:            desktopToolsLoader.top
        anchors.verticalCenter: desktopToolsLoader.verticalCenter

        //-- "Hamburger" menu for Mobile Devices
        Item {
            id:         actionButton
            visible:    ScreenTools.isMobile
            height:     cellHeight
            width:      cellHeight
            Image {
                id:             buttomImg
                anchors.fill:   parent
                source:         "/qmlimages/buttonMore.svg"
                mipmap:         true
                smooth:         true
                antialiasing:   true
                fillMode:       Image.PreserveAspectFit
            }
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    if (mouse.button == Qt.LeftButton)
                    {
                        mobileMenu.popup();
                    }
                }
            }
        }

        //-- Separator if Hamburger menu is visible
        Rectangle {
            visible:    actionButton.visible
            height:     cellHeight
            width:      cellHeight
            color:      "#00000000"
            anchors.verticalCenter: parent.verticalCenter
        }

        Loader {
            id:         activeVehicleLoader
            visible:    showMavStatus()
            source:     multiVehicleManager.activeVehicleAvailable ? "MainToolBarActiveVehicleComponent.qml" : ""

            property real cellHeight:       toolBarHolder.cellHeight
            property real cellSpacerSize:   toolBarHolder.cellSpacerSize
        }

        Rectangle {
            id: connectionStatus
            width: getProportionalDimmension(160)
            height: cellHeight
            visible: (_controller.connectionCount > 0 && multiVehicleManager.activeVehicleAvailable && activeVehicle.heartbeatTimeout != 0)
            anchors.verticalCenter: parent.verticalCenter
            color: "#00000000"
            border.color: "#00000000"
            border.width: 0

            QGCLabel {
                id: connectionStatusText
                text: qsTr("CONNECTION LOST")
                font.pixelSize: ScreenTools.defaultFontPixelSize
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                color: colorRedText
            }
        }
    } // Row

    Row {
        id:                     connectRow
        anchors.rightMargin:    verticalMargins
        anchors.right:          parent.right
        anchors.top:            desktopToolsLoader.top
        anchors.verticalCenter: desktopToolsLoader.verticalCenter
        height:                 desktopToolsLoader.height
        spacing:                cellSpacerSize

        Menu {
            id: connectMenu
            Component.onCompleted: {
                _controller.configListChanged.connect(connectMenu.updateConnectionList);
                connectMenu.updateConnectionList();
            }
            function addMenuEntry(name) {
                var label = "Add Connection"
                if(name !== "")
                    label = name;
                var mItem = connectMenu.addItem(label);
                var menuSlot = function() {_controller.onConnect(name)};
                mItem.triggered.connect(menuSlot);
            }
            function updateConnectionList() {
                connectMenu.clear();
                for(var i = 0; i < _controller.configList.length; i++) {
                    connectMenu.addMenuEntry(_controller.configList[i]);
                }
                if(_controller.configList.length > 0) {
                    connectMenu.addSeparator();
                }
                // Add "Add Connection" to the list
                connectMenu.addMenuEntry("");
            }
        }

        QGCButton {
            id:         connectButton
            width:      getProportionalDimmension(100)
            visible:    _controller.connectionCount === 0
            text:       qsTr("Connect")
            menu:       connectMenu
        }

        QGCButton {
            id:         disconnectButton
            width:      getProportionalDimmension(100)
            visible:    _controller.connectionCount === 1
            text:       qsTr("Disconnect")
            onClicked: {
                _controller.onDisconnect("");
            }
        }

        Menu {
            id: disconnectMenu
            Component.onCompleted: {
                _controller.connectedListChanged.connect(disconnectMenu.onConnectedListChanged)
            }
            function addMenuEntry(name) {
                var mItem = disconnectMenu.addItem(name);
                var menuSlot = function() {_controller.onDisconnect(name)};
                mItem.triggered.connect(menuSlot);
            }
            function onConnectedListChanged(conList) {
                disconnectMenu.clear();
                for(var i = 0; i < conList.length; i++) {
                    disconnectMenu.addMenuEntry(conList[i]);
                }
            }
        }

        QGCButton {
            id:         multidisconnectButton
            width:      getProportionalDimmension(100)
            text:       "Disconnect"
            visible:    _controller.connectionCount > 1
            menu:       disconnectMenu
        }
    } // Row

    // Progress bar
    Rectangle {
        id:             progressBar
        anchors.top:    desktopToolsLoader.bottom
        height:         getProportionalDimmension(3)
        width:          parent.width * _controller.progressBarValue
        color:          qgcPal.text
    }

    // Toolbar message area
    Rectangle {
        id:                     toolBarMessageArea
        anchors.leftMargin:     horizontalMargins
        anchors.rightMargin:    horizontalMargins
        anchors.topMargin:      verticalMargins
        anchors.bottomMargin:   verticalMargins
        anchors.top:            progressBar.bottom
        anchors.bottom:         parent.bottom
        anchors.left:           parent.left
        anchors.right:          parent.right
        color:                  qgcPal.windowShadeDark
        visible:                false

        QGCLabel {
            id:             toolBarMessage
            anchors.fill:   parent
            wrapMode:       Text.WordWrap
			color:			qgcPal.warningText
        }

        QGCButton {
            id:                     toolBarMessageCloseButton
            anchors.rightMargin:    horizontalMargins
            anchors.top:            parent.top
            anchors.right:          parent.right
			primary:				true
            text:                   "Close Message"

            onClicked: {
                parent.visible = false
                toolBarHolder.height = toolBarHeight
                _controller.onToolBarMessageClosed()
            }
        }
    }

    Component {
        id: desktopTools

        //---------------------------------------------------------------------
        //-- Main menu for Non Mobile Devices (Chevron Buttons)
        Row {
            id:             row11
            height:         cellHeight
            spacing:        -getProportionalDimmension(12)

            Connections {
                target: ScreenTools
                onRepaintRequested: {
                    setupButton.repaintChevron   = true;
                    planButton.repaintChevron    = true;
                    flyButton.repaintChevron     = true;
                }
            }
            Connections {
                target:controller
                onShowFlyView:  { flyButton.checked   = true }
                onShowPlanView: { planButton.checked  = true }
                onShowSetupView:{ setupButton.checked = true }
            }


            ExclusiveGroup { id: mainActionGroup }

            QGCToolBarButton {
                id:             setupButton
                width:          getProportionalDimmension(90)
                height:         cellHeight
                exclusiveGroup: mainActionGroup
                text:           "Setup"

                onClicked: {
                    checked = true
                    _controller.onSetupView();
                }
                z: 1000
            }

            QGCToolBarButton {
                id:             planButton
                width:          getProportionalDimmension(90)
                height:         cellHeight
                exclusiveGroup: mainActionGroup
                text:           "Plan"

                onClicked: {
                    checked = true
                    _controller.onPlanView();
                }
                z: 900
            }

            QGCToolBarButton {
                id:             flyButton
                width:          getProportionalDimmension(90)
                height:         cellHeight
                exclusiveGroup: mainActionGroup
                text:           "Fly"
                checked:        true

                onClicked: {
                    checked = true
                    _controller.onFlyView();
                }
                z: 800
            }
        } // Row
    } // Component - desktopTools
} // Rectangle
