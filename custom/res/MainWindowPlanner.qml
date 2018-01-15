/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Window   2.2
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtPositioning    5.3

import QGroundControl                       1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controls              1.0
import QGroundControl.FlightDisplay         1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0

/// Native QML top level window
Window {
    id:             _rootWindow
    width:          1280
    height:         720
    minimumWidth:   800
    minimumHeight:  400
    visible:        true

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var    currentPopUp:       null
    property real   currentCenterX:     0
    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property bool   communicationLost:  activeVehicle ? activeVehicle.connectionLost : false

    function showSetupView() {
    }

    function showMessage(message) {
        console.log('Root: ' + message)
    }

    Timer {
        id:        connectionTimer
        interval:  5000
        running:   false;
        repeat:    false;
        onTriggered: {
            //-- Vehicle is gone
            if(activeVehicle && communicationLost) {
                if(!activeVehicle.autoDisconnect) {
                    activeVehicle.disconnectInactiveVehicle()
                }
            }
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle
        onConnectionLostChanged: {
            if(communicationLost) {
                if(activeVehicle && !activeVehicle.autoDisconnect) {
                    //-- Communication lost
                    connectionTimer.start();
                }
            } else {
                connectionTimer.stop();
            }
        }
    }

    Item {
        id: mainWindow
        anchors.fill:   parent

        onHeightChanged: {
            ScreenTools.availableHeight = parent.height - toolBar.height
        }

        function disableToolbar() {
        }

        function enableToolbar() {
        }

        function showSettingsView() {
            rootLoader.sourceComponent = null
            if(currentPopUp) {
                currentPopUp.close()
            }
            //-- In settings view, the full height is available. Set to 0 so it is ignored.
            ScreenTools.availableHeight = 0
            planToolBar.visible = false
            planViewLoader.visible = false
            toolBar.visible = true
            settingsViewLoader.visible = true
        }

        function showPlanView() {
            rootLoader.sourceComponent = null
            if(currentPopUp) {
                currentPopUp.close()
            }
            ScreenTools.availableHeight = parent.height - toolBar.height
            settingsViewLoader.visible = false
            toolBar.visible = false
            planViewLoader.visible = true
            planToolBar.visible = true
        }

        function showFlyView() {
            showSettingsView()
        }

        function showAnalyzeView() {
        }

        property var messageQueue: []

        function showMessage(message) {
            console.log('Main: ' + message)
        }

        function showPopUp(dropItem, centerX) {
            rootLoader.sourceComponent = null
            var oldIndicator = indicatorDropdown.sourceComponent
            if(currentPopUp) {
                currentPopUp.close()
            }
            if(oldIndicator !== dropItem) {
                indicatorDropdown.centerX = centerX
                indicatorDropdown.sourceComponent = dropItem
                indicatorDropdown.visible = true
                currentPopUp = indicatorDropdown
            }
        }

        Rectangle {
            id:                 toolBar
            visible:            false
            height:             ScreenTools.toolbarHeight
            anchors.left:       parent.left
            anchors.right:      indicators.visible ? indicators.left : parent.right
            anchors.top:        parent.top
            color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
            Row {
                id:                     logoRow
                anchors.bottomMargin:   1
                anchors.left:           parent.left
                anchors.top:            parent.top
                anchors.bottom:         parent.bottom
                QGCToolBarButton {
                    id:                 settingsButton
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    source:             "/qmlimages/PaperPlane.svg"
                    logo:               true
                    checked:            false
                    onClicked: {
                        checked = false
                        mainWindow.showPlanView()
                    }
                }
            }
        }

        PlanToolBar {
            id:                 planToolBar
            height:             ScreenTools.toolbarHeight
            anchors.left:       parent.left
            anchors.right:      indicators.visible ? indicators.left : parent.right
            anchors.top:        parent.top
            onShowFlyView: {
                mainWindow.showSettingsView()
            }
            Component.onCompleted: {
                ScreenTools.availableHeight = parent.height - planToolBar.height
                planToolBar.visible = true
            }
        }

        Rectangle {
            id:                         indicators
            color:                      qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
            anchors.right:              parent.right
            anchors.top:                parent.top
            height:                     ScreenTools.toolbarHeight
            width:                      indicatorsRow.width
            visible:                    activeVehicle
            Row {
                id:                     indicatorsRow
                anchors.bottomMargin:   1
                anchors.right:          parent.right
                anchors.top:            parent.top
                anchors.bottom:         parent.bottom
                spacing:                ScreenTools.defaultFontPixelWidth * 3.25
                Rectangle {
                    height:             1
                    width:              1
                }
                Rectangle {
                    height:             parent.height * 0.75
                    width:              1
                    color:              qgcPal.text
                    opacity:            0.5
                    anchors.verticalCenter: parent.verticalCenter
                }
                Loader {
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
                    source:             "/typhoonh/YGPSIndicator.qml"
                }
                Loader {
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
                    source:             "/typhoonh/BatteryIndicator.qml"
                }
            }
        }

        Loader {
            id:                 settingsViewLoader
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        toolBar.bottom
            anchors.bottom:     parent.bottom
            source:             "/qml/AppSettings.qml"
            visible:            false
        }

        Loader {
            id:                 planViewLoader
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        toolBar.bottom
            anchors.bottom:     parent.bottom
            source:             "/qml/PlanView.qml"
            property var toolbar: planToolBar
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
        //-- Loader helper for any child, no matter how deep can display an element
        //   in the middle of the main window.
        Loader {
            id: rootLoader
            anchors.centerIn: parent
        }
        //-------------------------------------------------------------------------
        //-- Indicator Drop Down Info
        Loader {
            id: indicatorDropdown
            visible: false
            property real centerX: 0
            function close() {
                sourceComponent = null
                currentPopUp = null
            }
        }
        //-------------------------------------------------------------------------
        // Progress bar
        Rectangle {
            id:             progressBar
            anchors.top:    parent.top
            anchors.topMargin: ScreenTools.toolbarHeight
            anchors.left:   parent.left
            height:         ScreenTools.toolbarHeight * 0.05
            width:          activeVehicle ? activeVehicle.parameterManager.loadProgress * parent.width : 0
            color:          qgcPal.colorGreen
        }
    }
}

