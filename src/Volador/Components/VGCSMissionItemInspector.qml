/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Interactive Mission Item Inspector
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: inspectorRoot

    property var planMasterController: null
    property var missionController: planMasterController ? planMasterController.missionController : null
    readonly property var currentItem: (missionController && missionController.currentPlanViewItem) ? missionController.currentPlanViewItem : null

    implicitWidth: 260
    implicitHeight: inspectorCol.implicitHeight + 24
    radius: 4
    color: "#E6151C24" // 90% dark glass
    border.color: "#2C3847"
    border.width: 1

    function adjustAltitude(delta) {
        if (!currentItem) return
        if (currentItem.altitude && currentItem.altitude.value !== undefined) {
            var current = currentItem.altitude.value
            var updated = Math.max(0, current + delta)
            currentItem.altitude.value = updated
        } else if (currentItem.amslEntryAlt !== undefined) {
            currentItem.applyNewAltitude(Math.max(0, currentItem.amslEntryAlt + delta))
        }
    }

    function navigatePrevious() {
        if (!missionController || !missionController.visualItems) return
        var currIdx = missionController.currentPlanViewVIIndex
        if (currIdx > 0) {
            var target = missionController.visualItems.get(currIdx - 1)
            if (target) missionController.setCurrentPlanViewSeqNum(target.sequenceNumber, true)
        }
    }

    function navigateNext() {
        if (!missionController || !missionController.visualItems) return
        var currIdx = missionController.currentPlanViewVIIndex
        if (currIdx < missionController.visualItems.count - 1) {
            var target = missionController.visualItems.get(currIdx + 1)
            if (target) missionController.setCurrentPlanViewSeqNum(target.sequenceNumber, true)
        }
    }

    ColumnLayout {
        id: inspectorCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 8

        // 1. Title Bar & Sequence Step Navigators
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Rectangle {
                width: 6
                height: 6
                radius: 3
                color: currentItem ? "#FF6A00" : "#64748B"
            }

            Text {
                text: "ITEM INSPECTOR"
                font.family: "Inter"
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 0.8
                color: "#F5F7FA"
            }

            Item { Layout.fillWidth: true }

            // Step Navigators
            RowLayout {
                spacing: 2
                visible: !!currentItem

                Rectangle {
                    width: 22
                    height: 20
                    radius: 3
                    color: prevMouse.containsMouse ? "#2C3847" : "#1D2733"
                    border.color: "#2C3847"
                    border.width: 1
                    opacity: (missionController && missionController.currentPlanViewVIIndex > 0) ? 1.0 : 0.3

                    Text { anchors.centerIn: parent; text: "◀"; font.pixelSize: 8; color: "#F5F7FA" }

                    MouseArea {
                        id: prevMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: inspectorRoot.navigatePrevious()
                    }
                    ToolTip.visible: prevMouse.containsMouse
                    ToolTip.text: "Previous item"
                    ToolTip.delay: 300
                }

                Rectangle {
                    height: 20
                    implicitWidth: seqText.implicitWidth + 8
                    radius: 3
                    color: "#FF6A00"

                    Text {
                        id: seqText
                        anchors.centerIn: parent
                        text: currentItem ? (currentItem.sequenceNumber === 0 ? "HOME" : ("#" + currentItem.sequenceNumber)) : "--"
                        font.family: "JetBrains Mono"
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: "#0A0F14"
                    }
                }

                Rectangle {
                    width: 22
                    height: 20
                    radius: 3
                    color: nextMouse.containsMouse ? "#2C3847" : "#1D2733"
                    border.color: "#2C3847"
                    border.width: 1
                    opacity: (missionController && missionController.visualItems && missionController.currentPlanViewVIIndex < missionController.visualItems.count - 1) ? 1.0 : 0.3

                    Text { anchors.centerIn: parent; text: "▶"; font.pixelSize: 8; color: "#F5F7FA" }

                    MouseArea {
                        id: nextMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: inspectorRoot.navigateNext()
                    }
                    ToolTip.visible: nextMouse.containsMouse
                    ToolTip.text: "Next item"
                    ToolTip.delay: 300
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

        // State A: No item selected
        ColumnLayout {
            Layout.fillWidth: true
            visible: !currentItem
            spacing: 6

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "NO MISSION ITEM SELECTED"
                font.family: "Inter"
                font.pixelSize: 9
                font.weight: Font.Bold
                color: "#9BA8B5"
            }

            Text {
                Layout.fillWidth: true
                text: "Click a waypoint pin on the map or select from the waypoints list to inspect and edit parameters."
                font.family: "Inter"
                font.pixelSize: 8
                color: "#64748B"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // State B: Item Selected Details
        ColumnLayout {
            Layout.fillWidth: true
            visible: !!currentItem
            spacing: 8

            // 1. Command Type & Abbreviation
            RowLayout {
                Layout.fillWidth: true
                Text { text: "COMMAND"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.DemiBold; color: "#9BA8B5" }
                Item { Layout.fillWidth: true }
                Rectangle {
                    height: 20
                    implicitWidth: cmdLabel.implicitWidth + 10
                    radius: 3
                    color: "#1D2733"
                    border.color: "#FF6A00"
                    border.width: 1

                    Text {
                        id: cmdLabel
                        anchors.centerIn: parent
                        text: currentItem ? (currentItem.sequenceNumber === 0 ? "PLANNED HOME" : (currentItem.commandName ? currentItem.commandName.toUpperCase() : "WAYPOINT")) : "--"
                        font.family: "JetBrains Mono"
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: "#FF6A00"
                    }
                }
            }

            // 2. Geospatial Coordinates
            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: 3
                color: "#151C24"
                border.color: "#2C3847"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 8

                    Column {
                        Layout.fillWidth: true
                        spacing: 1
                        Text { text: "LATITUDE"; font.family: "Inter"; font.pixelSize: 7; font.weight: Font.Bold; color: "#64748B" }
                        Text {
                            text: (currentItem && currentItem.coordinate && currentItem.coordinate.isValid) ? currentItem.coordinate.latitude.toFixed(6) + "°" : "N/A"
                            font.family: "JetBrains Mono"; font.pixelSize: 9; color: "#F5F7FA"
                        }
                    }

                    Rectangle { width: 1; height: 24; color: "#2C3847" }

                    Column {
                        Layout.fillWidth: true
                        spacing: 1
                        Text { text: "LONGITUDE"; font.family: "Inter"; font.pixelSize: 7; font.weight: Font.Bold; color: "#64748B" }
                        Text {
                            text: (currentItem && currentItem.coordinate && currentItem.coordinate.isValid) ? currentItem.coordinate.longitude.toFixed(6) + "°" : "N/A"
                            font.family: "JetBrains Mono"; font.pixelSize: 9; color: "#F5F7FA"
                        }
                    }
                }
            }

            // 3. Interactive Altitude Editor
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "ALTITUDE TARGET"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.DemiBold; color: "#9BA8B5" }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: {
                            if (!currentItem) return "N/A"
                            if (currentItem.altitude && currentItem.altitude.value !== undefined) {
                                return currentItem.altitude.value.toFixed(1) + " m"
                            }
                            if (currentItem.amslEntryAlt !== undefined && !isNaN(currentItem.amslEntryAlt)) {
                                return currentItem.amslEntryAlt.toFixed(1) + " m"
                            }
                            return "0.0 m"
                        }
                        font.family: "JetBrains Mono"; font.pixelSize: 10; font.weight: Font.Bold; color: "#00C853"
                    }
                }

                // Altitude Quick Adjust Step Strip
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Rectangle {
                        Layout.fillWidth: true
                        height: 24
                        radius: 3
                        color: m10Mouse.containsMouse ? "#2C3847" : "#151C24"
                        border.color: "#2C3847"
                        border.width: 1
                        Text { anchors.centerIn: parent; text: "-10m"; font.family: "JetBrains Mono"; font.pixelSize: 8; font.weight: Font.Bold; color: "#F5F7FA" }
                        MouseArea {
                            id: m10Mouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: inspectorRoot.adjustAltitude(-10)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 24
                        radius: 3
                        color: m5Mouse.containsMouse ? "#2C3847" : "#151C24"
                        border.color: "#2C3847"
                        border.width: 1
                        Text { anchors.centerIn: parent; text: "-5m"; font.family: "JetBrains Mono"; font.pixelSize: 8; font.weight: Font.Bold; color: "#F5F7FA" }
                        MouseArea {
                            id: m5Mouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: inspectorRoot.adjustAltitude(-5)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 24
                        radius: 3
                        color: p5Mouse.containsMouse ? "#2C3847" : "#151C24"
                        border.color: "#2C3847"
                        border.width: 1
                        Text { anchors.centerIn: parent; text: "+5m"; font.family: "JetBrains Mono"; font.pixelSize: 8; font.weight: Font.Bold; color: "#00C853" }
                        MouseArea {
                            id: p5Mouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: inspectorRoot.adjustAltitude(5)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 24
                        radius: 3
                        color: p10Mouse.containsMouse ? "#2C3847" : "#151C24"
                        border.color: "#2C3847"
                        border.width: 1
                        Text { anchors.centerIn: parent; text: "+10m"; font.family: "JetBrains Mono"; font.pixelSize: 8; font.weight: Font.Bold; color: "#00C853" }
                        MouseArea {
                            id: p10Mouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: inspectorRoot.adjustAltitude(10)
                        }
                    }
                }
            }

            // 4. Altitude Reference Mode Selector
            RowLayout {
                Layout.fillWidth: true
                visible: currentItem && currentItem.altitudeMode !== undefined
                spacing: 4

                Text { text: "ALT MODE"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.DemiBold; color: "#9BA8B5" }
                Item { Layout.fillWidth: true }

                Rectangle {
                    height: 22
                    implicitWidth: relText.implicitWidth + 8
                    radius: 2
                    color: (currentItem && currentItem.altitudeMode === QGroundControl.AltitudeModeRelative) ? "#FF6A00" : "#151C24"
                    border.color: (currentItem && currentItem.altitudeMode === QGroundControl.AltitudeModeRelative) ? "#FF6A00" : "#2C3847"
                    border.width: 1
                    Text { id: relText; anchors.centerIn: parent; text: "REL"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.Bold; color: (currentItem && currentItem.altitudeMode === QGroundControl.AltitudeModeRelative) ? "#0A0F14" : "#F5F7FA" }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (currentItem) currentItem.setAltitudeMode(QGroundControl.AltitudeModeRelative)
                    }
                }

                Rectangle {
                    height: 22
                    implicitWidth: amslText.implicitWidth + 8
                    radius: 2
                    color: (currentItem && currentItem.altitudeMode === QGroundControl.AltitudeModeAbsolute) ? "#FF6A00" : "#151C24"
                    border.color: (currentItem && currentItem.altitudeMode === QGroundControl.AltitudeModeAbsolute) ? "#FF6A00" : "#2C3847"
                    border.width: 1
                    Text { id: amslText; anchors.centerIn: parent; text: "AMSL"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.Bold; color: (currentItem && currentItem.altitudeMode === QGroundControl.AltitudeModeAbsolute) ? "#0A0F14" : "#F5F7FA" }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (currentItem) currentItem.setAltitudeMode(QGroundControl.AltitudeModeAbsolute)
                    }
                }

                Rectangle {
                    height: 22
                    implicitWidth: terText.implicitWidth + 8
                    radius: 2
                    color: (currentItem && currentItem.altitudeMode === QGroundControl.AltitudeModeCalcAboveTerrain) ? "#FF6A00" : "#151C24"
                    border.color: (currentItem && currentItem.altitudeMode === QGroundControl.AltitudeModeCalcAboveTerrain) ? "#FF6A00" : "#2C3847"
                    border.width: 1
                    Text { id: terText; anchors.centerIn: parent; text: "AGL"; font.family: "Inter"; font.pixelSize: 8; font.weight: Font.Bold; color: (currentItem && currentItem.altitudeMode === QGroundControl.AltitudeModeCalcAboveTerrain) ? "#0A0F14" : "#F5F7FA" }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (currentItem) currentItem.setAltitudeMode(QGroundControl.AltitudeModeCalcAboveTerrain)
                    }
                }
            }

            // 5. Speed & Loiter Metrics
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                // Speed
                Rectangle {
                    Layout.fillWidth: true
                    height: 36
                    radius: 3
                    color: "#151C24"
                    border.color: "#2C3847"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 1
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: "SPEED"; font.family: "Inter"; font.pixelSize: 7; font.weight: Font.Bold; color: "#64748B" }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: (currentItem && currentItem.specifiedFlightSpeed > 0) ? (currentItem.specifiedFlightSpeed.toFixed(1) + " m/s") : "AUTO"
                            font.family: "JetBrains Mono"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA"
                        }
                    }
                }

                // Hold / Loiter
                Rectangle {
                    Layout.fillWidth: true
                    height: 36
                    radius: 3
                    color: "#151C24"
                    border.color: "#2C3847"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 1
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: "HOLD TIME"; font.family: "Inter"; font.pixelSize: 7; font.weight: Font.Bold; color: "#64748B" }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: (currentItem && currentItem.isLoiterItem && currentItem.loiterRadius > 0) ? (currentItem.loiterRadius.toFixed(0) + "m LOITER") : "PASS THROUGH"
                            font.family: "JetBrains Mono"; font.pixelSize: 8; color: "#9BA8B5"
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#2C3847" }

            // Delete Selected Item Action
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: 3
                color: delItemMouse.containsMouse ? "#D32F2F" : "#3A1E24"
                border.color: "#F44336"
                border.width: 1
                visible: currentItem && currentItem.sequenceNumber > 0

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    Text { text: "✕"; font.pixelSize: 9; color: "#FFFFFF" }
                    Text { text: "DELETE ITEM"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#FFFFFF" }
                }

                MouseArea {
                    id: delItemMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (missionController && missionController.currentPlanViewVIIndex >= 0) {
                            missionController.removeVisualItem(missionController.currentPlanViewVIIndex)
                        }
                    }
                }
            }
        }
    }
}
