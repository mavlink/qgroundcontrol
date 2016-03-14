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

import QtQuick 2.5
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0

Rectangle {
    id:     toolBar
    color:  opaqueBackground ? "#404040" : Qt.rgba(0,0,0,0.75)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var  activeVehicle:        QGroundControl.multiVehicleManager.activeVehicle
    property var  mainWindow:           null
    property bool isMessageImportant:   activeVehicle ? !activeVehicle.messageTypeNormal && !activeVehicle.messageTypeNone : false
    property bool isBackgroundDark:     true
    property bool opaqueBackground:     false

    /*
        Dev System (Mac OS)

        qml: Main Window Width:   1008
        qml: Toolbar height:      51.2
        qml: Default font:        12.8
        qml: Font (.75):          9.600000000000001
        qml: Font (.85):          10.88
        qml: Font 1.5):           19.200000000000003
        qml: Default Font Width:  8.328125
        qml: Default Font Height: 12.8
        qml: --
        qml: Real Font Height:    16
        qml: fontHRatio:          1
        qml: --
        qml: cellHeight:          38
        qml: tbFontSmall:         10
        qml: tbFontNormal:        12
        qml: tbFontLarge:         18
        qml: tbSpacing:           9.54

        Nexus 9

        qml: Main Window Width:   2048
        qml: Toolbar height:      90.9312
        qml: Default font:        38
        qml: Font (.75):          28.5
        qml: Font (.85):          32.3
        qml: Font 1.5):           57
        qml: Default Font Width:  20.0625
        qml: Default Font Height: 38
        qml: --
        qml: Real Font Height:    38
        qml: fontHRatio:          2.375
        qml: --
        qml: cellHeight:          68
        qml: tbFontSmall:         23.75
        qml: tbFontNormal:        28.5
        qml: tbFontLarge:         42.75
        qml: tbSpacing:           16.87552

        Nexus 7

        qml: Main Window Width:   1920
        qml: Toolbar height:      85.248
        qml: Default font:        38
        qml: Font (.75):          28.5
        qml: Font (.85):          32.3
        qml: Font 1.5):           57
        qml: Default Font Width:  20.140625
        qml: Default Font Height: 38
        qml: --
        qml: Real Font Height:    38
        qml: fontHRatio:          2.375
        qml: --
        qml: cellHeight:          63
        qml: tbFontSmall:         23.75
        qml: tbFontNormal:        28.5
        qml: tbFontLarge:         42.75
        qml: tbSpacing:           15.820800000000002

        Nexus 4

        qml: Main Window Width:   1196
        qml: Toolbar height:      79.65360000000001
        qml: Default font:        38
        qml: Font (.75):          28.5
        qml: Font (.85):          32.3
        qml: Font 1.5):           57
        qml: Default Font Width:  20.140625
        qml: Default Font Height: 38
        qml: --
        qml: Real Font Height:    38
        qml: fontHRatio:          2.375
        qml: --
        qml: cellHeight:          59
        qml: tbFontSmall:         23.75
        qml: tbFontNormal:        28.5
        qml: tbFontLarge:         42.75
        qml: tbSpacing:           9.85504

    */

    readonly property real  tbFontSmall:    10 * ScreenTools.fontHRatio
    readonly property real  tbFontNormal:   12 * ScreenTools.fontHRatio
    readonly property real  tbFontLarge:    18 * ScreenTools.fontHRatio

    readonly property var   colorGreen:     "#05f068"
    readonly property var   colorOrange:    "#f0ab06"
    readonly property var   colorRed:       "#fc4638"
    readonly property var   colorGrey:      "#7f7f7f"
    readonly property var   colorBlue:      "#636efe"
    readonly property var   colorWhite:     "#ffffff"

    signal showSetupView()
    signal showPlanView()
    signal showFlyView()

    MainToolBarController { id: _controller }

    function checkSetupButton() {
        setupButton.checked = true
    }

    function checkPlanButton() {
        planButton.checked = true
    }

    function checkFlyButton() {
        flyButton.checked = true
    }

    function getBatteryColor() {
        if(activeVehicle) {
            if(activeVehicle.battery.percentRemaining.value > 75) {
                return colorGreen
            }
            if(activeVehicle.battery.percentRemaining.value > 50) {
                return colorOrange
            }
            if(activeVehicle.battery.percentRemaining.value > 0.1) {
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

    Component.onCompleted: {
        //-- TODO: Get this from the actual state
        flyButton.checked = true
    }

    //---------------------------------------------
    // GPS Info
    Component {
        id: gpsInfo
        Rectangle {
            color:          Qt.rgba(0,0,0,0.75)
            width:          gpsCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height:         gpsCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            Column {
                id:                 gpsCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(gpsGrid.width, gpsLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent
                QGCLabel {
                    id:         gpsLabel
                    text:       (activeVehicle && activeVehicle.gps.count.value >= 0) ? "GPS Status" : "GPS Data Unavailable"
                    font.weight:Font.DemiBold
                    color:      colorWhite
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                GridLayout {
                    id:                 gpsGrid
                    visible:            (activeVehicle && activeVehicle.gps.count.value >= 0)
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2
                    QGCLabel {
                        text:   "GPS Count:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   activeVehicle ? activeVehicle.gps.count.valueString : "N/A"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "GPS Lock:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   activeVehicle ? activeVehicle.gps.lock.enumStringValue : "N/A"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "HDOP:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   activeVehicle ? activeVehicle.gps.hdop.valueString : "--.--"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "VDOP:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   activeVehicle ? activeVehicle.gps.vdop.valueString : "--.--"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "Course Over Ground:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   activeVehicle ? activeVehicle.gps.courseOverGround.valueString : "--.--"
                        color:  colorWhite
                    }
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
            color:          Qt.rgba(0,0,0,0.75)
            width:          battCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height:         battCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius:         ScreenTools.defaultFontPixelHeight * 0.5

            Column {
                id:                 battCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(battGrid.width, battLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:         battLabel
                    text:       "Battery Status"
                    color:      colorWhite
                    font.weight:Font.DemiBold
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                GridLayout {
                    id:                 battGrid
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2
                    QGCLabel {
                        text:   "Voltage:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   (activeVehicle && activeVehicle.battery.voltage.value != -1) ? (activeVehicle.battery.voltage.valueString + " " + activeVehicle.battery.voltage.units) : "N/A"
                        color:  getBatteryColor()
                    }
                    QGCLabel {
                        text:   "Accumulated Consumption:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   (activeVehicle && activeVehicle.battery.mahConsumed.value != -1) ? (activeVehicle.battery.mahConsumed.valueString + " " + activeVehicle.battery.mahConsumed.units) : "N/A"
                        color:  getBatteryColor()
                    }
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
            color:          Qt.rgba(0,0,0,0.75)
            width:          rcrssiCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height:         rcrssiCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            Column {
                id:                 rcrssiCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(rcrssiGrid.width, rssiLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent
                QGCLabel {
                    id:         rssiLabel
                    text:       activeVehicle ? (activeVehicle.rcRSSI > 0 ? "RC RSSI Status" : "RC RSSI Data Unavailable") : "N/A"
                    color:      colorWhite
                    font.weight:Font.DemiBold
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                GridLayout {
                    id:                 rcrssiGrid
                    visible:            activeVehicle && activeVehicle.rcRSSI > 0
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2
                    QGCLabel {
                        text:   "RSSI:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   activeVehicle ? (activeVehicle.rcRSSI + "%") : 0
                        color:  colorWhite
                    }
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
            color:          Qt.rgba(0,0,0,0.75)
            width:          telemCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height:         telemCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            Column {
                id:                 telemCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(telemGrid.width, telemLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent
                QGCLabel {
                    id:         telemLabel
                    text:       "Telemetry RSSI Status"
                    color:      colorWhite
                    font.weight:Font.DemiBold
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                GridLayout {
                    id:                 telemGrid
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2
                    QGCLabel {
                        text:   "Local RSSI:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   _controller.telemetryLRSSI + " dBm"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "Remote RSSI:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   _controller.telemetryRRSSI + " dBm"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "RX Errors:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   _controller.telemetryRXErrors
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "Errors Fixed:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   _controller.telemetryFixed
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "TX Buffer:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   _controller.telemetryTXBuffer
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "Local Noise:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   _controller.telemetryLNoise
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   "Remote Noise:"
                        color:  colorWhite
                    }
                    QGCLabel {
                        text:   _controller.telemetryRNoise
                        color:  colorWhite
                    }
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
    // Toolbar Row
    Row {
        id:             viewRow
        height:         mainWindow.tbCellHeight
        spacing:        mainWindow.tbSpacing
        anchors.left:   parent.left
        anchors.leftMargin:     mainWindow.tbSpacing
        anchors.verticalCenter: parent.verticalCenter

        ExclusiveGroup { id: mainActionGroup }

        QGCToolBarButton {
            id:                 preferencesButton
            width:              mainWindow.tbButtonWidth
            height:             mainWindow.tbCellHeight
            source:             "/qmlimages/Hamburger.svg"
            onClicked: {
                mainWindow.showLeftMenu();
                preferencesButton.checked = false;
            }
        }

        Rectangle {
            height: mainWindow.tbCellHeight
            width:  1
            color: Qt.rgba(1,1,1,0.45)
        }

        QGCToolBarButton {
            id:                 setupButton
            width:              mainWindow.tbButtonWidth
            height:             mainWindow.tbCellHeight
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Gears.svg"
            onClicked:          toolBar.showSetupView()
        }

        Rectangle {
            height: mainWindow.tbCellHeight
            width:  1
            color: Qt.rgba(1,1,1,0.45)
        }

        QGCToolBarButton {
            id:                 planButton
            width:              mainWindow.tbButtonWidth
            height:             mainWindow.tbCellHeight
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Plan.svg"
            onClicked:          toolBar.showPlanView()
        }

        Rectangle {
            height: mainWindow.tbCellHeight
            width:  1
            color: Qt.rgba(1,1,1,0.45)
        }

        QGCToolBarButton {
            id:                 flyButton
            width:              mainWindow.tbButtonWidth
            height:             mainWindow.tbCellHeight
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/PaperPlane.svg"
            onClicked:          toolBar.showFlyView()
        }

        Rectangle {
            height: mainWindow.tbCellHeight
            width:  1
            color: Qt.rgba(1,1,1,0.45)
        }

    }

    Item {
        id:                     vehicleIndicators
        height:                 mainWindow.tbCellHeight
        anchors.leftMargin:     mainWindow.tbSpacing * 2
        anchors.left:           viewRow.right
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter

        property bool vehicleConnectionLost: activeVehicle ? activeVehicle.connectionLost : false

        Loader {
            source:                 activeVehicle && !parent.vehicleConnectionLost ? "MainToolBarIndicators.qml" : ""
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCLabel {
            id:                     connectionLost
            text:                   "COMMUNICATION LOST"
            font.pixelSize:         tbFontLarge
            font.weight:            Font.DemiBold
            color:                  colorRed
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            visible:                parent.vehicleConnectionLost

        }

        QGCButton {
            anchors.rightMargin:     mainWindow.tbSpacing * 2
            anchors.right:          parent.right
            anchors.verticalCenter: parent.verticalCenter
            text:                   "Disconnect"
            visible:                parent.vehicleConnectionLost
            onClicked:              activeVehicle.disconnectInactiveVehicle()
        }
    }

    // Progress bar
    Rectangle {
        id:             progressBar
        anchors.bottom: parent.bottom
        height:         toolBar.height * 0.05
        width:          parent.width * _controller.progressBarValue
        color:          colorGreen
    }

}
