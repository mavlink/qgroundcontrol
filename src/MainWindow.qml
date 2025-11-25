/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtLocation
import QtPositioning

import QGroundControl.FlightMap

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1024
    height: 768
    title: "QGC Map Viewer"

    // Simple toolbar
    header: ToolBar {
        height: 40

        Row {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 10

            Label {
                text: "QGC Map Viewer"
                font.pixelSize: 16
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }

            Button {
                text: "Zoom In"
                anchors.verticalCenter: parent.verticalCenter
                onClicked: map.zoomLevel = Math.min(map.maximumZoomLevel, map.zoomLevel + 1)
            }

            Button {
                text: "Zoom Out"
                anchors.verticalCenter: parent.verticalCenter
                onClicked: map.zoomLevel = Math.max(map.minimumZoomLevel, map.zoomLevel - 1)
            }

            Label {
                text: "Zoom: " + map.zoomLevel.toFixed(1)
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // Main map
    FlightMap {
        id: map
        anchors.fill: parent

        center: QtPositioning.coordinate(37.7749, -122.4194) // San Francisco
        zoomLevel: 12

        // Enable mouse/touch interaction
        gesture.enabled: true
        gesture.acceptedGestures: MapGestureArea.PinchGesture | MapGestureArea.PanGesture | MapGestureArea.FlickGesture

        // Map scale indicator
        MapScale {
            id: mapScale
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: 10
            mapSource: map
        }
    }
}
