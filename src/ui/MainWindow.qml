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

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.ScreenTools   1.0

/// Qml for MainWindow
Item {
    id: mainWindow

    readonly property string _planViewSource:   "MissionEditor.qml"
    readonly property string _setupViewSource:  "SetupView.qml"

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }

    property real tbHeight:         ScreenTools.isMobile ? (ScreenTools.isTinyScreen ? (mainWindow.width * 0.0666) : (mainWindow.width * 0.0444)) : ScreenTools.defaultFontPixelSize * 4
    property int  tbCellHeight:     tbHeight * 0.75
    property real tbSpacing:        ScreenTools.isMobile ? width * 0.00824 : 9.54
    property real tbButtonWidth:    tbCellHeight * 1.3
    property real avaiableHeight:   height - tbHeight
    property real menuButtonWidth:  (tbButtonWidth * 2) + (tbSpacing * 4) + 1

    Connections {

        target: controller

        onShowFlyView: {
            flightView.visible          = true
            setupViewLoader.visible     = false
            planViewLoader.visible      = false
        }

        onShowPlanView: {
            if (planViewLoader.source   != _planViewSource) {
                planViewLoader.source   = _planViewSource
            }
            flightView.visible          = false
            setupViewLoader.visible     = false
            planViewLoader.visible      = true
        }

        onShowSetupView: {
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
    property var tabletPosition: QtPositioning.coordinate(37.803784, -122.462276)
    PositionSource {
        id:             positionSource
        updateInterval: 1000
        active:         true // ScreenTools.isMobile

        onPositionChanged: {
            tabletPosition          = positionSource.position.coordinate
            flightView.latitude     = tabletPosition.latitude
            flightView.longitude    = tabletPosition.longitude
            positionSource.active   = false
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
        avaiableHeight:     mainWindow.avaiableHeight
        visible:            true
    }

    Loader {
        id:                 planViewLoader
        anchors.fill:       parent
        visible:            false
        property var tabletPosition:    mainWindow.tabletPosition
    }

    Loader {
        id:                 setupViewLoader
        anchors.fill:       parent
        visible:            false
        property var tabletPosition:    mainWindow.tabletPosition
    }

}
