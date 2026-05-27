import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Alert system for swarm warnings and notifications
Rectangle {
    id: root

    color: qgcPal.panel
    radius: 4
    border.width: 1
    border.color: qgcPal.mapMission

    ListModel {
        id: alertModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // Header
        RowLayout {
            spacing: 4

            Label {
                text: "Alerts"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.9
                    bold: true
                }
                color: qgcPal.windowText
            }

            Item { Layout.fillWidth: true }

            // Alert count badge
            Rectangle {
                width: alertBadge.width + 8
                height: ScreenTools.defaultFontPixelHeight
                color: alertCount > 0 ? "#F44336" : "#4CAF50"
                radius: height / 2

                Label {
                    id: alertBadge
                    anchors.centerIn: parent
                    text: alertCount
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                        bold: true
                    }
                    color: "white"
                }
            }
        }

        // Alert list
        ListView {
            id: alertList
            Layout.fillWidth: true
            Layout.fillHeight: true

            model: alertModel

            spacing: 4

            delegate: Rectangle {
                width: parent ? parent.width : 200
                height: alertItemHeight
                color: alertBackgroundColor
                radius: 4

                readonly property real alertItemHeight: ScreenTools.defaultFontPixelHeight * 2.5

                readonly property color alertBackgroundColor: {
                    if (alertLevel === "critical") return "#FFEBEE"
                    if (alertLevel === "warning") return "#FFF3E0"
                    return "#E3F2FD"
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 4

                    RowLayout {
                        spacing: 4

                        Label {
                            text: alertIcon
                            font {
                                pixelSize: ScreenTools.defaultFontPixelHeight
                            }
                        }

                        ColumnLayout {
                            spacing: 0

                            Label {
                                text: alertTitle
                                font {
                                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.75
                                    bold: true
                                }
                                color: alertTextColor
                            }

                            Label {
                                text: alertTime
                                font {
                                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.5
                                }
                                color: qgcPal.windowText
                                opacity: 0.7
                            }
                        }

                        Item { Layout.fillWidth: true }

                        QGCButton {
                            text: "Dismiss"
                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.2
                            onClicked: {
                                alertModel.remove(index)
                            }
                        }
                    }

                    Label {
                        text: alertDescription
                        font {
                            pixelSize: ScreenTools.defaultFontPixelHeight * 0.6
                        }
                        color: qgcPal.windowText
                        wrapMode: Text.WordWrap
                        elide: Text.ElideRight
                    }
                }

                property string alertIcon: icon || "⚠"
                property string alertTitle: title || "Alert"
                property string alertDescription: description || ""
                property string alertLevel: level || "info"
                property string alertTime: timestamp || "Now"
                property color alertTextColor: {
                    if (alertLevel === "critical") return "#D32F2F"
                    if (alertLevel === "warning") return "#F57C00"
                    return "#1976D2"
                }
            }

            // Empty state
            Label {
                anchors.centerIn: parent
                visible: alertModel.count === 0
                text: "No active alerts"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
                }
                color: qgcPal.windowText
                opacity: 0.6
            }
        }

        // Clear all button
        QGCButton {
            text: "Clear All Alerts"
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
            enabled: alertModel.count > 0
            onClicked: {
                alertModel.clear()
            }
        }
    }

    property int alertCount: alertModel.count

    // Alert management functions
    function addAlert(title, description, level, icon) {
        alertModel.insert(0, {
            title: title,
            description: description,
            level: level || "info",
            icon: icon || "ℹ",
            timestamp: new Date().toLocaleTimeString()
        })
    }

    function addCollisionWarning(vehicleId1, vehicleId2) {
        addAlert(
            "Collision Risk",
            "Vehicles UAV-%1 and UAV-%2 are too close!".arg(vehicleId1).arg(vehicleId2),
            "critical",
            "⚠"
        )
    }

    function addBatteryWarning(vehicleId, percent) {
        addAlert(
            "Low Battery",
            "UAV-%1 battery at %2%".arg(vehicleId).arg(percent.toFixed(0)),
            percent < 15 ? "critical" : "warning",
            "🔋"
        )
    }

    function addConnectionLost(vehicleId) {
        addAlert(
            "Connection Lost",
            "UAV-%1 has lost connection".arg(vehicleId),
            "critical",
            "📡"
        )
    }

    function addEmergencyAlert() {
        addAlert(
            "EMERGENCY STOP",
            "Emergency stop is active. All vehicles have been commanded to stop.",
            "critical",
            "🚨"
        )
    }

    // Connection to SwarmManager signals
    Connections {
        target: SwarmManager

        function onCollisionWarning(vehicleId1, vehicleId2) {
            root.addCollisionWarning(vehicleId1, vehicleId2)
        }

        function onVehicleStatusChanged(vehicleId, status) {
            // Handle status change alerts
            if (status === 5) { // Emergency status
                root.addAlert("Vehicle Emergency", "UAV-%1 is in emergency state".arg(vehicleId), "critical", "🚨")
            }
        }

        function onEmergencyStopActiveChanged(active) {
            if (active) {
                root.addEmergencyAlert()
            }
        }
    }
}