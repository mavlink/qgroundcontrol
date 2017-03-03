/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.7
import QtQuick.Controls 2.1
import QtQuick.Layouts  1.2
import QtPositioning    5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

ColumnLayout {
    id:         root
    spacing:    ScreenTools.defaultFontPixelWidth * 0.5

    property var    map
    property var    fitFunctions
    property bool   showMission:          true
    property bool   showAllItems:         true
    property bool   showFollowVehicle:    false
    property bool   followVehicle:        false

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    QGCLabel { text: qsTr("Center map on:") }

    QGCButton {
        text:               qsTr("Mission")
        Layout.fillWidth:   true
        visible:            showMission
        enabled:            !followVehicleCheckBox.checked

        onClicked: {
            dropPanel.hide()
            fitFunctions.fitMapViewportToMissionItems()
        }
    }

    QGCButton {
        text:               qsTr("All items")
        Layout.fillWidth:   true
        visible:            showAllItems
        enabled:            !followVehicleCheckBox.checked

        onClicked: {
            dropPanel.hide()
            fitFunctions.fitMapViewportToAllItems()
        }
    }

    QGCButton {
        text:               qsTr("Home")
        Layout.fillWidth:   true
        enabled:            !followVehicleCheckBox.checked

        onClicked: {
            dropPanel.hide()
            map.center = fitFunctions.fitHomePosition()
        }
    }

    QGCButton {
        text:               qsTr("Current Location")
        Layout.fillWidth:   true
        enabled:            mainWindow.gcsPosition.isValid && !followVehicleCheckBox.checked

        onClicked: {
            dropPanel.hide()
            map.center = mainWindow.gcsPosition
        }
    }

    QGCButton {
        text:               qsTr("Vehicle")
        Layout.fillWidth:   true
        enabled:            _activeVehicle && _activeVehicle.latitude != 0 && _activeVehicle.longitude != 0 && !followVehicleCheckBox.checked

        onClicked: {
            dropPanel.hide()
            map.center = activeVehicle.coordinate
        }
    }

    QGCCheckBox {
        id:         followVehicleCheckBox
        text:       qsTr("Follow Vehicle")
        checked:    followVehicle
        visible:    showFollowVehicle

        onClicked:  {
            dropPanel.hide()
            root.followVehicle = checked
        }
    }
} // Column
