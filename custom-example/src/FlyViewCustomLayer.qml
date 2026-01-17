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

    property var parentToolInsets                       // These insets tell you what screen real estate is available for positioning the controls in your overlay
    property var totalToolInsets:   _totalToolInsets    // The insets updated for the custom overlay additions
    property var mapControl

    // ---------------- VIDEO IS HANDLED BY FLY VIEW ----------------
    // This overlay does NOT create a new video surface

    // ---------------- DASHBOARD PANEL ----------------
    Rectangle {
        id: dashboard
        width: parent.width * 0.35
        height: parent.height * 0.5
        anchors.top: parent.top
        anchors.right: parent.right
        radius: 6
        color: "#153247cc"
        border.width: 2
        border.color: "#a50605"

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
                    onClicked: sendCommand(31100) // CMD_START_TRACKING
                }

                QGCButton {
                    text: "Stop Tracking"
                    onClicked: sendCommand(31101) // CMD_STOP_TRACKING
                }
            }

            RowLayout {
                spacing: 6

                QGCButton {
                    text: "Clear Lock"
                    onClicked: sendCommand(31105) // CMD_CLEAR_LOCK
                }

                QGCButton {
                    text: "Set Depth"
                    onClicked: {
                        // Example: set depth min 5m, max 20m
                        sendCommand(31104, 5, 20)
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
                    onClicked: sendCommand(31102, targetIdInput.text, 0,0,0,0,0)
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
                    onClicked: sendCommand(31103, pixelX.text, pixelY.text,0,0,0,0)
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

    // ---------------- MAVLINK COMMAND FUNCTION ----------------
    function sendCommand(commandId, param1=0, param2=0, param3=0, param4=0, param5=0, param6=0, param7=0) {
        if (!QGroundControl.multiVehicleManager.activeVehicle) {
            console.log("No active vehicle")
            return
        }

        var vehicle = QGroundControl.multiVehicleManager.activeVehicle

        vehicle.sendMavCommandLong(
            255,          // targetSystem (255 for Companion / broadcast)
            190,          // targetComponent (Onboard Computer)
            commandId,
            false,        // confirmation
            param1,
            param2,
            param3,
            param4,
            param5,
            param6,
            param7
        )

        console.log("Sent command:", commandId, param1, param2, param3)
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
