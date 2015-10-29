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

    property var _toolbar: toolbarLoader.item

    readonly property string _planViewSource:   "MissionEditor.qml"
    readonly property string _setupViewSource:  "SetupView.qml"

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

        onShowToolbarMessage: _toolbar.showToolbarMessage(message)

        // The following are use for unit testing only

        onShowSetupFirmware:            setupViewLoader.item.showFirmwarePanel()
        onShowSetupParameters:          setupViewLoader.item.showParametersPanel()
        onShowSetupSummary:             setupViewLoader.item.showSummaryPanel()
        onShowSetupVehicleComponent:    setupViewLoader.item.showVehicleComponentPanel(vehicleComponent)
    }

    // We delay load the following control to improve boot time
    Component.onCompleted: {
        toolbarLoader.source = "MainToolBar.qml"
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

    Loader {
        id:                 toolbarLoader
        width:              parent.width
        height:             item ? item.height : 0
        z:                  QGroundControl.zOrderTopMost
    }

    FlightDisplayView {
        id:                 flightView
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        toolbarLoader.bottom
        anchors.bottom:     parent.bottom
        visible:            true
    }

    Loader {
        id:                 planViewLoader
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        toolbarLoader.bottom
        anchors.bottom:     parent.bottom
        visible:            false

        property var tabletPosition:    mainWindow.tabletPosition
    }

    Loader {
        id:                 setupViewLoader
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        toolbarLoader.bottom
        anchors.bottom:     parent.bottom
        visible:            false

        property var tabletPosition:    mainWindow.tabletPosition
    }
}
