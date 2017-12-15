/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.3
import QtQuick.Window       2.2
import QtQuick.Controls     1.2
import QtQuick.Dialogs      1.2
import QtPositioning        5.3
import QtGraphicalEffects   1.0

import QGroundControl                       1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controls              1.0
import QGroundControl.FlightDisplay         1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0

import TyphoonHQuickInterface               1.0

/// Native QML top level window
Window {
    id:             _rootWindow
    width:          1280
    height:         720
    minimumWidth:   800
    minimumHeight:  400
    visible:        true

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var    currentPopUp:           null
    property real   currentCenterX:         0
    property var    activeVehicle:          QGroundControl.multiVehicleManager.activeVehicle
    property bool   communicationLost:      activeVehicle ? activeVehicle.connectionLost : false
    property var    planMap:                planViewLoader.item ? planViewLoader.item.planMap : null
    property var    planMasterController:   planViewLoader.item ? planViewLoader.item.planMasterController : null
    property var    mapSettings:            QGroundControl.settingsManager.flightMapSettings
    property string mapType:                mapSettings.mapProvider.enumStringValue + " " + mapSettings.mapType.enumStringValue
    property real   minZoom:                2
    property real   maxZoom:                19
    property int    clientCount:            TyphoonHQuickInterface.clientList.length
    property bool   toolbarEnabled:         true

    function showSetupView() {
    }

    function showMessage(message) {
        console.log('Root: ' + message)
    }

    function handleMapChanges() {
        if(planMap) {
            var xl = 0
            var yl = 0
            var xr = planMap.width.toFixed(0) - 1  // Must be within boundaries of visible map
            var yr = planMap.height.toFixed(0) - 1 // Must be within boundaries of visible map
            var c0 = planMap.toCoordinate(Qt.point(xl, yl), false /* clipToViewPort */)
            var c1 = planMap.toCoordinate(Qt.point(xr, yr), false /* clipToViewPort */)
            QGroundControl.mapEngineManager.updateForCurrentView(c0.longitude, c0.latitude, c1.longitude, c1.latitude, minZoom, maxZoom, mapType)
            console.log('Tile count: ' + QGroundControl.mapEngineManager.tileCountStr)
            console.log('Tile size:  ' + QGroundControl.mapEngineManager.tileSizeStr)
        }
    }

    function updateMap() {
        for (var i = 0; i < planMap.supportedMapTypes.length; i++) {
            if (mapType === planMap.supportedMapTypes[i].name) {
                planMap.activeMapType = planMap.supportedMapTypes[i]
                handleMapChanges()
                return
            }
        }
    }

    Component.onCompleted: {
        QGroundControl.mapEngineManager.loadTileSets()
        updateMap()
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
        target: planMap
        onCenterChanged:    handleMapChanges()
        onZoomLevelChanged: handleMapChanges()
        onWidthChanged:     handleMapChanges()
        onHeightChanged:    handleMapChanges()
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
            toolbarEnabled = false
        }

        function enableToolbar() {
            toolbarEnabled = true
        }

        function showSettingsView() {
            if(toolbarEnabled) {
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
        }

        function showPlanView() {
            if(toolbarEnabled) {
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
                    enabled:            toolbarEnabled
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
            visible:                    (planViewLoader.visible && planMasterController && planMasterController.dirty) // clientCount &&  || activeVehicle
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
                    visible:            activeVehicle
                    source:             "/typhoonh/YGPSIndicator.qml"
                }
                Loader {
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
                    visible:            activeVehicle
                    source:             "/typhoonh/BatteryIndicator.qml"
                }
                //-- This is enabled (visisble) if we have received a broadcast
                //   from an ST16 and we are not connected to a vehicle.
                QGCButton {
                    text:               qsTr("Upload to ST16")
                    visible:            !activeVehicle
                    primary:            true
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: {
                        exportToST16.visible = true
                        mainWindow.disableToolbar()
                    }
                }
                Rectangle {
                    height:             1
                    width:              1
                    visible:            !activeVehicle
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
        //-------------------------------------------------------------------------
        //-- Upload to ST16
        Item {
            id:             exportToST16
            visible:        false
            anchors.fill:   parent
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Rectangle {
                id:             exportST16Shadow
                anchors.fill:   exportST16Rect
                radius:         exportST16Rect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       exportST16Shadow
                visible:            exportST16Rect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             exportST16Shadow
            }
            Rectangle {
                id:             exportST16Rect
                width:          ScreenTools.defaultFontPixelWidth * 100
                height:         copyCol.height * 1.25
                radius:         ScreenTools.defaultFontPixelWidth
                color:          qgcPal.alertBackground
                border.color:   qgcPal.alertBorder
                border.width:   2
                anchors.centerIn: parent
                Column {
                    id:                 copyCol
                    width:              exportST16Rect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 2
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("Upload Mission")
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           clientCount ? TyphoonHQuickInterface.clientList[0] : ""
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Rectangle {
                        color:          qgcPal.window
                        width:          exportUTMCheck.width  + (ScreenTools.defaultFontPixelWidth * 4)
                        height:         exportUTMCheck.height + ScreenTools.defaultFontPixelHeight
                        radius:         4
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCCheckBox {
                            id:                 exportUTMCheck
                            text:               qsTr("Include Map Tiles")
                            checked:            false
                            enabled:            !TyphoonHQuickInterface.copyingFiles
                            anchors.centerIn:   parent
                        }
                    }
                    ProgressBar {
                        width:          parent.width * 0.75
                        orientation:    Qt.Horizontal
                        minimumValue:   0
                        maximumValue:   100
                        value:          TyphoonHQuickInterface.updateProgress
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           TyphoonHQuickInterface.copyMessage
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCButton {
                            text:           !TyphoonHQuickInterface.copyingFiles ? qsTr("Upload") : qsTr("Cancel")
                            width:          ScreenTools.defaultFontPixelWidth  * 16
                            height:         ScreenTools.defaultFontPixelHeight * 2
                            enabled:        !TyphoonHQuickInterface.copyingDone
                            onClicked: {
                                console.log('Upload')
                                if(planMasterController) {
                                    TyphoonHQuickInterface.uploadMission(planMasterController)
                                }
                            }
                        }
                        QGCButton {
                            text:           qsTr("Close")
                            width:          ScreenTools.defaultFontPixelWidth  * 16
                            enabled:        !TyphoonHQuickInterface.copyingFiles
                            height:         ScreenTools.defaultFontPixelHeight * 2
                            onClicked: {
                                mainWindow.enableToolbar()
                                exportToST16.visible = false
                            }
                        }
                    }
                }
            }
        }
    }
}

