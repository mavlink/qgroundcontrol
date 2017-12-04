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

    function showSetupView() {
    }

    function showMessage(message) {
        console.log('Root: ' + message)
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

        Rectangle {
            id:                 toolBar
            visible:            false
            height:             ScreenTools.toolbarHeight
            anchors.left:       parent.left
            anchors.right:      parent.right
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
            anchors.right:      parent.right
            anchors.top:        parent.top
            onShowFlyView: {
                mainWindow.showSettingsView()
            }
            Component.onCompleted: {
                ScreenTools.availableHeight = parent.height - planToolBar.height
                planToolBar.visible = true
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
            anchors.fill:       parent
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
    }
}

