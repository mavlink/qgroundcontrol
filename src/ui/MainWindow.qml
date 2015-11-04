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

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtPositioning    5.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.ScreenTools   1.0

/// Qml for MainWindow
Item {
    id: mainWindow

    readonly property string _planViewSource:   "MissionEditor.qml"
    readonly property string _setupViewSource:  "SetupView.qml"

    property real avaiableHeight: height - toolBar.height

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

    // Detect tablet position
    property var tabletPosition: QtPositioning.coordinate(37.803784, -122.462276)
    PositionSource {
        id:             positionSource
        updateInterval: 1000
        active:         ScreenTools.isMobile

        onPositionChanged: {
            tabletPosition = positionSource.position.coordinate
            flightView.latitude = tabletPosition.latitude
            flightView.longitude = tabletPosition.longitude
            positionSource.active = false
        }
    }

    MainToolBar {
        id:                 toolBar
        height:             ScreenTools.isMobile ? (ScreenTools.isTinyScreen ? (mainWindow.width * 0.0666) : (mainWindow.width * 0.0444)) : ScreenTools.defaultFontPixelSize * 4
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
        mainWindow:         mainWindow
        isBackgroundDark:   flightView.isBackgroundDark
        z:                  QGroundControl.zOrderTopMost
    }

    FlightDisplayView {
        id:             flightView
        anchors.fill:   parent
        avaiableHeight: mainWindow.avaiableHeight
        visible:        true
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
