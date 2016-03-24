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
import QtQuick.Dialogs  1.2
import QtPositioning    5.2

import QGroundControl                       1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controls              1.0
import QGroundControl.FlightDisplay         1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0

/// Inner common QML for mainWindow
Item {
    id: mainWindow

    signal reallyClose

    readonly property string _planViewSource:   "MissionEditor.qml"
    readonly property string _setupViewSource:  "SetupView.qml"

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property real   tbHeight:           ScreenTools.isMobile ? (ScreenTools.isTinyScreen ? (mainWindow.width * 0.0666) : (mainWindow.width * 0.05)) : ScreenTools.defaultFontPixelSize * 4
    property int    tbCellHeight:       tbHeight * 0.75
    property real   tbSpacing:          ScreenTools.isMobile ? width * 0.00824 : 9.54
    property real   tbButtonWidth:      tbCellHeight * 1.35
    property real   menuButtonWidth:    (tbButtonWidth * 2) + (tbSpacing * 4) + 1
    property var    gcsPosition:        QtPositioning.coordinate()  // Starts as invalid coordinate
    property var    currentPopUp:       null
    property real   currentCenterX:     0
    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property string formatedMessage:    activeVehicle ? activeVehicle.formatedMessage : ""

    function showFlyView() {
        if(currentPopUp) {
            currentPopUp.close()
        }
        flightView.visible          = true
        setupViewLoader.visible     = false
        planViewLoader.visible      = false
        toolBar.checkFlyButton()
    }

    function showPlanView() {
        if(currentPopUp) {
            currentPopUp.close()
        }
        if (planViewLoader.source   != _planViewSource) {
            planViewLoader.source   = _planViewSource
        }
        flightView.visible          = false
        setupViewLoader.visible     = false
        planViewLoader.visible      = true
        toolBar.checkPlanButton()
    }

    function showSetupView() {
        if(currentPopUp) {
            currentPopUp.close()
        }
        if (setupViewLoader.source  != _setupViewSource) {
            setupViewLoader.source  = _setupViewSource
        }
        flightView.visible          = false
        setupViewLoader.visible     = true
        planViewLoader.visible      = false
        toolBar.checkSetupButton()
    }

    // The following are use for unit testing only

    function showSetupFirmware() {
        setupViewLoader.item.showFirmwarePanel()
    }

    function showSetupParameters() {
        setupViewLoader.item.showParametersPanel()
    }

    function showSetupSummary() {
        setupViewLoader.item.showSummaryPanel()
    }

    function showSetupVehicleComponent(vehicleComponent) {
        setupViewLoader.item.showVehicleComponentPanel(vehicleComponent)
    }

    /// Start the process of closing QGroundControl. Prompts the user are needed.
    function attemptWindowClose() {
        unsavedMissionCloseDialog.check()
    }

    function finishCloseProcess() {
        QGroundControl.linkManager.shutdown()
        // The above shutdown causes a flurry of activity as the vehicle components are removed. This in turn
        // causes the Windows Version of Qt to crash if you allow the close event to be accepted. In order to prevent
        // the crash, we ignore the close event and setup a delayed timer to close the window after things settle down.
        delayedWindowCloseTimer.start()
    }

    MessageDialog {
        id:                 unsavedMissionCloseDialog
        title:              qsTr("QGroundControl close")
        text:               qsTr("You have a mission edit in progress which has not been saved/sent. If you close you will lose changes. Are you sure you want to close?")
        standardButtons:    StandardButton.Yes | StandardButton.No
        modality:           Qt.ApplicationModal
        visible:            false

        onYes: activeConnectionsCloseDialog.check()

        function check() {
            if (planViewLoader.item && planViewLoader.item.syncNeeded) {
                unsavedMissionCloseDialog.open()
            } else {
                activeConnectionsCloseDialog.check()
            }
        }
    }

    MessageDialog {
        id:                 activeConnectionsCloseDialog
        title:              qsTr("QGroundControl close")
        text:               qsTr("There are still active connections to vehicles. Do you want to disconnect these before closing?")
        standardButtons:    StandardButton.Yes | StandardButton.Cancel
        modality:           Qt.ApplicationModal
        visible:            false
        onYes:              finishCloseProcess()

        function check() {
            if (QGroundControl.multiVehicleManager.activeVehicle) {
                activeConnectionsCloseDialog.open()
            } else {
                finishCloseProcess()
            }
        }
    }

    Timer {
        id:         delayedWindowCloseTimer
        interval:   1500
        running:    false
        repeat:     false

        onTriggered: {
            mainWindow.reallyClose()
        }
    }


    //-- Detect tablet position
    PositionSource {
        id:             positionSource
        updateInterval: 1000
        active:         true

        onPositionChanged: {
            if(positionSource.valid) {
                if(positionSource.position.coordinate.latitude) {
                    if(Math.abs(positionSource.position.coordinate.latitude)  > 0.001) {
                        if(positionSource.position.coordinate.longitude) {
                            if(Math.abs(positionSource.position.coordinate.longitude)  > 0.001) {
                                gcsPosition = positionSource.position.coordinate
                            }
                        }
                    }
                }
            }
        }
    }

    property var messageQueue: []

    function showMessage(message) {
        if(criticalMmessageArea.visible) {
            messageQueue.push(message)
        } else {
            criticalMessageText.text = message
            criticalMmessageArea.visible = true
        }
    }

    function showLeftMenu() {
        if(!leftPanel.visible) {
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
        if(QGroundControl.multiVehicleManager.activeVehicleAvailable) {
            messageText.text = activeVehicle.formatedMessages
            //-- Hack to scroll to last message
            for (var i = 0; i < activeVehicle.messageCount; i++)
                messageFlick.flick(0,-5000)
            activeVehicle.resetMessages()
        } else {
            messageText.text = qsTr("No Messages")
        }
        currentPopUp = messageArea
        messageArea.visible = true
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
        anchors.fill:       parent
        visible:            false
        z:                  QGroundControl.zOrderTopMost + 100
        active:             visible
        source:             "MainWindowLeftPanel.qml"
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

        onShowSetupView:    mainWindow.showSetupView()
        onShowPlanView:     mainWindow.showPlanView()
        onShowFlyView:      mainWindow.showFlyView()
    }

    FlightDisplayView {
        id:             flightView
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.top:    toolBar.bottom
        anchors.bottom: parent.bottom
        visible:        true
    }

    Loader {
        id:                 planViewLoader
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.top:    toolBar.bottom
        anchors.bottom: parent.bottom
        visible:            false
    }

    Loader {
        id:                 setupViewLoader
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.top:    toolBar.bottom
        anchors.bottom: parent.bottom
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
            messageArea.visible = false
        }

        width:              mainWindow.width  * 0.5
        height:             mainWindow.height * 0.5
        color:              Qt.rgba(0,0,0,0.8)
        visible:            false
        radius:             ScreenTools.defaultFontPixelHeight * 0.5
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.top:                parent.top
        anchors.topMargin:          tbHeight + ScreenTools.defaultFontPixelHeight
        QGCFlickable {
            id:                 messageFlick
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.fill:       parent
            contentHeight:      messageText.height
            contentWidth:       messageText.width
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
    //-------------------------------------------------------------------------
    //-- Critical Message Area
    Rectangle {
        id: criticalMmessageArea

        function close() {
            //-- Are there messages in the waiting queue?
            if(mainWindow.messageQueue.length) {
                criticalMessageText.text = ""
                //-- Show all messages in queue
                for (var i = 0; i < mainWindow.messageQueue.length; i++) {
                    criticalMessageText.append(mainWindow.messageQueue[i])
                }
                //-- Clear it
                mainWindow.messageQueue = []
            } else {
                criticalMessageText.text = ""
                criticalMmessageArea.visible = false
            }
        }

        width:              mainWindow.width  * 0.55
        height:             ScreenTools.defaultFontPixelHeight * ScreenTools.fontHRatio * 6
        color:              qgcPal.window
        visible:            false
        radius:             ScreenTools.defaultFontPixelHeight * 0.5
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.bottom:             parent.bottom
        anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight

        MouseArea {
            // This MouseArea prevents the Map below it from getting Mouse events. Without this
            // things like mousewheel will scroll the Flickable and then scroll the map as well.
            anchors.fill:       parent
            preventStealing:    true
            onWheel:            wheel.accepted = true
        }

        Flickable {
            id:                 criticalMessageFlick
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.fill:       parent
            contentHeight:      criticalMessageText.height
            contentWidth:       criticalMessageText.width
            boundsBehavior:     Flickable.StopAtBounds
            pixelAligned:       true
            clip:               true

            TextEdit {
                id:             criticalMessageText
                width:          criticalMmessageArea.width - criticalClose.width - (ScreenTools.defaultFontPixelHeight * 2)
                anchors.left:   parent.left
                readOnly:       true
                textFormat:     TextEdit.RichText
                font.weight:    Font.DemiBold
                wrapMode:       TextEdit.WordWrap
                color:          qgcPal.warningText
            }
        }

        //-- Dismiss Critical Message
        QGCColoredImage {
            id:                 criticalClose
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.top:        parent.top
            anchors.right:      parent.right
            width:              ScreenTools.defaultFontPixelHeight * 1.5
            height:             ScreenTools.defaultFontPixelHeight * 1.5
            source:             "/res/XDelete.svg"
            fillMode:           Image.PreserveAspectFit
            color:              qgcPal.warningText

            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    criticalMmessageArea.close()
                }
            }
        }

        //-- More text below indicator
        QGCColoredImage {
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.bottom:     parent.bottom
            anchors.right:      parent.right
            width:              ScreenTools.defaultFontPixelHeight * 1.5
            height:             ScreenTools.defaultFontPixelHeight * 1.5
            source:             "/res/ArrowDown.svg"
            fillMode:           Image.PreserveAspectFit
            visible:            criticalMessageText.lineCount > 5
            color:              qgcPal.warningText

            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    criticalMessageFlick.flick(0,-500)
                }
            }
        }
    }
}

