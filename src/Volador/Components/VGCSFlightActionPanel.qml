/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Flight Action Panel
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: actionPanelRoot

    property var guidedController
    property var activeVehicle: (typeof QGroundControl !== "undefined" && QGroundControl.multiVehicleManager) ? QGroundControl.multiVehicleManager.activeVehicle : null

    // Command Feedback State Machine
    // States: "READY", "SENDING", "ACCEPTED", "REJECTED", "FAILED", "TIMEOUT"
    property string commandState: "READY"
    property int    activeActionCode: 0
    property string activeActionName: ""
    property string feedbackTitle: ""
    property string feedbackReason: ""
    property string lastAutopilotMsg: ""

    // Action Availability Computations (Derived strictly from authoritative vehicle state)
    readonly property bool isCommLost:         activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isVehicleConnected: activeVehicle !== null && activeVehicle.vehicleLinkManager !== null && !activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool isArmAvailable:     isVehicleConnected && guidedController && !activeVehicle.armed && guidedController.showArm
    readonly property bool isDisarmAvailable:  isVehicleConnected && guidedController && activeVehicle.armed && (guidedController.showDisarm || guidedController.showEmergenyStop)
    readonly property bool isTakeoffAvailable: isVehicleConnected && guidedController && guidedController.showTakeoff
    readonly property bool isMissionAvailable: isVehicleConnected && guidedController && (guidedController.showStartMission || guidedController.showContinueMission)
    readonly property bool isRTLAvailable:     isVehicleConnected && guidedController && guidedController.showRTL
    readonly property bool isLandAvailable:    isVehicleConnected && guidedController && guidedController.showLand
    readonly property bool isPauseAvailable:   isVehicleConnected && guidedController && guidedController.showPause

    implicitWidth: mainCol.implicitWidth + 24
    implicitHeight: mainCol.implicitHeight + 16
    radius: 4
    color: "#E6151C24" // 90% opacity dark surface
    border.color: (commandState === "REJECTED" || commandState === "FAILED") ? "#F44336" : (commandState === "TIMEOUT" ? "#FFC107" : (commandState === "ACCEPTED" ? "#00C853" : (commandState === "SENDING" ? "#FF6A00" : "#2C3847")))
    border.width: 1

    Behavior on border.color { ColorAnimation { duration: 200 } }

    // Pulsing animation for SENDING state
    SequentialAnimation on opacity {
        id: pulseAnimation
        running: commandState === "SENDING"
        loops: Animation.Infinite
        NumberAnimation { to: 0.75; duration: 600; easing.type: Easing.InOutQuad }
        NumberAnimation { to: 1.0;  duration: 600; easing.type: Easing.InOutQuad }
    }

    // Timer to reset successful command state back to READY
    Timer {
        id: feedbackResetTimer
        interval: 2500
        repeat: false
        onTriggered: resetToReady()
    }

    // Timer to auto-dismiss error / failure / timeout states
    Timer {
        id: feedbackErrorTimer
        interval: 8000
        repeat: false
        onTriggered: resetToReady()
    }

    // Safety watchdog timer: prevent button from ever getting stuck in SENDING
    Timer {
        id: commandTimeoutWatchdog
        interval: 7000
        repeat: false
        onTriggered: {
            if (commandState === "SENDING") {
                commandState = "TIMEOUT"
                feedbackTitle = (activeActionName ? activeActionName : "COMMAND") + " TIMEOUT"
                feedbackReason = qsTr("Vehicle did not acknowledge command within timeout period.")
                feedbackErrorTimer.restart()
            }
        }
    }

    function resetToReady() {
        commandTimeoutWatchdog.stop()
        feedbackResetTimer.stop()
        feedbackErrorTimer.stop()
        commandState = "READY"
        activeActionCode = 0
        activeActionName = ""
        feedbackTitle = ""
        feedbackReason = ""
    }

    function getActionName(code) {
        if (!guidedController) return "ACTION"
        if (code === guidedController.actionArm || code === guidedController.actionForceArm) return "ARM"
        if (code === guidedController.actionDisarm) return "DISARM"
        if (code === guidedController.actionEmergencyStop) return "EMERGENCY STOP"
        if (code === guidedController.actionTakeoff) return "TAKEOFF"
        if (code === guidedController.actionRTL) return "RTL"
        if (code === guidedController.actionLand) return "LAND"
        if (code === guidedController.actionPause) return "PAUSE"
        if (code === guidedController.actionStartMission) return "START MISSION"
        if (code === guidedController.actionContinueMission) return "RESUME MISSION"
        return "COMMAND"
    }

    function handleActionExecuted(actionCode) {
        if (!activeVehicle) return
        commandState = "SENDING"
        activeActionCode = actionCode
        activeActionName = getActionName(actionCode)
        feedbackTitle = "TRANSMITTING " + activeActionName + " COMMAND"
        feedbackReason = qsTr("Awaiting acknowledgement from flight controller...")
        commandTimeoutWatchdog.restart()
    }

    function handleActionCancelled(actionCode) {
        if (commandState === "SENDING" && activeActionCode === actionCode) {
            resetToReady()
        }
    }

    Connections {
        target: guidedController
        function onActionExecuted(actionCode, actionData) {
            handleActionExecuted(actionCode)
        }
        function onActionCancelled(actionCode) {
            handleActionCancelled(actionCode)
        }
    }

    Connections {
        target: activeVehicle

        function onMavCommandResult(vehicleId, targetComponent, command, ackResult, failureCode) {
            commandTimeoutWatchdog.stop()

            // Check if failure is due to command timeout (no response)
            if (failureCode === 1 /* MavCmdResultFailureNoResponseToCommand */) {
                commandState = "TIMEOUT"
                feedbackTitle = (activeActionName ? activeActionName : "COMMAND") + " TIMEOUT"
                feedbackReason = qsTr("Vehicle did not respond to command (Timeout).")
                feedbackErrorTimer.restart()
                return
            }

            // Acknowledged by autopilot
            if (ackResult === 0 /* MAV_RESULT_ACCEPTED */) {
                commandState = "ACCEPTED"
                feedbackTitle = (activeActionName ? activeActionName : "COMMAND") + " ACCEPTED"
                feedbackReason = qsTr("Autopilot acknowledged and confirmed execution.")
                feedbackResetTimer.restart()
            } else if (ackResult === 1 /* MAV_RESULT_TEMPORARILY_REJECTED */ || ackResult === 2 /* MAV_RESULT_DENIED */) {
                commandState = "REJECTED"
                feedbackTitle = (activeActionName ? activeActionName : "COMMAND") + " REJECTED"
                feedbackReason = lastAutopilotMsg ? lastAutopilotMsg : (activeVehicle.prearmError ? activeVehicle.prearmError : (ackResult === 1 ? qsTr("Temporarily rejected by autopilot.") : qsTr("Command denied by autopilot safety checks.")))
                feedbackErrorTimer.restart()
            } else if (ackResult === 3 /* MAV_RESULT_UNSUPPORTED */ || ackResult === 4 /* MAV_RESULT_FAILED */) {
                commandState = "FAILED"
                feedbackTitle = (activeActionName ? activeActionName : "COMMAND") + " FAILED"
                feedbackReason = lastAutopilotMsg ? lastAutopilotMsg : (activeVehicle.prearmError ? activeVehicle.prearmError : (ackResult === 3 ? qsTr("Command not supported by flight controller.") : qsTr("Command execution failed on autopilot.")))
                feedbackErrorTimer.restart()
            }
        }

        function onTextMessageReceived(sysid, componentid, severity, text, description) {
            if (text && text.length > 0) {
                lastAutopilotMsg = text
                if (commandState === "SENDING" || commandState === "REJECTED" || commandState === "FAILED") {
                    var lower = text.toLowerCase()
                    if (lower.indexOf("arm") !== -1 || lower.indexOf("failsafe") !== -1 || lower.indexOf("reject") !== -1 || lower.indexOf("denied") !== -1 || lower.indexOf("fail") !== -1 || lower.indexOf("preflight") !== -1 || lower.indexOf("prearm") !== -1) {
                        feedbackReason = text
                    }
                }
            }
        }

        function onPrearmErrorChanged(prearmError) {
            if (prearmError && prearmError.length > 0) {
                lastAutopilotMsg = prearmError
                if (commandState === "REJECTED" || commandState === "FAILED") {
                    feedbackReason = prearmError
                }
            }
        }

        function onArmedChanged(armed) {
            if (commandState === "SENDING") {
                if (guidedController && (activeActionCode === guidedController.actionArm || activeActionCode === guidedController.actionForceArm) && armed) {
                    commandState = "ACCEPTED"
                    feedbackTitle = "ARM COMMAND ACCEPTED"
                    feedbackReason = qsTr("Vehicle is now ARMED.")
                    commandTimeoutWatchdog.stop()
                    feedbackResetTimer.restart()
                } else if (guidedController && (activeActionCode === guidedController.actionDisarm || activeActionCode === guidedController.actionEmergencyStop) && !armed) {
                    commandState = "ACCEPTED"
                    feedbackTitle = "DISARM COMMAND ACCEPTED"
                    feedbackReason = qsTr("Vehicle is now DISARMED.")
                    commandTimeoutWatchdog.stop()
                    feedbackResetTimer.restart()
                }
            }
        }

        function onFlightModeChanged(flightMode) {
            if (commandState === "SENDING" && guidedController) {
                if (activeActionCode === guidedController.actionRTL ||
                    activeActionCode === guidedController.actionLand ||
                    activeActionCode === guidedController.actionStartMission ||
                    activeActionCode === guidedController.actionContinueMission ||
                    activeActionCode === guidedController.actionPause) {
                    commandState = "ACCEPTED"
                    feedbackTitle = activeActionName + " ACCEPTED"
                    feedbackReason = qsTr("Flight mode transitioned to: %1").arg(flightMode ? flightMode.toUpperCase() : "REQUESTED")
                    commandTimeoutWatchdog.stop()
                    feedbackResetTimer.restart()
                }
            }
        }
    }

    onActiveVehicleChanged: {
        resetToReady()
    }

    ColumnLayout {
        id: mainCol
        anchors.centerIn: parent
        spacing: 6

        // ====================================================================
        // 1. STATUS & COMMAND FEEDBACK BANNER (Integrated Aerospace Feedback)
        // ====================================================================
        Rectangle {
            id: feedbackBanner
            Layout.preferredWidth: actionRow.implicitWidth
            Layout.preferredHeight: bannerContent.implicitHeight + 8
            radius: 3
            visible: !activeVehicle || isCommLost || commandState !== "READY" || (activeVehicle && !activeVehicle.armed && activeVehicle.prearmError && activeVehicle.prearmError.length > 0)

            color: {
                if (!activeVehicle) return "#151C24"
                if (isCommLost) return Qt.rgba(0.957, 0.263, 0.212, 0.15)
                if (commandState === "SENDING") return Qt.rgba(0.129, 0.588, 0.953, 0.15)
                if (commandState === "ACCEPTED") return Qt.rgba(0.0, 0.784, 0.325, 0.15)
                if (commandState === "REJECTED" || commandState === "FAILED") return Qt.rgba(0.957, 0.263, 0.212, 0.2)
                if (commandState === "TIMEOUT") return Qt.rgba(1.0, 0.757, 0.027, 0.18)
                if (!activeVehicle.armed && activeVehicle.prearmError && activeVehicle.prearmError.length > 0) return Qt.rgba(1.0, 0.757, 0.027, 0.15)
                return "#151C24"
            }

            border.color: {
                if (!activeVehicle) return "#2C3847"
                if (isCommLost) return "#F44336"
                if (commandState === "SENDING") return "#2196F3"
                if (commandState === "ACCEPTED") return "#00C853"
                if (commandState === "REJECTED" || commandState === "FAILED") return "#F44336"
                if (commandState === "TIMEOUT") return "#FFC107"
                if (!activeVehicle.armed && activeVehicle.prearmError && activeVehicle.prearmError.length > 0) return "#FFC107"
                return "#2C3847"
            }
            border.width: 1

            ColumnLayout {
                id: bannerContent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 6
                spacing: 3

                // Top Line: Icon + Title + Action
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Rectangle {
                        width: 6
                        height: 6
                        radius: 3
                        color: {
                            if (!activeVehicle) return "#64748B"
                            if (isCommLost) return "#F44336"
                            if (commandState === "SENDING") return "#2196F3"
                            if (commandState === "ACCEPTED") return "#00C853"
                            if (commandState === "REJECTED" || commandState === "FAILED") return "#F44336"
                            if (commandState === "TIMEOUT") return "#FFC107"
                            return "#FFC107"
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: {
                            if (!activeVehicle) return "NO VEHICLE CONNECTED — FLIGHT CONTROLS LOCKED"
                            if (isCommLost) return "VEHICLE CONNECTION LOST — CONTROLS DISABLED"
                            if (commandState === "SENDING") return feedbackTitle.toUpperCase()
                            if (commandState === "ACCEPTED") return feedbackTitle.toUpperCase()
                            if (commandState === "REJECTED") return feedbackTitle.toUpperCase()
                            if (commandState === "FAILED") return feedbackTitle.toUpperCase()
                            if (commandState === "TIMEOUT") return feedbackTitle.toUpperCase()
                            if (!activeVehicle.armed && activeVehicle.prearmError) return "PRE-ARM SAFETY CHECK ACTIVE"
                            return ""
                        }
                        font.family: "Inter"
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        font.letterSpacing: 0.5
                        color: {
                            if (!activeVehicle) return "#9BA8B5"
                            if (isCommLost) return "#F44336"
                            if (commandState === "SENDING") return "#90CAF9"
                            if (commandState === "ACCEPTED") return "#00E676"
                            if (commandState === "REJECTED" || commandState === "FAILED") return "#FF8A80"
                            if (commandState === "TIMEOUT") return "#FFE082"
                            return "#FFD54F"
                        }
                        elide: Text.ElideRight
                    }

                    // Dismiss Button (for errors and timeouts)
                    Rectangle {
                        visible: commandState === "REJECTED" || commandState === "FAILED" || commandState === "TIMEOUT"
                        Layout.preferredHeight: 16
                        Layout.preferredWidth: dismissText.implicitWidth + 12
                        radius: 2
                        color: dismissMouse.containsMouse ? "#3A4B5F" : "#2C3847"
                        border.color: "#64748B"
                        border.width: 1

                        Text {
                            id: dismissText
                            anchors.centerIn: parent
                            text: "DISMISS"
                            font.family: "Inter"
                            font.pixelSize: 8
                            font.weight: Font.Bold
                            color: "#F5F7FA"
                        }

                        MouseArea {
                            id: dismissMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: resetToReady()
                        }
                    }
                }

                // Sub-Line: Genuine Reason / Explanation (Monospace for telemetry precision)
                Text {
                    Layout.fillWidth: true
                    visible: (commandState !== "READY" && feedbackReason.length > 0) || (activeVehicle && !activeVehicle.armed && activeVehicle.prearmError && activeVehicle.prearmError.length > 0)
                    text: {
                        if (commandState !== "READY" && feedbackReason.length > 0) {
                            return "Reason: " + feedbackReason
                        }
                        if (activeVehicle && !activeVehicle.armed && activeVehicle.prearmError) {
                            return "Reason: " + activeVehicle.prearmError
                        }
                        return ""
                    }
                    font.family: "JetBrains Mono"
                    font.pixelSize: 8
                    font.weight: Font.Normal
                    color: "#F5F7FA"
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }
        }

        // ====================================================================
        // 2. ACTION CONTROLS BUTTON ROW
        // ====================================================================
        RowLayout {
            id: actionRow
            spacing: 8

            // ----------------------------------------------------------------
            // 1. ARM / DISARM Action Button
            // ----------------------------------------------------------------
            Rectangle {
                id: armBtn
                readonly property bool isArmed: activeVehicle ? activeVehicle.armed : false
                readonly property bool isSending: commandState === "SENDING" && guidedController && (activeActionCode === guidedController.actionArm || activeActionCode === guidedController.actionDisarm || activeActionCode === guidedController.actionEmergencyStop || activeActionCode === guidedController.actionForceArm)
                readonly property bool isAccepted: commandState === "ACCEPTED" && guidedController && (activeActionCode === guidedController.actionArm || activeActionCode === guidedController.actionDisarm)
                readonly property bool isRejected: (commandState === "REJECTED" || commandState === "FAILED") && guidedController && (activeActionCode === guidedController.actionArm || activeActionCode === guidedController.actionDisarm)
                readonly property bool isTimeout: commandState === "TIMEOUT" && guidedController && (activeActionCode === guidedController.actionArm || activeActionCode === guidedController.actionDisarm)
                readonly property bool isActionAvailable: isArmed ? isDisarmAvailable : isArmAvailable

                Layout.preferredHeight: 32
                Layout.preferredWidth: Math.max(armContent.implicitWidth + 22, 92)
                implicitWidth: Layout.preferredWidth
                implicitHeight: 32
                radius: 3

                color: {
                    if (isSending) return Qt.rgba(0.129, 0.588, 0.953, 0.3)
                    if (isAccepted) return "#00C853"
                    if (isRejected) return "#D32F2F"
                    if (isTimeout) return "#FFA000"
                    if (!isVehicleConnected) return "#151C24"
                    if (isArmed) {
                        return armMouse.containsMouse ? "#D32F2F" : "#F44336"
                    } else {
                        return isActionAvailable ? (armMouse.containsMouse ? "#FF8533" : "#FF6A00") : "#151C24"
                    }
                }

                border.color: {
                    if (isSending) return "#2196F3"
                    if (isAccepted) return "#00E676"
                    if (isRejected) return "#FF8A80"
                    if (isTimeout) return "#FFE082"
                    if (!isActionAvailable) return "#2C3847"
                    return "transparent"
                }
                border.width: isActionAvailable ? 0 : 1

                opacity: (!isVehicleConnected || !isActionAvailable) && !isSending && !isAccepted && !isRejected && !isTimeout ? 0.35 : 1.0

                Row {
                    id: armContent
                    anchors.centerIn: parent
                    spacing: 5

                    Rectangle {
                        width: 6
                        height: 6
                        radius: 3
                        anchors.verticalCenter: parent.verticalCenter
                        color: {
                            if (armBtn.isSending) return "#2196F3"
                            if (armBtn.isAccepted) return "#FFFFFF"
                            if (armBtn.isRejected) return "#FFFFFF"
                            if (armBtn.isTimeout) return "#0A0F14"
                            if (!actionPanelRoot.isVehicleConnected) return "#64748B"
                            return "#FFFFFF"
                        }
                    }

                    Text {
                        text: {
                            if (armBtn.isSending) return (armBtn.isArmed ? "DISARMING..." : "ARMING...")
                            if (armBtn.isAccepted) return (armBtn.isArmed ? "✔ ARMED" : "✔ DISARMED")
                            if (armBtn.isRejected) return "✖ REJECTED"
                            if (armBtn.isTimeout) return "⏱ TIMEOUT"
                            return armBtn.isArmed ? "DISARM" : "ARM"
                        }
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.5
                        color: armBtn.isTimeout ? "#0A0F14" : (armBtn.isActionAvailable || armBtn.isArmed || armBtn.isSending || armBtn.isAccepted || armBtn.isRejected ? "#FFFFFF" : "#64748B")
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: armMouse
                    anchors.fill: parent
                    hoverEnabled: armBtn.isActionAvailable && !armBtn.isSending
                    cursorShape: armBtn.isActionAvailable && !armBtn.isSending ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!actionPanelRoot.isVehicleConnected || !guidedController || armBtn.isSending) return
                        if (armBtn.isArmed) {
                            if (guidedController.showDisarm) {
                                guidedController.confirmAction(guidedController.actionDisarm)
                            } else if (guidedController.showEmergenyStop) {
                                guidedController.confirmAction(guidedController.actionEmergencyStop)
                            }
                        } else {
                            if (guidedController.showArm) {
                                guidedController.confirmAction(guidedController.actionArm)
                            }
                        }
                    }
                }
            }

            // ----------------------------------------------------------------
            // 2. TAKEOFF Action Button
            // ----------------------------------------------------------------
            Rectangle {
                id: takeoffBtn
                readonly property bool isSending: commandState === "SENDING" && guidedController && activeActionCode === guidedController.actionTakeoff
                readonly property bool isAccepted: commandState === "ACCEPTED" && guidedController && activeActionCode === guidedController.actionTakeoff
                readonly property bool isRejected: (commandState === "REJECTED" || commandState === "FAILED") && guidedController && activeActionCode === guidedController.actionTakeoff
                readonly property bool isTimeout: commandState === "TIMEOUT" && guidedController && activeActionCode === guidedController.actionTakeoff
                readonly property bool isActionAvailable: isTakeoffAvailable

                Layout.preferredHeight: 32
                Layout.preferredWidth: Math.max(takeoffContent.implicitWidth + 22, 92)
                implicitWidth: Layout.preferredWidth
                implicitHeight: 32
                radius: 3

                color: {
                    if (isSending) return Qt.rgba(0.0, 0.784, 0.325, 0.3)
                    if (isAccepted) return "#00C853"
                    if (isRejected) return "#D32F2F"
                    if (isTimeout) return "#FFA000"
                    if (!isVehicleConnected || !isActionAvailable) return "#151C24"
                    return takeoffMouse.containsMouse ? "#00E676" : "#00C853"
                }

                border.color: {
                    if (isSending) return "#00E676"
                    if (isAccepted) return "#00E676"
                    if (isRejected) return "#FF8A80"
                    if (isTimeout) return "#FFE082"
                    return "#2C3847"
                }
                border.width: isActionAvailable ? 0 : 1

                opacity: isActionAvailable || isSending || isAccepted || isRejected || isTimeout ? 1.0 : 0.35

                Row {
                    id: takeoffContent
                    anchors.centerIn: parent
                    spacing: 5

                    Text {
                        text: {
                            if (takeoffBtn.isSending) return "TAKEOFF..."
                            if (takeoffBtn.isAccepted) return "✔ TAKEOFF"
                            if (takeoffBtn.isRejected) return "✖ REJECTED"
                            if (takeoffBtn.isTimeout) return "⏱ TIMEOUT"
                            return "▲ TAKEOFF"
                        }
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.5
                        color: {
                            if (takeoffBtn.isRejected || takeoffBtn.isAccepted || takeoffBtn.isSending) return "#FFFFFF"
                            if (takeoffBtn.isActionAvailable) return "#0A0F14"
                            return "#64748B"
                        }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: takeoffMouse
                    anchors.fill: parent
                    hoverEnabled: takeoffBtn.isActionAvailable && !takeoffBtn.isSending
                    cursorShape: takeoffBtn.isActionAvailable && !takeoffBtn.isSending ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!actionPanelRoot.isVehicleConnected || !guidedController || takeoffBtn.isSending) return
                        if (guidedController.showTakeoff) {
                            guidedController.confirmAction(guidedController.actionTakeoff)
                        }
                    }
                }
            }

            // ----------------------------------------------------------------
            // 3. MISSION / RESUME Action Button
            // ----------------------------------------------------------------
            Rectangle {
                id: missionBtn
                readonly property bool isSending: commandState === "SENDING" && guidedController && (activeActionCode === guidedController.actionStartMission || activeActionCode === guidedController.actionContinueMission)
                readonly property bool isAccepted: commandState === "ACCEPTED" && guidedController && (activeActionCode === guidedController.actionStartMission || activeActionCode === guidedController.actionContinueMission)
                readonly property bool isRejected: (commandState === "REJECTED" || commandState === "FAILED") && guidedController && (activeActionCode === guidedController.actionStartMission || activeActionCode === guidedController.actionContinueMission)
                readonly property bool isTimeout: commandState === "TIMEOUT" && guidedController && (activeActionCode === guidedController.actionStartMission || activeActionCode === guidedController.actionContinueMission)
                readonly property bool isActionAvailable: isMissionAvailable

                Layout.preferredHeight: 32
                Layout.preferredWidth: Math.max(missionContent.implicitWidth + 22, 92)
                implicitWidth: Layout.preferredWidth
                implicitHeight: 32
                radius: 3

                color: {
                    if (isSending) return Qt.rgba(1.0, 0.416, 0.0, 0.3)
                    if (isAccepted) return "#00C853"
                    if (isRejected) return "#D32F2F"
                    if (isTimeout) return "#FFA000"
                    if (!isVehicleConnected || !isActionAvailable) return "#151C24"
                    return missionMouse.containsMouse ? "#2C3847" : "#1D2733"
                }

                border.color: {
                    if (isSending) return "#FF8533"
                    if (isAccepted) return "#00E676"
                    if (isRejected) return "#FF8A80"
                    if (isTimeout) return "#FFE082"
                    return isActionAvailable ? "#FF6A00" : "#2C3847"
                }
                border.width: 1

                opacity: isActionAvailable || isSending || isAccepted || isRejected || isTimeout ? 1.0 : 0.35

                Row {
                    id: missionContent
                    anchors.centerIn: parent
                    spacing: 5

                    Text {
                        text: {
                            if (missionBtn.isSending) return "TRANSMITTING..."
                            if (missionBtn.isAccepted) return "✔ MISSION SET"
                            if (missionBtn.isRejected) return "✖ REJECTED"
                            if (missionBtn.isTimeout) return "⏱ TIMEOUT"
                            return (guidedController && guidedController.showContinueMission) ? "RESUME" : "MISSION"
                        }
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.5
                        color: {
                            if (missionBtn.isTimeout) return "#0A0F14"
                            if (missionBtn.isActionAvailable || missionBtn.isSending || missionBtn.isAccepted || missionBtn.isRejected) return "#F5F7FA"
                            return "#64748B"
                        }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: missionMouse
                    anchors.fill: parent
                    hoverEnabled: missionBtn.isActionAvailable && !missionBtn.isSending
                    cursorShape: missionBtn.isActionAvailable && !missionBtn.isSending ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!actionPanelRoot.isVehicleConnected || !guidedController || missionBtn.isSending) return
                        if (guidedController.showContinueMission) {
                            guidedController.confirmAction(guidedController.actionContinueMission)
                        } else if (guidedController.showStartMission) {
                            guidedController.confirmAction(guidedController.actionStartMission)
                        }
                    }
                }
            }

            // ----------------------------------------------------------------
            // 4. RTL Action Button
            // ----------------------------------------------------------------
            Rectangle {
                id: rtlBtn
                readonly property bool isSending: commandState === "SENDING" && guidedController && activeActionCode === guidedController.actionRTL
                readonly property bool isAccepted: commandState === "ACCEPTED" && guidedController && activeActionCode === guidedController.actionRTL
                readonly property bool isRejected: (commandState === "REJECTED" || commandState === "FAILED") && guidedController && activeActionCode === guidedController.actionRTL
                readonly property bool isTimeout: commandState === "TIMEOUT" && guidedController && activeActionCode === guidedController.actionRTL
                readonly property bool isActionAvailable: isRTLAvailable

                Layout.preferredHeight: 32
                Layout.preferredWidth: Math.max(rtlContent.implicitWidth + 20, 80)
                implicitWidth: Layout.preferredWidth
                implicitHeight: 32
                radius: 3

                color: {
                    if (isSending) return Qt.rgba(1.0, 0.757, 0.027, 0.3)
                    if (isAccepted) return "#00C853"
                    if (isRejected) return "#D32F2F"
                    if (isTimeout) return "#FFA000"
                    if (!isVehicleConnected || !isActionAvailable) return "#151C24"
                    return rtlMouse.containsMouse ? "#FFA000" : "#FFC107"
                }

                border.color: {
                    if (isSending) return "#FFE082"
                    if (isAccepted) return "#00E676"
                    if (isRejected) return "#FF8A80"
                    if (isTimeout) return "#FFE082"
                    return "#2C3847"
                }
                border.width: isActionAvailable ? 0 : 1

                opacity: isActionAvailable || isSending || isAccepted || isRejected || isTimeout ? 1.0 : 0.35

                Row {
                    id: rtlContent
                    anchors.centerIn: parent
                    spacing: 5

                    Text {
                        text: {
                            if (rtlBtn.isSending) return "RTL..."
                            if (rtlBtn.isAccepted) return "✔ RTL SET"
                            if (rtlBtn.isRejected) return "✖ REJECTED"
                            if (rtlBtn.isTimeout) return "⏱ TIMEOUT"
                            return "⮌ RTL"
                        }
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.5
                        color: {
                            if (rtlBtn.isRejected || rtlBtn.isAccepted || rtlBtn.isSending) return "#FFFFFF"
                            if (rtlBtn.isActionAvailable) return "#0A0F14"
                            return "#64748B"
                        }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: rtlMouse
                    anchors.fill: parent
                    hoverEnabled: rtlBtn.isActionAvailable && !rtlBtn.isSending
                    cursorShape: rtlBtn.isActionAvailable && !rtlBtn.isSending ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!actionPanelRoot.isVehicleConnected || !guidedController || rtlBtn.isSending) return
                        if (guidedController.showRTL) {
                            guidedController.confirmAction(guidedController.actionRTL)
                        }
                    }
                }
            }

            // ----------------------------------------------------------------
            // 5. LAND Action Button
            // ----------------------------------------------------------------
            Rectangle {
                id: landBtn
                readonly property bool isSending: commandState === "SENDING" && guidedController && activeActionCode === guidedController.actionLand
                readonly property bool isAccepted: commandState === "ACCEPTED" && guidedController && activeActionCode === guidedController.actionLand
                readonly property bool isRejected: (commandState === "REJECTED" || commandState === "FAILED") && guidedController && activeActionCode === guidedController.actionLand
                readonly property bool isTimeout: commandState === "TIMEOUT" && guidedController && activeActionCode === guidedController.actionLand
                readonly property bool isActionAvailable: isLandAvailable

                Layout.preferredHeight: 32
                Layout.preferredWidth: Math.max(landContent.implicitWidth + 20, 80)
                implicitWidth: Layout.preferredWidth
                implicitHeight: 32
                radius: 3

                color: {
                    if (isSending) return Qt.rgba(0.0, 0.784, 0.325, 0.3)
                    if (isAccepted) return "#00C853"
                    if (isRejected) return "#D32F2F"
                    if (isTimeout) return "#FFA000"
                    if (!isVehicleConnected || !isActionAvailable) return "#151C24"
                    return landMouse.containsMouse ? "#2C3847" : "#1D2733"
                }

                border.color: {
                    if (isSending) return "#00E676"
                    if (isAccepted) return "#00E676"
                    if (isRejected) return "#FF8A80"
                    if (isTimeout) return "#FFE082"
                    return isActionAvailable ? "#00C853" : "#2C3847"
                }
                border.width: 1

                opacity: isActionAvailable || isSending || isAccepted || isRejected || isTimeout ? 1.0 : 0.35

                Row {
                    id: landContent
                    anchors.centerIn: parent
                    spacing: 5

                    Text {
                        text: {
                            if (landBtn.isSending) return "LANDING..."
                            if (landBtn.isAccepted) return "✔ LAND SET"
                            if (landBtn.isRejected) return "✖ REJECTED"
                            if (landBtn.isTimeout) return "⏱ TIMEOUT"
                            return "▼ LAND"
                        }
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.5
                        color: {
                            if (landBtn.isTimeout) return "#0A0F14"
                            if (landBtn.isActionAvailable || landBtn.isSending || landBtn.isAccepted || landBtn.isRejected) return "#F5F7FA"
                            return "#64748B"
                        }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: landMouse
                    anchors.fill: parent
                    hoverEnabled: landBtn.isActionAvailable && !landBtn.isSending
                    cursorShape: landBtn.isActionAvailable && !landBtn.isSending ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!actionPanelRoot.isVehicleConnected || !guidedController || landBtn.isSending) return
                        if (guidedController.showLand) {
                            guidedController.confirmAction(guidedController.actionLand)
                        }
                    }
                }
            }

            // ----------------------------------------------------------------
            // 6. PAUSE / HOLD Action Button
            // ----------------------------------------------------------------
            Rectangle {
                id: pauseBtn
                readonly property bool isSending: commandState === "SENDING" && guidedController && activeActionCode === guidedController.actionPause
                readonly property bool isAccepted: commandState === "ACCEPTED" && guidedController && activeActionCode === guidedController.actionPause
                readonly property bool isRejected: (commandState === "REJECTED" || commandState === "FAILED") && guidedController && activeActionCode === guidedController.actionPause
                readonly property bool isTimeout: commandState === "TIMEOUT" && guidedController && activeActionCode === guidedController.actionPause
                readonly property bool isActionAvailable: isPauseAvailable

                Layout.preferredHeight: 32
                Layout.preferredWidth: Math.max(pauseContent.implicitWidth + 20, 80)
                implicitWidth: Layout.preferredWidth
                implicitHeight: 32
                radius: 3

                color: {
                    if (isSending) return Qt.rgba(1.0, 0.757, 0.027, 0.3)
                    if (isAccepted) return "#00C853"
                    if (isRejected) return "#D32F2F"
                    if (isTimeout) return "#FFA000"
                    if (!isVehicleConnected || !isActionAvailable) return "#151C24"
                    return pauseMouse.containsMouse ? "#2C3847" : "#1D2733"
                }

                border.color: {
                    if (isSending) return "#FFE082"
                    if (isAccepted) return "#00E676"
                    if (isRejected) return "#FF8A80"
                    if (isTimeout) return "#FFE082"
                    return isActionAvailable ? "#FFC107" : "#2C3847"
                }
                border.width: 1

                opacity: isActionAvailable || isSending || isAccepted || isRejected || isTimeout ? 1.0 : 0.35

                Row {
                    id: pauseContent
                    anchors.centerIn: parent
                    spacing: 5

                    Text {
                        text: {
                            if (pauseBtn.isSending) return "PAUSING..."
                            if (pauseBtn.isAccepted) return "✔ PAUSED"
                            if (pauseBtn.isRejected) return "✖ REJECTED"
                            if (pauseBtn.isTimeout) return "⏱ TIMEOUT"
                            return "⏸ PAUSE"
                        }
                        font.family: "Inter"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.5
                        color: {
                            if (pauseBtn.isTimeout) return "#0A0F14"
                            if (pauseBtn.isActionAvailable || pauseBtn.isSending || pauseBtn.isAccepted || pauseBtn.isRejected) return "#F5F7FA"
                            return "#64748B"
                        }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: pauseMouse
                    anchors.fill: parent
                    hoverEnabled: pauseBtn.isActionAvailable && !pauseBtn.isSending
                    cursorShape: pauseBtn.isActionAvailable && !pauseBtn.isSending ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!actionPanelRoot.isVehicleConnected || !guidedController || pauseBtn.isSending) return
                        if (guidedController.showPause) {
                            guidedController.confirmAction(guidedController.actionPause)
                        }
                    }
                }
            }
        }
    }
}
