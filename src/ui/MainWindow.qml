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

import QtQuick          2.5
import QtQuick.Controls 1.2
import QtPositioning    5.2

import QGroundControl                       1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controls              1.0
import QGroundControl.FlightDisplay         1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0

/// Qml for MainWindow
Item {
    id: mainWindow

    readonly property string _planViewSource:   "MissionEditor.qml"
    readonly property string _setupViewSource:  "SetupView.qml"

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }

    property real   tbHeight:           ScreenTools.isMobile ? (ScreenTools.isTinyScreen ? (mainWindow.width * 0.0666) : (mainWindow.width * 0.05)) : ScreenTools.defaultFontPixelSize * 4
    property int    tbCellHeight:       tbHeight * 0.75
    property real   tbSpacing:          ScreenTools.isMobile ? width * 0.00824 : 9.54
    property real   tbButtonWidth:      tbCellHeight * 1.35
    property real   availableHeight:    height - tbHeight
    property real   menuButtonWidth:    (tbButtonWidth * 2) + (tbSpacing * 4) + 1

    property var    defaultPosition:    QtPositioning.coordinate(37.803784, -122.462276)
    property var    tabletPosition:     defaultPosition

    property var    currentPopUp:       null
    property real   currentCenterX:     0
    property var    activeVehicle:      multiVehicleManager.activeVehicle
    property string formatedMessage:    activeVehicle ? activeVehicle.formatedMessage : ""

    Connections {

        target: controller

        onShowFlyView: {
            if(currentPopUp) {
                currentPopUp.close()
            }
            flightView.visible          = true
            setupViewLoader.visible     = false
            planViewLoader.visible      = false
        }

        onShowPlanView: {
            if(currentPopUp) {
                currentPopUp.close()
            }
            if (planViewLoader.source   != _planViewSource) {
                planViewLoader.source   = _planViewSource
            }
            flightView.visible          = false
            setupViewLoader.visible     = false
            planViewLoader.visible      = true
        }

        onShowSetupView: {
            if(currentPopUp) {
                currentPopUp.close()
            }
            if (setupViewLoader.source  != _setupViewSource) {
                setupViewLoader.source  = _setupViewSource
            }
            flightView.visible          = false
            setupViewLoader.visible     = true
            planViewLoader.visible      = false
        }

        onShowToolbarMessage: toolBar.showToolbarMessage(message)

        // The following are use for unit testing only

        onShowSetupFirmware:            setupViewLoader.item.showFirmwarePanel()
        onShowSetupParameters:          setupViewLoader.item.showParametersPanel()
        onShowSetupSummary:             setupViewLoader.item.showSummaryPanel()
        onShowSetupVehicleComponent:    setupViewLoader.item.showVehicleComponentPanel(vehicleComponent)
    }

    //-- Detect tablet position
    PositionSource {
        id:             positionSource
        updateInterval: 1000
        active:         false
        onPositionChanged: {
            if(positionSource.valid) {
                if(positionSource.position.coordinate.latitude) {
                    if(Math.abs(positionSource.position.coordinate.latitude)  > 0.001) {
                        if(positionSource.position.coordinate.longitude) {
                            if(Math.abs(positionSource.position.coordinate.longitude)  > 0.001) {
                                tabletPosition = positionSource.position.coordinate
                            }
                        }
                    }
                }
            }
            positionSource.stop()
        }
    }

    function showLeftMenu() {
        if(!leftPanel.visible && !leftPanel.item.animateShowDialog.running) {
            leftPanel.visible = true
            leftPanel.item.animateShowDialog.start()
        } else if(leftPanel.visible && !leftPanel.item.animateShowDialog.running) {
            //-- If open, toggle it closed
            hideLeftMenu()
        }
    }

    function hideLeftMenu() {
        if(leftPanel.visible && !leftPanel.item.animateHideDialog.running) {
            leftPanel.item.animateHideDialog.start()
        }
    }

    function setMapInteractive(enabled) {
        flightView.interactive = enabled
    }

    onFormatedMessageChanged: {
        if(messageArea.visible) {
            messageText.append(formatedMessage)
            //-- Hack to scroll down
            messageFlick.flick(0,-500)
        }
    }

    function showMessageArea() {
        if(currentPopUp) {
            currentPopUp.close()
        }
        if(multiVehicleManager.activeVehicleAvailable) {
            messageText.text = activeVehicle.formatedMessages
            //-- Hack to scroll to last message
            for (var i = 0; i < activeVehicle.messageCount; i++)
                messageFlick.flick(0,-5000)
            activeVehicle.resetMessages()
        } else {
            messageText.text = "No Messages"
        }
        currentPopUp = messageArea
        messageArea.visible = true
        mainWindow.setMapInteractive(false)
    }

    function showPopUp(dropItem, centerX) {
        if(currentPopUp) {
            currentPopUp.close()
        }
        indicatorDropdown.centerX = centerX
        indicatorDropdown.sourceComponent = dropItem
        indicatorDropdown.visible = true
        currentPopUp = indicatorDropdown
    }

    //-- Left Settings Menu
    Loader {
        id:                 leftPanel
        anchors.fill:       mainWindow
        visible:            false
        z:                  QGroundControl.zOrderTopMost + 100
    }

    //-- Main UI

    MainToolBar {
        id:                 toolBar
        height:             tbHeight
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
        mainWindow:         mainWindow
        opaqueBackground:   leftPanel.visible
        isBackgroundDark:   flightView.isBackgroundDark
        z:                  QGroundControl.zOrderTopMost
        Component.onCompleted: {
            leftPanel.source = "MainWindowLeftPanel.qml"
        }
    }

    FlightDisplayView {
        id:                 flightView
        anchors.fill:       parent
        availableHeight:    mainWindow.availableHeight
        visible:            true
        Component.onCompleted: {
            positionSource.start()
        }
    }

    Loader {
        id:                 planViewLoader
        anchors.fill:       parent
        visible:            false
    }

    Loader {
        id:                 setupViewLoader
        anchors.fill:       parent
        visible:            false
    }

    //-------------------------------------------------------------------------
    //-- Dismiss Pop Up Messages
    MouseArea {
        visible:        currentPopUp != null
        enabled:        currentPopUp != null
        anchors.fill:   parent
        onClicked: {
            currentPopUp.close()
        }
    }

    //-------------------------------------------------------------------------
    //-- Indicator Drop Down Info
    Loader {
        id: indicatorDropdown
        visible: false
        property real centerX: 0
        function close() {
            sourceComponent = null
            mainWindow.currentPopUp = null
        }
    }

    //-------------------------------------------------------------------------
    //-- System Message Area
    Rectangle {
        id:                 messageArea

        function close() {
            currentPopUp = null
            messageText.text    = ""
            mainWindow.setMapInteractive(true)
            messageArea.visible = false
        }

        width:              mainWindow.width  * 0.5
        height:             mainWindow.height * 0.5
        color:              Qt.rgba(0,0,0,0.75)
        visible:            false
        radius:             ScreenTools.defaultFontPixelHeight * 0.5
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.top:                parent.top
        anchors.topMargin:          tbHeight + ScreenTools.defaultFontPixelHeight
        Flickable {
            id:                 messageFlick
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.fill:       parent
            contentHeight:      messageText.height
            contentWidth:       messageText.width
            boundsBehavior:     Flickable.StopAtBounds
            pixelAligned:       true
            clip:               true
            TextEdit {
                id:         messageText
                readOnly:   true
                textFormat: TextEdit.RichText
                color:      "white"
            }
        }
        //-- Dismiss System Message
        Image {
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.top:        parent.top
            anchors.right:      parent.right
            width:              ScreenTools.defaultFontPixelHeight * 1.5
            height:             ScreenTools.defaultFontPixelHeight * 1.5
            source:             "/res/XDelete.svg"
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    messageArea.close()
                }
            }
        }
    }

}
