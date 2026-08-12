/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Mission Execution HUD
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtPositioning
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: hudRoot

    // =========================================================================
    // AUTHORITATIVE PUBLIC API
    // =========================================================================
    property var activeVehicle: null
    property var missionController: null
    property var planMasterController: null
    property var guidedActionsController: null
    property var vehicleLinkManager: activeVehicle ? activeVehicle.vehicleLinkManager : null

    // Presentation Layout
    implicitWidth: 380
    implicitHeight: mainLayout.implicitHeight + 20
    radius: 4
    color: "#E6151C24" // 90% opacity dark glass
    border.color: isCommLost ? "#F44336" : (isMissionExecuting ? "#00C853" : (isMissionPaused ? "#FFC107" : "#2C3847"))
    border.width: 1

    Behavior on border.color { ColorAnimation { duration: 250 } }

    // =========================================================================
    // REACTIVE STATE DERIVATION (Strictly Authoritative)
    // =========================================================================
    readonly property bool isCommLost:         activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && !activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isArmed:            activeVehicle !== null && activeVehicle.armed
    readonly property bool isFlying:           activeVehicle !== null && activeVehicle.flying
    readonly property string flightMode:       activeVehicle ? activeVehicle.flightMode : ""

    readonly property bool isMissionMode: activeVehicle !== null && activeVehicle.missionFlightMode !== "" && (flightMode === activeVehicle.missionFlightMode)
    readonly property bool isPauseMode:   activeVehicle !== null && activeVehicle.pauseFlightMode !== "" && (flightMode === activeVehicle.pauseFlightMode)
    readonly property bool isRTLMode:     activeVehicle !== null && activeVehicle.rtlFlightMode !== "" && (flightMode === activeVehicle.rtlFlightMode || (activeVehicle.smartRTLFlightMode && flightMode === activeVehicle.smartRTLFlightMode))
    readonly property bool isLandMode:    activeVehicle !== null && activeVehicle.landFlightMode !== "" && (flightMode === activeVehicle.landFlightMode)

    readonly property bool isMissionExecuting: isArmed && isFlying && isMissionMode
    readonly property bool isMissionPaused:    isArmed && isFlying && isPauseMode

    readonly property bool hasMissionItems: !!(missionController && missionController.visualItems && missionController.visualItems.count > 1)

    // Track whether mission was executing when comm was lost (prevents fake reset)
    property bool wasExecutingOnCommLoss: false

    onIsCommLostChanged: {
        if (isCommLost && isMissionExecuting) {
            wasExecutingOnCommLoss = true
        } else if (!isCommLost) {
            wasExecutingOnCommLoss = false
        }
    }

    // =========================================================================
    // PRESENTATION-LAYER ELAPSED TIME TRACKING
    // =========================================================================
    property int elapsedSeconds: 0
    property bool missionHasStarted: false

    Timer {
        id: executionTimer
        interval: 1000
        repeat: true
        running: hudRoot.isMissionExecuting && !hudRoot.isCommLost
        onTriggered: {
            hudRoot.elapsedSeconds += 1
        }
    }

    onIsMissionExecutingChanged: {
        if (isMissionExecuting) {
            if (!missionHasStarted) {
                missionHasStarted = true
                elapsedSeconds = 0
            }
        }
    }

    onIsArmedChanged: {
        if (!isArmed && !isFlying) {
            // Disarmed on ground -> reset for next flight
            missionHasStarted = false
        }
    }

    onActiveVehicleChanged: {
        elapsedSeconds = 0
        missionHasStarted = false
        wasExecutingOnCommLoss = false
    }

    // =========================================================================
    // HELPER FUNCTIONS (Geospatial & Progression Calculation)
    // =========================================================================

    function getCurrentVisualItem() {
        if (!hasMissionItems) return null
        var currIdx = missionController.currentMissionIndex
        if (currIdx >= 0 && currIdx < missionController.visualItems.count) {
            return missionController.visualItems.get(currIdx)
        }
        return null
    }

    function getWaypointText() {
        if (!activeVehicle || !hasMissionItems) return "N/A"
        var totalWPs = missionController.missionItemCount > 0 ? missionController.missionItemCount : (missionController.visualItems.count - 1)
        var currIdx = missionController.currentMissionIndex
        if (currIdx <= 0) {
            return "WP 1 / " + totalWPs
        }
        var vi = getCurrentVisualItem()
        var seq = (vi && vi.sequenceNumber !== undefined) ? vi.sequenceNumber : currIdx
        return "WP " + seq + " / " + totalWPs
    }

    function getCurrentCommandText() {
        if (!activeVehicle || !hasMissionItems) return "N/A"
        var vi = getCurrentVisualItem()
        if (!vi) return "N/A"
        if (vi.commandName && vi.commandName.length > 0) {
            return vi.commandName.toUpperCase()
        }
        if (vi.commandDescription && vi.commandDescription.length > 0) {
            return vi.commandDescription.toUpperCase()
        }
        return "WAYPOINT"
    }

    function getDistanceToNextText() {
        if (!activeVehicle || !activeVehicle.coordinate || !activeVehicle.coordinate.isValid || !hasMissionItems) return "N/A"
        var vi = getCurrentVisualItem()
        if (!vi || !vi.coordinate || !vi.coordinate.isValid) return "N/A"
        var dist = activeVehicle.coordinate.distanceTo(vi.coordinate)
        if (isNaN(dist) || dist < 0) return "N/A"
        if (dist >= 1000) {
            return (dist / 1000.0).toFixed(2) + " km"
        }
        return Math.round(dist) + " m"
    }

    function getRemainingRouteText() {
        if (!activeVehicle || !activeVehicle.coordinate || !activeVehicle.coordinate.isValid || !hasMissionItems) return "N/A"
        var currIdx = Math.max(1, missionController.currentMissionIndex)
        var items = missionController.visualItems
        if (!items || items.count <= 1) return "N/A"

        var totalDist = 0
        var prevCoord = activeVehicle.coordinate

        for (var i = currIdx; i < items.count; i++) {
            var item = items.get(i)
            if (item && item.coordinate && item.coordinate.isValid) {
                var segDist = prevCoord.distanceTo(item.coordinate)
                if (!isNaN(segDist) && segDist > 0) {
                    totalDist += segDist
                    prevCoord = item.coordinate
                }
            }
        }

        if (totalDist <= 0) return "N/A"
        if (totalDist >= 1000) {
            return (totalDist / 1000.0).toFixed(2) + " km"
        }
        return Math.round(totalDist) + " m"
    }

    function formatTime(seconds) {
        if (isNaN(seconds) || seconds <= 0) return "N/A"
        var hrs = Math.floor(seconds / 3600)
        var mins = Math.floor((seconds % 3600) / 60)
        var secs = Math.floor(seconds % 60)
        var mStr = (mins < 10 ? "0" : "") + mins
        var sStr = (secs < 10 ? "0" : "") + secs
        if (hrs > 0) {
            var hStr = (hrs < 10 ? "0" : "") + hrs
            return hStr + ":" + mStr + ":" + sStr
        }
        return mStr + ":" + sStr
    }

    function getElapsedText() {
        if (!activeVehicle || !missionHasStarted || elapsedSeconds <= 0) return "N/A"
        return formatTime(elapsedSeconds)
    }

    function getProgressFraction() {
        if (!activeVehicle || !hasMissionItems) return 0.0
        var totalWPs = missionController.missionItemCount > 0 ? missionController.missionItemCount : (missionController.visualItems.count - 1)
        if (totalWPs <= 0) return 0.0
        var currIdx = missionController.currentMissionIndex
        if (currIdx <= 0) return 0.0
        var fraction = currIdx / totalWPs
        return Math.min(1.0, Math.max(0.0, fraction))
    }

    function getEstimatedRemainingText() {
        if (!activeVehicle || !isMissionExecuting || !missionController || missionController.missionTime <= 0) return "N/A"
        var pct = getProgressFraction()
        if (isNaN(pct) || pct < 0 || pct >= 1.0) {
            if (pct >= 1.0) return "00:00"
            return "N/A"
        }
        var remSecs = Math.max(0, Math.round((1.0 - pct) * missionController.missionTime))
        return "ETA " + formatTime(remSecs)
    }

    function getExecutionStateTitle() {
        if (!activeVehicle) return "OFFLINE"
        if (isCommLost) {
            return (wasExecutingOnCommLoss || isMissionMode) ? "AUTOPILOT EXECUTING (LINK LOST)" : "LINK LOST"
        }
        if (isMissionExecuting) return "EXECUTING MISSION"
        if (isMissionPaused) return "MISSION PAUSED"
        if (isRTLMode) return "RETURNING (RTL)"
        if (isLandMode) return "LANDING"
        if (isArmed && !isFlying && isMissionMode) return "ARMED (READY TO FLY)"
        if (isArmed) return "ARMED (" + (flightMode ? flightMode.toUpperCase() : "MANUAL") + ")"
        if (hasMissionItems) return "PLAN LOADED (DISARMED)"
        return "STANDBY (NO PLAN)"
    }

    function getExecutionStateColor() {
        if (!activeVehicle) return "#64748B"
        if (isCommLost) return "#F44336"
        if (isMissionExecuting) return "#00C853"
        if (isMissionPaused) return "#FFC107"
        if (isRTLMode) return "#FF6A00"
        if (isLandMode) return "#FFC107"
        if (isArmed) return "#FF6A00"
        if (hasMissionItems) return "#9BA8B5"
        return "#64748B"
    }

    // =========================================================================
    // VISUAL LAYOUT
    // =========================================================================
    ColumnLayout {
        id: mainLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 8

        // ---------------------------------------------------------------------
        // 1. HEADER ROW (Title & Reactive State Chip)
        // ---------------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Rectangle {
                width: 6
                height: 6
                radius: 3
                color: hudRoot.getExecutionStateColor()
            }

            Text {
                text: "MISSION EXECUTION"
                font.family: "Inter"
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 0.8
                color: "#F5F7FA"
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                height: 20
                implicitWidth: stateText.implicitWidth + 12
                radius: 3
                color: Qt.rgba(
                    hudRoot.getExecutionStateColor() === "#00C853" ? 0.0 : (hudRoot.getExecutionStateColor() === "#F44336" ? 0.95 : (hudRoot.getExecutionStateColor() === "#FFC107" ? 1.0 : 0.4)),
                    hudRoot.getExecutionStateColor() === "#00C853" ? 0.78 : (hudRoot.getExecutionStateColor() === "#F44336" ? 0.26 : (hudRoot.getExecutionStateColor() === "#FFC107" ? 0.75 : 0.4)),
                    hudRoot.getExecutionStateColor() === "#00C853" ? 0.32 : (hudRoot.getExecutionStateColor() === "#F44336" ? 0.21 : (hudRoot.getExecutionStateColor() === "#FFC107" ? 0.02 : 0.4)),
                    0.15
                )
                border.color: hudRoot.getExecutionStateColor()
                border.width: 1

                Text {
                    id: stateText
                    anchors.centerIn: parent
                    text: hudRoot.getExecutionStateTitle()
                    font.family: "JetBrains Mono"
                    font.pixelSize: 8
                    font.weight: Font.Bold
                    color: hudRoot.getExecutionStateColor()
                }
            }
        }

        // ---------------------------------------------------------------------
        // 2. TELEMETRY LOSS WARNING BANNER (Only when isCommLost)
        // ---------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 24
            radius: 3
            color: Qt.rgba(0.95, 0.26, 0.21, 0.2)
            border.color: "#F44336"
            border.width: 1
            visible: hudRoot.isCommLost

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                Text {
                    text: "●"
                    font.pixelSize: 10
                    color: "#F44336"
                }

                Text {
                    text: (hudRoot.wasExecutingOnCommLoss || hudRoot.isMissionMode) ? "AUTOPILOT EXECUTING — TELEMETRY LOST" : "VEHICLE TELEMETRY LINK LOST"
                    font.family: "Inter"
                    font.pixelSize: 8
                    font.weight: Font.Bold
                    font.letterSpacing: 0.5
                    color: "#F5F7FA"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "FAILSAFE MONITOR"
                    font.family: "JetBrains Mono"
                    font.pixelSize: 7
                    font.weight: Font.Bold
                    color: "#FFC107"
                }
            }
        }

        // ---------------------------------------------------------------------
        // 3. MAIN MISSION ROW (WP X/Y | CURRENT COMMAND | DIST TO NEXT)
        // ---------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 48
            radius: 3
            color: "#151C24"
            border.color: "#2C3847"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                // Waypoint Index
                Column {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: "ACTIVE WAYPOINT"
                        font.family: "Inter"
                        font.pixelSize: 7
                        font.weight: Font.Bold
                        color: "#64748B"
                    }
                    Text {
                        text: hudRoot.getWaypointText()
                        font.family: "JetBrains Mono"
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        color: hudRoot.activeVehicle && hudRoot.hasMissionItems ? "#FF6A00" : "#64748B"
                    }
                }

                Rectangle { width: 1; height: 28; color: "#2C3847" }

                // Current Command
                Column {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: "CURRENT COMMAND"
                        font.family: "Inter"
                        font.pixelSize: 7
                        font.weight: Font.Bold
                        color: "#64748B"
                    }
                    Text {
                        text: hudRoot.getCurrentCommandText()
                        font.family: "JetBrains Mono"
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: "#F5F7FA"
                        elide: Text.ElideRight
                    }
                }

                Rectangle { width: 1; height: 28; color: "#2C3847" }

                // Distance to Next
                Column {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: "DIST TO NEXT"
                        font.family: "Inter"
                        font.pixelSize: 7
                        font.weight: Font.Bold
                        color: "#64748B"
                    }
                    Text {
                        text: hudRoot.getDistanceToNextText()
                        font.family: "JetBrains Mono"
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        color: hudRoot.activeVehicle && hudRoot.hasMissionItems ? "#00C853" : "#64748B"
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // 4. SECONDARY METRICS (ROUTE REMAINING | ELAPSED | ESTIMATED REMAINING)
        // ---------------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            // Route Remaining
            Rectangle {
                Layout.fillWidth: true
                height: 38
                radius: 3
                color: "#151C24"
                border.color: "#2C3847"
                border.width: 1

                Column {
                    anchors.centerIn: parent
                    spacing: 1
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "ROUTE REMAINING"
                        font.family: "Inter"
                        font.pixelSize: 7
                        font.weight: Font.Bold
                        color: "#64748B"
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: hudRoot.getRemainingRouteText()
                        font.family: "JetBrains Mono"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: "#F5F7FA"
                    }
                }
            }

            // Elapsed Flight Time
            Rectangle {
                Layout.fillWidth: true
                height: 38
                radius: 3
                color: "#151C24"
                border.color: "#2C3847"
                border.width: 1

                Column {
                    anchors.centerIn: parent
                    spacing: 1
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "ELAPSED TIME"
                        font.family: "Inter"
                        font.pixelSize: 7
                        font.weight: Font.Bold
                        color: "#64748B"
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: hudRoot.getElapsedText()
                        font.family: "JetBrains Mono"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: "#F5F7FA"
                    }
                }
            }

            // Estimated Remaining (ETA)
            Rectangle {
                Layout.fillWidth: true
                height: 38
                radius: 3
                color: "#151C24"
                border.color: "#2C3847"
                border.width: 1

                Column {
                    anchors.centerIn: parent
                    spacing: 1
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "EST. REMAINING"
                        font.family: "Inter"
                        font.pixelSize: 7
                        font.weight: Font.Bold
                        color: "#64748B"
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: hudRoot.getEstimatedRemainingText()
                        font.family: "JetBrains Mono"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: hudRoot.isMissionExecuting ? "#00C853" : "#9BA8B5"
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // 5. PROGRESS SECTION (Horizontal Bar & Percentage)
        // ---------------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "MISSION PROGRESS"
                    font.family: "Inter"
                    font.pixelSize: 7
                    font.weight: Font.Bold
                    font.letterSpacing: 0.5
                    color: "#64748B"
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: {
                        if (!hudRoot.activeVehicle || !hudRoot.hasMissionItems) return "N/A"
                        return Math.round(hudRoot.getProgressFraction() * 100) + "%"
                    }
                    font.family: "JetBrains Mono"
                    font.pixelSize: 8
                    font.weight: Font.Bold
                    color: hudRoot.activeVehicle && hudRoot.hasMissionItems ? "#FF6A00" : "#64748B"
                }
            }

            // Progress Track
            Rectangle {
                Layout.fillWidth: true
                height: 4
                radius: 2
                color: "#2C3847"

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    radius: 2
                    width: parent.width * hudRoot.getProgressFraction()
                    color: hudRoot.isMissionExecuting ? "#00C853" : (hudRoot.isMissionPaused ? "#FFC107" : "#FF6A00")
                    visible: hudRoot.activeVehicle !== null && hudRoot.hasMissionItems && width > 0

                    Behavior on width {
                        NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
                    }
                }
            }
        }
    }
}
