import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    enum MonitorState {
        Idle,
        AwaitingCalibration,
        Running,
        AwaitingStop
    }

    readonly property var digiview: SVState.digiview
    property bool localMonitorActive: false
    property int monitoredCamera: 0
    property int monitoredCommand: DigiviewProtocol.CalibrationCommandStartMag
    property int monitorState: SVCalibrationSettings.MonitorState.Idle
    property int calibrationStatus: DigiviewProtocol.CalibrationStatusNotStarted
    property int completedFaceMask: 0
    property int magnetometerProgress: 0
    property string resultText: ""
    property bool resultSucceeded: false
    property int missedResponsePolls: 0
    // CALIBRATION_PARAMETERS has no request/session ID, so correlation intentionally relies on ordered per-camera replies.
    property bool sessionCommandObserved: false
    property bool stopCommandObserved: false

    readonly property real margin: ScreenTools.defaultFontPixelHeight
    readonly property real spacing: ScreenTools.defaultFontPixelHeight * 0.75
    readonly property int maximumMissedResponsePolls: 10
    readonly property int stopAcknowledgementTimeoutMs: 5000
    readonly property bool stopPending:
        monitorState === SVCalibrationSettings.MonitorState.AwaitingStop
    readonly property bool showingMagnetometer:
        commandSelector.currentValue === DigiviewProtocol.CalibrationCommandStartMag

    function stopMonitoring() {
        calibrationPollTimer.stop()
        stopAcknowledgementTimer.stop()
        localMonitorActive = false
        monitorState = SVCalibrationSettings.MonitorState.Idle
        sessionCommandObserved = false
        stopCommandObserved = false
    }

    function startCalibration() {
        calibrationStatus = DigiviewProtocol.CalibrationStatusNotStarted
        completedFaceMask = 0
        magnetometerProgress = 0
        resultText = ""
        resultSucceeded = false
        missedResponsePolls = 0
        sessionCommandObserved = false
        stopCommandObserved = false

        if (!digiview || !digiview.sessionActive) {
            resultText = qsTr("DigiView is not connected.")
            return
        }

        monitoredCamera = cameraSelector.value
        monitoredCommand = commandSelector.currentValue
        localMonitorActive = true
        monitorState = SVCalibrationSettings.MonitorState.AwaitingCalibration

        if (!digiview.sendCalibrationParameters(monitoredCamera, monitoredCommand)) {
            stopMonitoring()
            resultText = qsTr("Calibration could not be started.")
            return
        }

        digiview.requestCalibrationParameters(monitoredCamera)
        calibrationPollTimer.start()
    }

    function stopCalibration() {
        if (!localMonitorActive || stopPending || !digiview || !digiview.sessionActive) {
            return
        }

        if (!digiview.sendCalibrationParameters(monitoredCamera, DigiviewProtocol.CalibrationCommandStop)) {
            resultSucceeded = false
            resultText = qsTr("Calibration stop request could not be sent.")
            return
        }

        monitorState = SVCalibrationSettings.MonitorState.AwaitingStop
        stopCommandObserved = false
        missedResponsePolls = 0
        resultSucceeded = false
        resultText = qsTr("Stopping calibration. Waiting for DigiView response...")
        stopAcknowledgementTimer.start()
    }

    function handleCalibrationResponse(cameraId, command, status, completedMask, magProgress) {
        if (!localMonitorActive || cameraId !== monitoredCamera) {
            return
        }

        const sessionCommandMatches = command === monitoredCommand
        const isIdleTerminalResponse = command === DigiviewProtocol.CalibrationCommandNone
        if (sessionCommandMatches) {
            sessionCommandObserved = true
            if (!stopPending) {
                monitorState = SVCalibrationSettings.MonitorState.Running
            }
        } else if (stopPending && command === DigiviewProtocol.CalibrationCommandStop) {
            stopCommandObserved = true
        } else if (!isIdleTerminalResponse) {
            return
        }

        const isMagnetometerSession = monitoredCommand === DigiviewProtocol.CalibrationCommandStartMag
        const isSuccessfulTerminal = isMagnetometerSession
            ? status === DigiviewProtocol.CalibrationStatusMagComplete
            : status === DigiviewProtocol.CalibrationStatus6DofComplete
        const isMagnetometerFailure = isMagnetometerSession
            && status === DigiviewProtocol.CalibrationStatusMagFailed
        const isGenericFailure = status === DigiviewProtocol.CalibrationStatusFailed

        if (isIdleTerminalResponse) {
            if ((!sessionCommandObserved && !stopCommandObserved)
                || (!isSuccessfulTerminal && !isMagnetometerFailure && !isGenericFailure)
                || (stopPending && stopCommandObserved && !isGenericFailure)) {
                return
            }
        }

        missedResponsePolls = 0
        calibrationStatus = status
        completedFaceMask = completedMask & 0x3f
        magnetometerProgress = Math.max(0, Math.min(100, magProgress))

        if (!isIdleTerminalResponse) {
            return
        }

        if (isGenericFailure) {
            resultSucceeded = false
            resultText = qsTr("Calibration Failed")
            stopMonitoring()
        } else if (isMagnetometerFailure) {
            resultSucceeded = false
            resultText = qsTr("Magnetometer calibration failed.")
            stopMonitoring()
        } else if (isMagnetometerSession) {
            resultSucceeded = true
            resultText = qsTr("Magnetometer calibration complete.")
            stopMonitoring()
        } else if (isSuccessfulTerminal) {
            resultSucceeded = true
            resultText = qsTr("Gyroscope and accelerometer calibration complete.")
            stopMonitoring()
        }
    }

    Component.onDestruction: stopMonitoring()

    Connections {
        target: root.digiview

        function onCalibrationParametersReceived(cameraId, command, status, completedMask, magProgress) {
            root.handleCalibrationResponse(cameraId, command, status, completedMask, magProgress)
        }

        function onConnectedChanged() {
            if (root.localMonitorActive && (!root.digiview || !root.digiview.sessionActive)) {
                root.stopMonitoring()
                root.resultSucceeded = false
                root.resultText = qsTr("Calibration monitoring stopped because DigiView disconnected.")
            }
        }
    }

    Timer {
        id: calibrationPollTimer

        interval: 500
        repeat: true
        onTriggered: {
            if (!root.digiview || !root.digiview.sessionActive) {
                root.stopMonitoring()
                return
            }

            root.missedResponsePolls++
            if (!root.stopPending && root.missedResponsePolls >= root.maximumMissedResponsePolls) {
                root.stopMonitoring()
                root.resultSucceeded = false
                root.resultText = qsTr("Calibration monitoring stopped: no response for 5 seconds.")
                return
            }

            root.digiview.requestCalibrationParameters(root.monitoredCamera)
        }
    }

    Timer {
        id: stopAcknowledgementTimer

        interval: root.stopAcknowledgementTimeoutMs
        repeat: false
        onTriggered: {
            if (!root.stopPending) {
                return
            }

            root.stopMonitoring()
            root.resultSucceeded = false
            root.resultText = qsTr(
                "DigiView did not acknowledge the stop request. Verify the calibration state before starting again.")
        }
    }

    QGCPalette { id: qgcPalette }

    Rectangle {
        anchors.fill: parent
        radius: ScreenTools.defaultBorderRadius * 2
        color: qgcPalette.window

        Flickable {
            anchors.fill: parent
            anchors.margins: root.margin
            contentWidth: width
            contentHeight: contentColumn.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            ColumnLayout {
                id: contentColumn

                width: parent.width
                spacing: root.spacing

                QGCLabel {
                    text: qsTr("Calibration")
                    font.pointSize: ScreenTools.largeFontPointSize
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: imuContent.implicitHeight + root.margin * 2
                    radius: ScreenTools.defaultBorderRadius * 2
                    color: qgcPalette.windowShade
                    border.width: 1
                    border.color: qgcPalette.windowShadeLight

                    ColumnLayout {
                        id: imuContent

                        anchors.fill: parent
                        anchors.margins: root.margin
                        spacing: root.spacing

                        QGCLabel {
                            text: qsTr("IMU")
                            font.bold: true
                            font.pointSize: ScreenTools.mediumFontPointSize
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: root.spacing
                            rowSpacing: root.spacing

                            QGCLabel { text: qsTr("Camera") }

                            SpinBox {
                                id: cameraSelector

                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                                from: 0
                                to: 5
                                value: 0
                                enabled: !root.localMonitorActive
                            }

                            QGCLabel { text: qsTr("Calibration") }

                            QGCComboBox {
                                id: commandSelector

                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 26
                                textRole: "label"
                                valueRole: "value"
                                enabled: !root.localMonitorActive
                                model: [
                                    { label: qsTr("Magnetometer"), value: DigiviewProtocol.CalibrationCommandStartMag },
                                    { label: qsTr("Gyro + Accelerometer"), value: DigiviewProtocol.CalibrationCommandStart6Dof }
                                ]
                            }
                        }

                        QGCButton {
                            text: root.localMonitorActive ? qsTr("Stop") : qsTr("Start")
                            primary: true
                            enabled: !!root.digiview && root.digiview.sessionActive && !root.stopPending
                            onClicked: root.localMonitorActive ? root.stopCalibration() : root.startCalibration()
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: root.spacing / 2
                            visible: root.showingMagnetometer

                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 24
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 9
                                radius: height / 2
                                color: "transparent"
                                border.width: 2
                                border.color: root.stopPending ? qgcPalette.buttonHighlight
                                    : root.resultText !== ""
                                    ? (root.resultSucceeded ? qgcPalette.colorGreen : qgcPalette.colorRed)
                                    : qgcPalette.buttonHighlight

                                QGCLabel {
                                    anchors.centerIn: parent
                                    text: "∞"
                                    font.pixelSize: parent.height * 0.75
                                    color: parent.border.color
                                }
                            }

                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("Move the camera smoothly in a figure-eight pattern until calibration completes.")
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                            }

                            ProgressBar {
                                Layout.fillWidth: true
                                from: 0
                                to: 100
                                value: root.magnetometerProgress
                            }

                            QGCLabel {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("%1%").arg(root.magnetometerProgress)
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            columnSpacing: root.spacing
                            rowSpacing: root.spacing
                            visible: !root.showingMagnetometer

                             // Mount image mapping: +Z=nose up (VehicleTailDown.png), -Z=nose down (VehicleNoseDown.png), +Y=upside down (VehicleUpsideDown.png), -Y=down (VehicleDown.png), +X=right side (VehicleRight.png), -X=left side (VehicleLeft.png); mask bit ordering remains +X, -X, +Y, -Y, +Z, -Z.
                            Repeater {
                                model: [
                                     { label: "+X", image: "qrc:///qmlimages/VehicleRight.png", bit: 0,
                                      status: DigiviewProtocol.CalibrationStatus6DofXPositive },
                                     { label: "-X", image: "qrc:///qmlimages/VehicleLeft.png", bit: 1,
                                      status: DigiviewProtocol.CalibrationStatus6DofXNegative },
                                     { label: "+Y", image: "qrc:///qmlimages/VehicleUpsideDown.png", bit: 2,
                                      status: DigiviewProtocol.CalibrationStatus6DofYPositive },
                                     { label: "-Y", image: "qrc:///qmlimages/VehicleDown.png", bit: 3,
                                      status: DigiviewProtocol.CalibrationStatus6DofYNegative },
                                     { label: "+Z", image: "qrc:///qmlimages/VehicleTailDown.png", bit: 4,
                                      status: DigiviewProtocol.CalibrationStatus6DofZPositive },
                                     { label: "-Z", image: "qrc:///qmlimages/VehicleNoseDown.png", bit: 5,
                                      status: DigiviewProtocol.CalibrationStatus6DofZNegative }
                                ]

                                delegate: Rectangle {
                                    id: calibrationTile

                                    required property var modelData

                                    readonly property bool faceComplete:
                                        (root.completedFaceMask & (1 << modelData.bit)) !== 0
                                    readonly property bool faceActive: root.calibrationStatus === modelData.status

                                    Layout.fillWidth: true
                                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 10
                                    radius: ScreenTools.defaultBorderRadius
                                    color: qgcPalette.window
                                    border.width: 3
                                    border.color: faceComplete ? qgcPalette.colorGreen
                                        : (faceActive ? qgcPalette.colorRed : qgcPalette.windowShadeLight)

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: root.spacing / 2
                                        spacing: root.spacing / 4

                                        Image {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            source: modelData.image
                                            fillMode: Image.PreserveAspectFit
                                        }

                                        QGCLabel {
                                            Layout.alignment: Qt.AlignHCenter
                                            text: modelData.label
                                            color: calibrationTile.faceComplete ? qgcPalette.colorGreen
                                                : (calibrationTile.faceActive ? qgcPalette.colorRed : qgcPalette.text)
                                            font.bold: calibrationTile.faceComplete || calibrationTile.faceActive
                                        }
                                    }
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            visible: root.resultText !== ""
                            text: root.resultText
                            color: root.stopPending ? qgcPalette.text
                                : (root.resultSucceeded ? qgcPalette.colorGreen : qgcPalette.colorRed)
                            font.bold: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar { }
        }
    }
}
