/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control - Enterprise Drone Fleet Management Module
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

Rectangle {
    id:             root
    color:          qgcPal.window
    anchors.fill:   parent

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    // Mock Fleet Storage Model (Designed for SQLite / Cloud API binding)
    ListModel {
        id: fleetModel

        ListElement {
            name:               "Volador Apex Hexa #1"
            modelName:          "Volador Apex-6"
            serialNumber:       "VOL-HEX-2026-001"
            vehicleType:        "Hexarotor"
            firmware:           "PX4 Autopilot v1.14"
            status:             "Operational"       // Operational, In Mission, Maintenance, Offline
            regDate:            "2026-01-15"
            lastFlight:         "2026-07-27 14:30"
            flightHours:        "124.5 hrs"
            totalMissions:      84
            batteryCycles:      42
            motorInterval:      "875 / 1000 hrs"
            propellerStatus:    "Good (Inspected)"
            nextInspection:     "2026-08-15"
            notes:              "Primary surveying aircraft for heavy payload camera mapping."
        }

        ListElement {
            name:               "Volador VTOL Scout #4"
            modelName:          "Volador Ranger-V"
            serialNumber:       "VOL-VTL-2026-004"
            vehicleType:        "VTOL Fixed-Wing"
            firmware:           "PX4 Autopilot v1.14"
            status:             "In Mission"
            regDate:            "2026-02-01"
            lastFlight:         "2026-07-27 16:45"
            flightHours:        "310.2 hrs"
            totalMissions:      142
            batteryCycles:      98
            motorInterval:      "690 / 1000 hrs"
            propellerStatus:    "Replaced 2026-06-10"
            nextInspection:     "2026-08-01"
            notes:              "Long-range linear corridor mapping and pipeline inspection."
        }

        ListElement {
            name:               "Volador Agrispray #9"
            modelName:          "Volador Agri-10"
            serialNumber:       "VOL-AGR-2026-009"
            vehicleType:        "Multirotor Sprayer"
            firmware:           "ArduPilot Copter 4.5"
            status:             "Maintenance"
            regDate:            "2026-03-10"
            lastFlight:         "2026-07-25 11:20"
            flightHours:        "88.0 hrs"
            totalMissions:      48
            batteryCycles:      115
            motorInterval:      "912 / 1000 hrs (Check)"
            propellerStatus:    "Replacement Due"
            nextInspection:     "2026-07-28 (OVERDUE)"
            notes:              "Precision agriculture spraying. Motor #3 bearing noise noted."
        }

        ListElement {
            name:               "Volador Inspector #12"
            modelName:          "Volador Spot-X"
            serialNumber:       "VOL-INS-2026-012"
            vehicleType:        "Quadrotor"
            firmware:           "PX4 Autopilot v1.14"
            status:             "Offline"
            regDate:            "2026-04-20"
            lastFlight:         "2026-07-20 09:15"
            flightHours:        "204.1 hrs"
            totalMissions:      96
            batteryCycles:      64
            motorInterval:      "796 / 1000 hrs"
            propellerStatus:    "Good"
            nextInspection:     "2026-08-20"
            notes:              "Confined space and thermal solar panel inspection drone."
        }
    }

    property int selectedIndex: 0
    property string searchText: ""
    property string statusFilter: "All"

    QGCFlickable {
        anchors.margins:    _margins
        anchors.fill:       parent
        contentWidth:       mainColumn.width
        contentHeight:      mainColumn.height
        clip:               true

        ColumnLayout {
            id:         mainColumn
            spacing:    _margins * 1.2
            width:      Math.max(700, parent.width - (_margins * 2))

            // 1. Dashboard Header Summary Cards
            RowLayout {
                spacing: _margins
                Layout.fillWidth: true

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 90
                    color: qgcPal.windowShade
                    radius: 6

                    ColumnLayout {
                        anchors.centerIn: parent
                        QGCLabel { text: qsTr("TOTAL FLEET"); font.pointSize: ScreenTools.smallFontPointSize; color: qgcPal.text }
                        QGCLabel { text: "4 Drones"; font.pointSize: ScreenTools.largeFontPointSize; font.bold: true; color: qgcPal.colorOrange }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 90
                    color: qgcPal.windowShade
                    radius: 6

                    ColumnLayout {
                        anchors.centerIn: parent
                        QGCLabel { text: qsTr("ACTIVE / FLYING"); font.pointSize: ScreenTools.smallFontPointSize; color: qgcPal.text }
                        QGCLabel { text: "2 Active"; font.pointSize: ScreenTools.largeFontPointSize; font.bold: true; color: "#00E04B" }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 90
                    color: qgcPal.windowShade
                    radius: 6

                    ColumnLayout {
                        anchors.centerIn: parent
                        QGCLabel { text: qsTr("MAINTENANCE DUE"); font.pointSize: ScreenTools.smallFontPointSize; color: qgcPal.text }
                        QGCLabel { text: "1 Drone"; font.pointSize: ScreenTools.largeFontPointSize; font.bold: true; color: "#FF6A00" }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 90
                    color: qgcPal.windowShade
                    radius: 6

                    ColumnLayout {
                        anchors.centerIn: parent
                        QGCLabel { text: qsTr("FLEET HEALTH"); font.pointSize: ScreenTools.smallFontPointSize; color: qgcPal.text }
                        QGCLabel { text: "95% Good"; font.pointSize: ScreenTools.largeFontPointSize; font.bold: true; color: "#00E04B" }
                    }
                }
            }

            // 2. Filter & Controls Toolbar
            RowLayout {
                spacing: _margins
                Layout.fillWidth: true

                QGCLabel { text: qsTr("Filter Status:"); font.bold: true }

                QGCButton {
                    text:       "All (4)"
                    primary:    statusFilter === "All"
                    onClicked:  statusFilter = "All"
                }
                QGCButton {
                    text:       "Operational"
                    primary:    statusFilter === "Operational"
                    onClicked:  statusFilter = "Operational"
                }
                QGCButton {
                    text:       "In Mission"
                    primary:    statusFilter === "In Mission"
                    onClicked:  statusFilter = "In Mission"
                }
                QGCButton {
                    text:       "Maintenance"
                    primary:    statusFilter === "Maintenance"
                    onClicked:  statusFilter = "Maintenance"
                }
                QGCButton {
                    text:       "Offline"
                    primary:    statusFilter === "Offline"
                    onClicked:  statusFilter = "Offline"
                }
            }

            // 3. Main Split View (Drone List & Detail Inspection)
            RowLayout {
                spacing: _margins
                Layout.fillWidth: true

                // Drone List Grid
                Rectangle {
                    Layout.preferredWidth:  parent.width * 0.48
                    implicitHeight:         420
                    color:                  qgcPal.windowShadeDark
                    radius:                 6

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: _margins

                        QGCLabel {
                            text:           qsTr("REGISTERED DRONE FLEET")
                            font.bold:      true
                            color:          qgcPal.colorOrange
                        }

                        ListView {
                            id:             droneListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model:          fleetModel
                            spacing:        8
                            clip:           true

                            delegate: Rectangle {
                                width:          droneListView.width
                                height:         65
                                color:          index === root.selectedIndex ? qgcPal.windowShade : qgcPal.window
                                radius:         4
                                border.color:   index === root.selectedIndex ? qgcPal.colorOrange : qgcPal.groupBorder
                                border.width:   1

                                property bool matchesFilter: statusFilter === "All" || model.status === statusFilter

                                visible: matchesFilter
                                implicitHeight: matchesFilter ? 65 : 0

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked:    root.selectedIndex = index
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 12

                                    // Status Indicator Pill
                                    Rectangle {
                                        width: 12
                                        height: 12
                                        radius: 6
                                        color: {
                                            if (model.status === "Operational") return "#00E04B"
                                            if (model.status === "In Mission") return "#FF6A00"
                                            if (model.status === "Maintenance") return "#F32836"
                                            return "#8B949E"
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        QGCLabel { text: model.name; font.bold: true }
                                        QGCLabel { text: model.serialNumber + " | " + model.vehicleType; font.pointSize: ScreenTools.smallFontPointSize; color: qgcPal.text }
                                    }

                                    ColumnLayout {
                                        QGCLabel { text: model.status; font.bold: true; color: qgcPal.colorOrange }
                                        QGCLabel { text: model.flightHours; font.pointSize: ScreenTools.smallFontPointSize }
                                    }
                                }
                            }
                        }
                    }
                }

                // Selected Drone Inspection Details Card
                Rectangle {
                    Layout.fillWidth:       true
                    implicitHeight:         420
                    color:                  qgcPal.windowShadeDark
                    radius:                 6

                    ColumnLayout {
                        anchors.fill:       parent
                        anchors.margins:    _margins
                        spacing:            8

                        property var currentDrone: fleetModel.get(root.selectedIndex)

                        QGCLabel {
                            text:           parent.currentDrone.name + " (" + parent.currentDrone.serialNumber + ")"
                            font.pointSize: ScreenTools.mediumFontPointSize
                            font.bold:      true
                            color:          qgcPal.colorOrange
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: qgcPal.groupBorder
                        }

                        GridLayout {
                            columns: 2
                            rowSpacing: 8
                            columnSpacing: 20
                            Layout.fillWidth: true

                            QGCLabel { text: qsTr("Model:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.modelName; font.bold: true }

                            QGCLabel { text: qsTr("Vehicle Type:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.vehicleType; font.bold: true }

                            QGCLabel { text: qsTr("Autopilot Firmware:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.firmware; font.bold: true }

                            QGCLabel { text: qsTr("Current Status:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.status; font.bold: true; color: qgcPal.colorOrange }

                            QGCLabel { text: qsTr("Total Flight Hours:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.flightHours }

                            QGCLabel { text: qsTr("Total Missions:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.totalMissions + " Completed" }

                            QGCLabel { text: qsTr("Battery Cycles:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.batteryCycles + " Cycles" }

                            QGCLabel { text: qsTr("Motor Maintenance:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.motorInterval }

                            QGCLabel { text: qsTr("Propeller Status:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.propellerStatus }

                            QGCLabel { text: qsTr("Next Inspection:"); color: qgcPal.text }
                            QGCLabel { text: parent.parent.currentDrone.nextInspection; font.bold: true; color: "#00E04B" }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: qgcPal.groupBorder
                        }

                        QGCLabel { text: qsTr("Maintenance Notes:"); font.bold: true; color: qgcPal.text }
                        QGCLabel {
                            text: parent.currentDrone.notes
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            color: qgcPal.text
                        }
                    }
                }
            }
        }
    }
}
