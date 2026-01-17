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
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay

Item {
    id: flyDashboard
    anchors.fill: parent

    property var parentToolInsets                       // These insets tell you what screen real estate are available for positioning the controls in your overlay
    property var totalToolInsets:   _totalToolInsets    // The insets updated for the custom overlay additions
    property var mapControl

    // ---------------- MAVLINK COMMAND FUNCTION ----------------
    function sendCommand(commandId, param1=0, param2=0, param3=0, param4=0, param5=0, param6=0, param7=0) {
        if (!QGroundControl.multiVehicleManager.activeVehicle) {
            console.log("No active vehicle")
            return
        }

        var vehicle = QGroundControl.multiVehicleManager.activeVehicle

        // Send to vehicle - QGC will route to connected MAVLink systems
        // Your Jetson should be connected as a MAVLink companion computer
        vehicle.sendCommand(
            commandId,     // command ID (31100-31105)
            false,         // showError (false = no error display)
            param1,         // parameter 1 (track_id or pixel_x)
            param2,         // parameter 2 (pixel_y or depth_min)
            param3,         // parameter 3 (depth_max)
            param4,         // parameter 4 (unused)
            param5,         // parameter 5 (unused)
            param6,         // parameter 6 (unused)
            param7          // parameter 7 (unused)
        )

        console.log("Sent MAVLink command:", commandId, "params:", param1, param2, param3)
    }

    // ---------------- VIDEO IS HANDLED BY FLY VIEW ----------------
    // This overlay does NOT create a new video surface

    // ---------------- DASHBOARD PANEL ----------------
    Rectangle {
        id: dashboard
        width: parent.width * 0.175  // Half of previous width (was 0.35)
        height: parent.height * 0.5
        anchors.top: parent.top
        anchors.right: parent.right
        radius: 6
        color: "#2c3e5080"  // Standard dark blue with 50% opacity (80 = 50% alpha)
        border.width: 2
        border.color: "#3498db"  // Lighter blue border

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            QGCLabel {
                text: "Al-Bayrak Object Tracking"
                font.bold: true
                font.pointSize: ScreenTools.mediumFontPointSize
                color: "white"
            }

            // ---------------- TRACKING CONTROLS ----------------
            RowLayout {
                spacing: 6

                QGCButton {
                    text: "Start Tracking"
                    onClicked: {
                        // Step 1: Enable tracking mode first
                        sendCommand(31100) // START_TRACKING
                    }
                }

                QGCButton {
                    text: "Stop Tracking"
                    onClicked: {
                        // Step 3: Disable tracking mode
                        sendCommand(31101) // STOP_TRACKING
                    }
                }
            }

            RowLayout {
                spacing: 6

                QGCButton {
                    text: "Clear Lock"
                    onClicked: {
                        // Clear current lock without stopping tracking mode
                        sendCommand(31105) // CLEAR_LOCK
                    }
                }

                QGCButton {
                    text: "Set Depth"
                    onClicked: {
                        // Set depth filtering range: min 5m, max 20m
                        sendCommand(31104, 5, 20) // SET_DEPTH_RANGE
                    }
                }
            }

            // ---------------- TARGET SELECTION ----------------
            QGCLabel {
                text: "Select Target by ID:"
                color: "white"
            }

            RowLayout {
                spacing: 6
                QGCTextField {
                    id: targetIdInput
                    placeholderText: "Track ID"
                    width: 80
                }
                QGCButton {
                    text: "Select"
                    onClicked: {
                        // Step 2: Select target by track ID (from bounding box #ID)
                        var trackId = parseInt(targetIdInput.text) || 0
                        sendCommand(31102, trackId, 0) // SELECT_TARGET_ID
                    }
                }
            }

            QGCLabel {
                text: "Select Target by Pixel:"
                color: "white"
            }

            RowLayout {
                spacing: 6
                QGCTextField {
                    id: pixelX
                    placeholderText: "U"
                    width: 50
                }
                QGCTextField {
                    id: pixelY
                    placeholderText: "V"
                    width: 50
                }
                QGCButton {
                    text: "Select"
                    onClicked: {
                        // Alternative: Select target by pixel coordinates
                        var x = parseFloat(pixelX.text) || 0
                        var y = parseFloat(pixelY.text) || 0
                        sendCommand(31103, x, y) // SELECT_TARGET_PIXEL
                    }
                }
            }

            // ---------------- TELEMETRY DISPLAY ----------------
            QGCLabel {
                id: telemetryLabel
                text: "Feedback: --"
                color: "#28a745"
                font.pointSize: ScreenTools.smallFontPointSize
            }
        }
    }

    // ---------------- TOOL INSETS MANAGEMENT ----------------
    QGCToolInsets {
        id:                     _totalToolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   dashboard.x
        rightEdgeBottomInset:   parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }

    // ---------------- NAMED_VALUE_INT TELEMETRY ----------------
    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle
        ignoreUnknownSignals: true

        function onNamedValueIntReceived(vehicleId, componentId, name, value) {
            if (name.startsWith("TRK")) {
                telemetryLabel.text = "Feedback: " + name + " = " + value
            }
        }
    }
}
