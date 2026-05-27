import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Scrollable list of all swarm vehicles
Rectangle {
    id: root

    color: qgcPal.panel
    radius: 4
    border.width: 1
    border.color: qgcPal.mapMission

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 2

        // Header
        RowLayout {
            spacing: 4

            Label {
                text: "Vehicle List"
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.9
                    bold: true
                }
                color: qgcPal.windowText
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "%1 vehicles".arg(SwarmManager.totalVehicles)
                font {
                    pixelSize: ScreenTools.defaultFontPixelHeight * 0.7
                }
                color: qgcPal.windowText
            }
        }

        // List container
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: vehicleListView
                model: SwarmManager.swarmMembers

                spacing: 4

                delegate: Item {
                    width: parent ? parent.width : 200
                    height: SwarmManager.vehicles().count > 0 ? implicitHeight : 0

                    readonly property var vehicleData: modelData

                    SwarmVehicleStatus {
                        id: vehicleStatus
                        width: parent ? parent.width - 8 : 192
                        height: implicitHeight

                        vehicleId: vehicleData ? vehicleData.id : 0
                        vehicleName: vehicleData ? vehicleData.name : ""
                        isLeader: vehicleData ? vehicleData.isLeader : false
                        batteryPercent: vehicleData ? vehicleData.batteryPercent : 0
                        signalStrength: vehicleData ? vehicleData.signalStrength : 0
                        isArmed: vehicleData ? vehicleData.armed : false
                        isFlying: vehicleData ? vehicleData.flying : false

                        readonly property color statusReady: "#4CAF50"
                        readonly property color statusInMission: "#2196F3"
                        readonly property color statusWarning: "#FF9800"
                        readonly property color statusError: "#F44336"
                        readonly property color statusDisconnected: "#9E9E9E"

                        statusColor: {
                            var status = vehicleData ? vehicleData.status : 0
                            switch (status) {
                                case 3: return statusInMission     // InMission
                                case 4: return statusWarning      // ReturningHome
                                case 5: return statusError        // Emergency
                                case 6: return statusDisconnected // Landed
                                default: return vehicleData && vehicleData.armed ? statusReady : statusDisconnected
                            }
                        }
                    }
                }

                // Empty state
                Label {
                    anchors.centerIn: parent
                    visible: parent.count === 0
                    text: "No vehicles connected"
                    font {
                        pixelSize: ScreenTools.defaultFontPixelHeight * 0.9
                    }
                    color: qgcPal.windowText
                    opacity: 0.6
                }
            }
        }

        // Bulk actions footer
        RowLayout {
            spacing: 4

            QGCButton {
                text: "Select All"
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                Layout.fillWidth: true
                onClicked: {
                    // Select all vehicles logic
                }
            }

            QGCButton {
                text: "Deselect All"
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                Layout.fillWidth: true
                onClicked: {
                    SwarmManager.deselectAllVehicles()
                }
            }
        }
    }
}