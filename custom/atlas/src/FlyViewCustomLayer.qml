import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Item {
    id: root

    property var parentToolInsets
    property var totalToolInsets: _toolInsets
    property var mapControl

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _battery0: _activeVehicle && _activeVehicle.batteries && _activeVehicle.batteries.count > 0
                            ? _activeVehicle.batteries.get(0)
                            : null

    // ---------------------------------------------------------------------
    // LAYOUT
    // ---------------------------------------------------------------------
    readonly property real leftRailExpandedWidth: 94
    readonly property real leftRailCollapsedWidth: 42
    readonly property real leftRailWidth: cameraControlsExpanded ? leftRailExpandedWidth : leftRailCollapsedWidth
    readonly property real rightPanelWidth: Math.min(292, Math.max(268, width * 0.175))
    readonly property real effectiveRightPanelWidth: atlasPayload.aiAvailable ? rightPanelWidth : 0
    readonly property real mapPanelWidth: Math.min(292, Math.max(252, width * 0.175))
    readonly property real bottomDeckLeft: mapPanelWidth + 18

    // ---------------------------------------------------------------------
    // ESTADO LOCAL DA UI
    // ---------------------------------------------------------------------
    property bool gimbalControlEnabled: false
    property bool clickTrackMode: false
    property bool roiMode: false

    property real mouseOriginX: 0
    property real mouseOriginY: 0
    property real yawCommand: 0
    property real pitchCommand: 0
    property real deadzonePixels: 5
    property real sensitivity: 1.0

    property real roiStartX: 0
    property real roiStartY: 0
    property real roiCurrentX: 0
    property real roiCurrentY: 0

    property int recordingSeconds: 0
    property bool _lastRecordingState: false
    property int videoRecordingSeconds: 0
    property bool _lastVideoRecordingState: false
    property bool videoRecordingPending: false

    // Layout modular: cada secção pode ser recolhida e a escolha fica guardada.
    property bool cameraControlsExpanded: QGroundControl.loadBoolGlobalSetting("AtlasVideoControlV08/CameraExpanded", true)
    property bool aiPanelExpanded: QGroundControl.loadBoolGlobalSetting("AtlasVideoControlV08/AiExpanded", true)
    property bool trackingPanelExpanded: QGroundControl.loadBoolGlobalSetting("AtlasVideoControlV08/TrackingExpanded", true)
    property bool gimbalPanelExpanded: QGroundControl.loadBoolGlobalSetting("AtlasVideoControlV08/GimbalExpanded", true)
    property bool videoModeExpanded: QGroundControl.loadBoolGlobalSetting("AtlasVideoControlV08/VideoModeExpanded", true)
    property bool eventPanelExpanded: QGroundControl.loadBoolGlobalSetting("AtlasVideoControlV08/EventExpanded", true)
    property bool telemetryPanelExpanded: QGroundControl.loadBoolGlobalSetting("AtlasVideoControlV08/TelemetryExpanded", true)
    property bool attitudePanelExpanded: QGroundControl.loadBoolGlobalSetting("AtlasVideoControlV08/AttitudeExpanded", true)
    property bool systemPanelExpanded: QGroundControl.loadBoolGlobalSetting("AtlasVideoControlV08/SystemExpanded", true)

    readonly property color panelColor: "#E80A1822"
    readonly property color panelColorSoft: "#D40B1D29"
    readonly property color borderColor: "#2D4C60"
    readonly property color accentColor: "#2B9FFF"
    readonly property color goodColor: "#39D353"
    readonly property color warningColor: "#FFB020"
    readonly property color dangerColor: "#FF3040"
    readonly property color mutedColor: "#7C8A94"
    readonly property color textColor: "#F2F7FA"

    function sendGimbal(yaw, pitch) {
        atlasPayload.gimbalRate(yaw, pitch)
    }

    function stopGimbal() {
        root.yawCommand = 0
        root.pitchCommand = 0
        gimbalSendTimer.stop()
        atlasPayload.gimbalStop()
    }

    function setZoom(value) {
        atlasPayload.zoom(value)
    }

    function videoProfileUiIndex() {
        if (atlasPayload.videoProfile === "ROBUST") {
            return 2
        }
        if (atlasPayload.videoProfile === "BALANCED") {
            return 1
        }
        return 0
    }

    function setVideoProfileFromUi(index) {
        var protocolProfiles = ["LOW", "BALANCED", "ROBUST"]
        if (index >= 0 && index < protocolProfiles.length) {
            atlasPayload.setVideoProfile(protocolProfiles[index])
        }
    }

    function videoProfileLabel() {
        if (atlasPayload.videoProfile === "ROBUST") {
            return "STABLE"
        }
        if (atlasPayload.videoProfile === "LOW") {
            return "FAST"
        }
        return "BALANCED"
    }

    function streamModeLabel() {
        return atlasPayload.streamMode === "PIP" ? "RAW+AI" : atlasPayload.streamMode
    }

    function videoRecordFormatIndex() {
        var value = Number(QGroundControl.settingsManager.videoSettings.recordingFormat.rawValue)
        return value === 2 ? 0 : (value === 1 ? 1 : 2)
    }

    function screenFpsIndex() {
        var fps = atlasScreenRecorder.frameRate
        if (fps === 20) return 0
        if (fps === 30) return 1
        if (fps === 60) return 2
        if (fps === 120) return 3
        if (fps === 144) return 4
        return 2
    }

    function screenResolutionIndex() {
        return atlasScreenRecorder.resolutionMode === "1080P" ? 1
             : (atlasScreenRecorder.resolutionMode === "720P" ? 2 : 0)
    }

    function screenBitrateIndex() {
        var value = atlasScreenRecorder.videoBitRateMbps
        var values = [8, 12, 16, 20, 40, 60]
        for (var i = 0; i < values.length; i++) {
            if (values[i] === value) return i
        }
        return 2
    }

    function setVideoRecordFormat(index) {
        var values = [2, 1, 0] // mp4, mov, mkv
        if (index >= 0 && index < values.length) {
            QGroundControl.settingsManager.videoSettings.recordingFormat.rawValue = values[index]
        }
    }

    function resolutionWidth(label) {
        return label === "720p" ? 1280 : 1920
    }

    function resolutionHeight(label) {
        return label === "720p" ? 720 : 1080
    }

    function formatDuration(seconds) {
        var hours = Math.floor(seconds / 3600)
        var mins = Math.floor((seconds % 3600) / 60)
        var secs = seconds % 60
        return (hours < 10 ? "0" : "") + hours + ":" +
               (mins < 10 ? "0" : "") + mins + ":" +
               (secs < 10 ? "0" : "") + secs
    }

    function factValueWithUnits(fact, fallback) {
        if (!fact) {
            return fallback
        }
        var units = fact.units || ""
        return fact.valueString + (units.length > 0 ? " " + units : "")
    }

    function aiClassEnabled(name) {
        var classes = atlasPayload.aiClasses
        for (var i = 0; i < classes.length; i++) {
            if (classes[i] === name) {
                return true
            }
        }
        return false
    }

    function toggleAiClass(name) {
        var classes = atlasPayload.aiClasses
        var result = []
        var found = false

        for (var i = 0; i < classes.length; i++) {
            if (classes[i] === name) {
                found = true
            } else {
                result.push(classes[i])
            }
        }

        if (!found) {
            result.push(name)
        }

        if (result.length === 0) {
            result.push(name)
        }

        atlasPayload.setAiClasses(result)
    }

    function addLocalEvent(level, message) {
        eventModel.insert(0, {
            "time": Qt.formatTime(new Date(), "hh:mm:ss"),
            "level": level,
            "message": message
        })

        while (eventModel.count > 9) {
            eventModel.remove(eventModel.count - 1)
        }
    }

    function saveLayoutState() {
        QGroundControl.saveBoolGlobalSetting("AtlasVideoControlV08/CameraExpanded", root.cameraControlsExpanded)
        QGroundControl.saveBoolGlobalSetting("AtlasVideoControlV08/AiExpanded", root.aiPanelExpanded)
        QGroundControl.saveBoolGlobalSetting("AtlasVideoControlV08/TrackingExpanded", root.trackingPanelExpanded)
        QGroundControl.saveBoolGlobalSetting("AtlasVideoControlV08/GimbalExpanded", root.gimbalPanelExpanded)
        QGroundControl.saveBoolGlobalSetting("AtlasVideoControlV08/VideoModeExpanded", root.videoModeExpanded)
        QGroundControl.saveBoolGlobalSetting("AtlasVideoControlV08/EventExpanded", root.eventPanelExpanded)
        QGroundControl.saveBoolGlobalSetting("AtlasVideoControlV08/TelemetryExpanded", root.telemetryPanelExpanded)
        QGroundControl.saveBoolGlobalSetting("AtlasVideoControlV08/AttitudeExpanded", root.attitudePanelExpanded)
        QGroundControl.saveBoolGlobalSetting("AtlasVideoControlV08/SystemExpanded", root.systemPanelExpanded)
    }

    function resetLayout() {
        root.cameraControlsExpanded = true
        root.aiPanelExpanded = true
        root.trackingPanelExpanded = true
        root.gimbalPanelExpanded = true
        root.videoModeExpanded = true
        root.eventPanelExpanded = true
        root.telemetryPanelExpanded = true
        root.attitudePanelExpanded = true
        root.systemPanelExpanded = true
        root.saveLayoutState()
        root.addLocalEvent("info", "Layout reset")
    }

    function operatorLayout() {
        root.cameraControlsExpanded = true
        root.aiPanelExpanded = false
        root.trackingPanelExpanded = false
        root.gimbalPanelExpanded = true
        root.videoModeExpanded = true
        root.eventPanelExpanded = false
        root.telemetryPanelExpanded = false
        root.attitudePanelExpanded = false
        root.systemPanelExpanded = false
        root.saveLayoutState()
        root.addLocalEvent("info", "Operator layout")
    }

    function configLayout() {
        root.cameraControlsExpanded = true
        root.aiPanelExpanded = true
        root.trackingPanelExpanded = true
        root.gimbalPanelExpanded = true
        root.videoModeExpanded = true
        root.eventPanelExpanded = true
        root.telemetryPanelExpanded = true
        root.attitudePanelExpanded = true
        root.systemPanelExpanded = true
        root.saveLayoutState()
        root.addLocalEvent("info", "Config layout")
    }

    Component.onCompleted: {
        QGroundControl.saveBoolGlobalSetting("MainFlyWindowIsMap", false)
        addLocalEvent("info", "ATLAS Video Control v0.12.2 ready")
    }

    // ---------------------------------------------------------------------
    // COMPONENTES REUTILIZÁVEIS
    // ---------------------------------------------------------------------

    component AtlasPanel: Rectangle {
        radius: 7
        color: root.panelColor
        border.width: 1
        border.color: root.borderColor
    }

    component AtlasButton: Rectangle {
        id: atlasButton

        property string label: ""
        property string iconText: ""
        property string iconSource: ""
        property bool selected: false
        property color selectedColor: root.accentColor
        property color selectedBorderColor: "#72C4FF"
        signal clicked()

        implicitWidth: 78
        implicitHeight: 40
        radius: 6
        color: !enabled ? "#465057" :
               buttonMouse.pressed ? "#38556A" :
               selected ? selectedColor : "#132B39"
        border.width: 1
        border.color: selected ? selectedBorderColor : "#365367"
        opacity: enabled ? 1.0 : 0.52

        Column {
            anchors.centerIn: parent
            width: parent.width - 8
            spacing: (atlasButton.iconText.length > 0 || atlasButton.iconSource.length > 0) ? 2 : 0

            Item {
                visible: atlasButton.iconText.length > 0 || atlasButton.iconSource.length > 0
                anchors.horizontalCenter: parent.horizontalCenter
                width: 24
                height: 24

                Image {
                    anchors.fill: parent
                    source: atlasButton.iconSource
                    visible: atlasButton.iconSource.length > 0
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }

                QGCLabel {
                    visible: atlasButton.iconSource.length === 0 && atlasButton.iconText.length > 0
                    anchors.centerIn: parent
                    text: atlasButton.iconText
                    color: root.textColor
                    font.pointSize: ScreenTools.defaultFontPointSize * 1.25
                }
            }

            QGCLabel {
                width: parent.width
                text: atlasButton.label
                color: root.textColor
                font.bold: atlasButton.selected
                font.pointSize: ScreenTools.smallFontPointSize * 0.92
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
            }
        }

        MouseArea {
            id: buttonMouse
            anchors.fill: parent
            enabled: atlasButton.enabled
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: atlasButton.clicked()
        }
    }

    component AtlasToggle: Rectangle {
        id: atlasToggle

        property string label: ""
        property bool checked: false
        signal toggled(bool checked)

        implicitHeight: 28
        radius: 4
        color: "transparent"

        Rectangle {
            id: dot
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 10
            height: 10
            radius: 5
            color: atlasToggle.checked ? root.goodColor : root.mutedColor
        }

        QGCLabel {
            anchors.left: dot.right
            anchors.leftMargin: 7
            anchors.verticalCenter: parent.verticalCenter
            text: atlasToggle.label
            color: root.textColor
            font.pointSize: ScreenTools.smallFontPointSize
        }

        Rectangle {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 38
            height: 20
            radius: 10
            color: atlasToggle.checked ? root.accentColor : "#42515B"

            Rectangle {
                width: 16
                height: 16
                radius: 8
                y: 2
                x: atlasToggle.checked ? parent.width - width - 2 : 2
                color: "white"

                Behavior on x {
                    NumberAnimation { duration: 90 }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: atlasToggle.toggled(!atlasToggle.checked)
        }
    }

    component AtlasSectionHeader: Item {
        id: sectionHeader

        property string iconText: ""
        property string iconSource: ""
        property string title: ""
        property bool expanded: true
        signal toggleRequested()

        implicitHeight: 34

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 9
            anchors.verticalCenter: parent.verticalCenter
            spacing: 7

            Item {
                width: 18
                height: 18
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    anchors.fill: parent
                    source: sectionHeader.iconSource
                    visible: sectionHeader.iconSource.length > 0
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                    opacity: 0.95
                }

                QGCLabel {
                    anchors.centerIn: parent
                    visible: sectionHeader.iconSource.length === 0
                    text: sectionHeader.iconText
                    color: root.accentColor
                    font.pointSize: ScreenTools.defaultFontPointSize * 0.92
                    font.bold: true
                }
            }

            QGCLabel {
                anchors.verticalCenter: parent.verticalCenter
                text: sectionHeader.title
                color: "#D7EFFB"
                font.bold: true
                font.pointSize: ScreenTools.defaultFontPointSize * 0.96
            }
        }

        QGCLabel {
            anchors.right: parent.right
            anchors.rightMargin: 9
            anchors.verticalCenter: parent.verticalCenter
            text: sectionHeader.expanded ? "⌃" : "⌄"
            color: "#B9D9E8"
            font.bold: true
            font.pointSize: ScreenTools.defaultFontPointSize * 0.95
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: sectionHeader.toggleRequested()
        }
    }

    component AtlasSlider: Slider {
        id: atlasSlider
        implicitHeight: 24

        background: Rectangle {
            x: atlasSlider.leftPadding
            y: atlasSlider.topPadding + atlasSlider.availableHeight / 2 - height / 2
            width: atlasSlider.availableWidth
            height: 5
            radius: 3
            color: "#29414F"
            border.width: 1
            border.color: "#38586A"

            Rectangle {
                width: atlasSlider.visualPosition * parent.width
                height: parent.height
                radius: parent.radius
                color: root.accentColor
            }
        }

        handle: Rectangle {
            x: atlasSlider.leftPadding + atlasSlider.visualPosition * (atlasSlider.availableWidth - width)
            y: atlasSlider.topPadding + atlasSlider.availableHeight / 2 - height / 2
            width: 17
            height: 17
            radius: width / 2
            color: atlasSlider.pressed ? "#DFF4FF" : "#F6FBFF"
            border.width: 2
            border.color: root.accentColor
        }
    }

    component TelemetryCell: Rectangle {
        property string title: ""
        property string value: "--"
        property color valueColor: root.textColor

        radius: 5
        color: "#720B1C27"
        border.width: 1
        border.color: "#263F50"

        Column {
            anchors.fill: parent
            anchors.margins: 7
            spacing: 2

            QGCLabel {
                width: parent.width
                text: parent.parent.title
                color: "#8FA8B8"
                font.pointSize: ScreenTools.smallFontPointSize * 0.84
                elide: Text.ElideRight
            }

            QGCLabel {
                width: parent.width
                text: parent.parent.value
                color: parent.parent.valueColor
                font.bold: true
                font.pointSize: ScreenTools.defaultFontPointSize * 1.04
                elide: Text.ElideRight
            }
        }
    }

    component SystemCard: Rectangle {
        property string title: ""
        property bool online: false
        property bool warning: false
        property string line1: ""
        property string line2: ""
        property string line3: ""

        radius: 5
        color: "#720B1C27"
        border.width: 1
        border.color: "#263F50"

        Column {
            anchors.fill: parent
            anchors.margins: 7
            spacing: 2

            Row {
                spacing: 5

                Rectangle {
                    width: 9
                    height: 9
                    radius: 5
                    anchors.verticalCenter: parent.verticalCenter
                    color: parent.parent.parent.warning ? root.warningColor :
                           parent.parent.parent.online ? root.goodColor : root.mutedColor
                }

                QGCLabel {
                    text: parent.parent.parent.title
                    color: root.textColor
                    font.bold: true
                    font.pointSize: ScreenTools.smallFontPointSize * 0.88
                }
            }

            QGCLabel {
                width: parent.width
                text: parent.parent.line1
                color: "#C9D7DF"
                font.pointSize: ScreenTools.smallFontPointSize * 0.78
                elide: Text.ElideRight
            }

            QGCLabel {
                width: parent.width
                text: parent.parent.line2
                color: "#C9D7DF"
                font.pointSize: ScreenTools.smallFontPointSize * 0.78
                elide: Text.ElideRight
            }

            QGCLabel {
                width: parent.width
                text: parent.parent.line3
                color: "#C9D7DF"
                font.pointSize: ScreenTools.smallFontPointSize * 0.78
                elide: Text.ElideRight
            }
        }
    }

    // ---------------------------------------------------------------------
    // TIMERS / EVENTOS
    // ---------------------------------------------------------------------

    Timer {
        id: gimbalSendTimer
        interval: 50
        repeat: true
        running: false
        triggeredOnStart: true

        onTriggered: {
            if (!root.gimbalControlEnabled || !atlasPayload.connected || !atlasPayload.a8Connected) {
                root.stopGimbal()
                return
            }

            root.sendGimbal(root.yawCommand, root.pitchCommand)
        }
    }

    Timer {
        id: zoomStopTimer
        interval: 120
        repeat: false
        onTriggered: root.setZoom(0)
    }

    Timer {
        id: recordingTimer
        interval: 1000
        repeat: true
        running: atlasPayload.recording
        onTriggered: root.recordingSeconds++
    }

    Timer {
        id: videoRecordingTimer
        interval: 1000
        repeat: true
        running: QGroundControl.videoManager.recording
        onTriggered: root.videoRecordingSeconds++
    }

    Timer {
        id: videoRecordingPendingTimer
        interval: 3000
        repeat: false
        onTriggered: {
            if (root.videoRecordingPending && !QGroundControl.videoManager.recording) {
                root.videoRecordingPending = false
                root.addLocalEvent("warning", "VIDEO REC did not start - check live stream and Video Save Path")
            }
        }
    }

    ListModel {
        id: eventModel
    }

    Connections {
        target: atlasPayload

        function onEventReceived(level, message, timestampMs) {
            var eventDate = new Date(Number(timestampMs))
            eventModel.insert(0, {
                "time": Qt.formatTime(eventDate, "hh:mm:ss"),
                "level": level,
                "message": message
            })

            while (eventModel.count > 9) {
                eventModel.remove(eventModel.count - 1)
            }
        }

        function onStatusChanged() {
            if (atlasPayload.recording !== root._lastRecordingState) {
                root._lastRecordingState = atlasPayload.recording
                root.recordingSeconds = 0
            }

            if (!atlasPayload.a8Connected && root.gimbalControlEnabled) {
                root.gimbalControlEnabled = false
                root.stopGimbal()
            }

            if (!atlasPayload.trackingModeEnabled) {
                root.clickTrackMode = false
                root.roiMode = false
            }
        }
    }

    Connections {
        target: atlasKml
        function onEventReceived(level, message, timestampMs) {
            root.addLocalEvent(level, message)
        }
    }

    Connections {
        target: QGroundControl.videoManager

        function onRecordingChanged(recording) {
            root.videoRecordingPending = false
            videoRecordingPendingTimer.stop()
            if (recording !== root._lastVideoRecordingState) {
                root._lastVideoRecordingState = recording
                root.videoRecordingSeconds = 0
                root.addLocalEvent("info", recording ? "Local video recording started" : "Local video recording stopped")
            }
        }
    }

    Connections {
        target: atlasScreenRecorder

        function onRecordingStarted(filePath) {
            root.addLocalEvent("info", "Screen recording started")
        }

        function onRecordingStopped(filePath) {
            root.addLocalEvent("info", "Screen recording saved")
        }

        function onRecordingError(message) {
            root.addLocalEvent("error", "Screen REC: " + message)
        }
    }

    // ---------------------------------------------------------------------
    // TOOL INSETS
    // ---------------------------------------------------------------------

    QGCToolInsets {
        id: _toolInsets

        leftEdgeTopInset: Math.max(parentToolInsets.leftEdgeTopInset, root.leftRailWidth + 18)
        leftEdgeCenterInset: Math.max(parentToolInsets.leftEdgeCenterInset, root.leftRailWidth + 18)
        leftEdgeBottomInset: Math.max(parentToolInsets.leftEdgeBottomInset, root.bottomDeckLeft)

        rightEdgeTopInset: Math.max(parentToolInsets.rightEdgeTopInset, root.effectiveRightPanelWidth + (atlasPayload.aiAvailable ? 18 : 0))
        rightEdgeCenterInset: Math.max(parentToolInsets.rightEdgeCenterInset, root.effectiveRightPanelWidth + (atlasPayload.aiAvailable ? 18 : 0))
        rightEdgeBottomInset: Math.max(parentToolInsets.rightEdgeBottomInset, root.effectiveRightPanelWidth + (atlasPayload.aiAvailable ? 18 : 0))

        topEdgeLeftInset: Math.max(parentToolInsets.topEdgeLeftInset, 70)
        topEdgeCenterInset: Math.max(parentToolInsets.topEdgeCenterInset, 102)
        topEdgeRightInset: parentToolInsets.topEdgeRightInset

        bottomEdgeLeftInset: Math.max(parentToolInsets.bottomEdgeLeftInset, bottomDeck.height + 10)
        bottomEdgeCenterInset: Math.max(parentToolInsets.bottomEdgeCenterInset, bottomDeck.height + 10)
        bottomEdgeRightInset: Math.max(parentToolInsets.bottomEdgeRightInset, bottomDeck.height + 10)
    }

    // ---------------------------------------------------------------------
    // LEFT PAYLOAD RAIL
    // Conceito video-focused: apenas controlos essenciais, sem cabeçalho alto.
    // Continua recolhível para o operador libertar ainda mais imagem.
    // ---------------------------------------------------------------------

    AtlasPanel {
        id: leftRail
        z: 50
        anchors.left: parent.left
        anchors.top: videoModePanel.bottom
        anchors.leftMargin: 8
        anchors.topMargin: 8
        width: root.leftRailWidth
        height: root.cameraControlsExpanded ? cameraColumn.implicitHeight + 12 : 38

        Column {
            id: cameraColumn
            anchors.fill: parent
            anchors.margins: 6
            spacing: 7
            visible: root.cameraControlsExpanded

            AtlasButton {
                width: parent.width
                height: 62
                iconSource: "qrc:/atlas/icons/gimbal_control.svg"
                label: root.gimbalControlEnabled ? "GIMBAL ON" : "GIMBAL"
                selected: root.gimbalControlEnabled
                enabled: atlasPayload.a8Connected

                onClicked: {
                    root.gimbalControlEnabled = !root.gimbalControlEnabled
                    root.clickTrackMode = false
                    root.roiMode = false
                    if (!root.gimbalControlEnabled) {
                        root.stopGimbal()
                    }
                }
            }

            AtlasButton {
                width: parent.width
                height: 55
                iconSource: "qrc:/atlas/icons/zoom_plus.svg"
                label: "ZOOM +"
                enabled: atlasPayload.a8Connected
                onClicked: {
                    root.setZoom(1)
                    zoomStopTimer.restart()
                }
            }

            AtlasButton {
                width: parent.width
                height: 55
                iconSource: "qrc:/atlas/icons/zoom_minus.svg"
                label: "ZOOM -"
                enabled: atlasPayload.a8Connected
                onClicked: {
                    root.setZoom(-1)
                    zoomStopTimer.restart()
                }
            }

            AtlasButton {
                width: parent.width
                height: 55
                iconSource: "qrc:/atlas/icons/recenter.svg"
                label: "CENTER"
                enabled: atlasPayload.a8Connected
                onClicked: {
                    root.stopGimbal()
                    atlasPayload.gimbalCenter()
                }
            }

            AtlasButton {
                width: parent.width
                height: 55
                iconSource: "qrc:/atlas/icons/photo.svg"
                label: "PHOTO"
                enabled: atlasPayload.a8Connected
                onClicked: atlasPayload.takePhoto()
            }

            Rectangle {
                width: parent.width
                height: 64
                radius: 6
                color: atlasPayload.recording ? "#7AB02026" : "#132B39"
                border.width: 1
                border.color: atlasPayload.recording ? root.dangerColor : root.borderColor
                opacity: atlasPayload.a8Connected ? 1.0 : 0.52

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    QGCLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "●  REC"
                        color: atlasPayload.recording ? root.dangerColor : root.textColor
                        font.bold: true
                    }

                    QGCLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: atlasPayload.recording ? root.formatDuration(root.recordingSeconds) : "--:--"
                        color: root.textColor
                        font.pointSize: ScreenTools.smallFontPointSize
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: atlasPayload.a8Connected
                    cursorShape: Qt.PointingHandCursor
                    onClicked: atlasPayload.setRecording(!atlasPayload.recording)
                }
            }
        }

        Rectangle {
            z: 3
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 3
            width: root.cameraControlsExpanded ? 18 : parent.width - 6
            height: root.cameraControlsExpanded ? 18 : parent.height - 6
            radius: 4
            color: collapseCameraMouse.containsMouse ? "#365367" : "#172E3B"
            border.width: 1
            border.color: "#45677A"

            QGCLabel {
                anchors.centerIn: parent
                text: root.cameraControlsExpanded ? "‹" : "›"
                color: "#BFE6FF"
                font.bold: true
            }

            MouseArea {
                id: collapseCameraMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.cameraControlsExpanded = !root.cameraControlsExpanded
                    root.saveLayoutState()
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    // TARGET CARD SOBRE O VÍDEO
    // ---------------------------------------------------------------------

    AtlasPanel {
        id: targetCard
        z: 46
        width: 238
        height: 142
        anchors.top: parent.top
        anchors.right: rightPanel.left
        anchors.rightMargin: 10
        anchors.topMargin: 8
        visible: atlasPayload.trackingModeEnabled && atlasPayload.targetValid

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4

            Row {
                width: parent.width
                spacing: 6

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.dangerColor
                }

                QGCLabel {
                    text: "Tracking: " + atlasPayload.targetClass + " #" + atlasPayload.targetId
                    color: root.textColor
                    font.bold: true
                }
            }

            QGCLabel {
                text: "Distance: " + atlasPayload.targetDistanceM.toFixed(1) + " m"
                color: root.textColor
            }

            QGCLabel {
                text: "Bearing: " + atlasPayload.targetBearingDeg.toFixed(0) + "°"
                color: root.textColor
            }

            QGCLabel {
                text: "Lat: " + atlasPayload.targetLatitude.toFixed(6)
                color: "#B8CCD8"
                font.pointSize: ScreenTools.smallFontPointSize * 0.9
            }

            Row {
                width: parent.width

                QGCLabel {
                    width: parent.width - 42
                    text: "Lon: " + atlasPayload.targetLongitude.toFixed(6)
                    color: "#B8CCD8"
                    font.pointSize: ScreenTools.smallFontPointSize * 0.9
                }

                AtlasButton {
                    width: 36
                    height: 30
                    label: "◎"
                    enabled: atlasPayload.a8Connected
                    onClicked: atlasPayload.gimbalPreset("TARGET")
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    // VIDEO INTERACTION AREA
    // ---------------------------------------------------------------------

    MouseArea {
        id: videoInteraction
        z: 30

        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.right: rightPanel.left
        anchors.rightMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.bottom: bottomDeck.top
        anchors.bottomMargin: 8

        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: atlasPayload.trackingModeEnabled && (root.clickTrackMode || root.roiMode) ? Qt.CrossCursor :
                     root.gimbalControlEnabled ? Qt.SizeAllCursor : Qt.ArrowCursor

        onPressed: function(mouse) {
            if (atlasPayload.trackingModeEnabled && root.roiMode) {
                root.roiStartX = mouse.x
                root.roiStartY = mouse.y
                root.roiCurrentX = mouse.x
                root.roiCurrentY = mouse.y
                return
            }

            if (root.gimbalControlEnabled) {
                root.mouseOriginX = mouse.x
                root.mouseOriginY = mouse.y
                root.yawCommand = 0
                root.pitchCommand = 0
                return
            }

            if (!root.clickTrackMode) {
                mouse.accepted = false
            }
        }

        onPositionChanged: function(mouse) {
            if (atlasPayload.trackingModeEnabled && root.roiMode && pressed) {
                root.roiCurrentX = mouse.x
                root.roiCurrentY = mouse.y
                return
            }

            if (!root.gimbalControlEnabled || !pressed) {
                return
            }

            var dx = mouse.x - root.mouseOriginX
            var dy = mouse.y - root.mouseOriginY

            if (Math.abs(dx) < root.deadzonePixels && Math.abs(dy) < root.deadzonePixels) {
                root.yawCommand = 0
                root.pitchCommand = 0
                if (gimbalSendTimer.running) {
                    root.stopGimbal()
                }
                return
            }

            if (Math.abs(dx) > Math.abs(dy)) {
                root.pitchCommand = 0
                root.yawCommand = Math.max(-100, Math.min(100, dx * 200.0 * root.sensitivity / width))
            } else {
                root.yawCommand = 0
                root.pitchCommand = Math.max(-100, Math.min(100, -dy * 200.0 * root.sensitivity / height))
            }

            if (!gimbalSendTimer.running) {
                gimbalSendTimer.start()
            }
        }

        onReleased: function(mouse) {
            if (atlasPayload.trackingModeEnabled && root.roiMode) {
                root.roiCurrentX = mouse.x
                root.roiCurrentY = mouse.y

                var x0 = Math.min(root.roiStartX, root.roiCurrentX)
                var y0 = Math.min(root.roiStartY, root.roiCurrentY)
                var x1 = Math.max(root.roiStartX, root.roiCurrentX)
                var y1 = Math.max(root.roiStartY, root.roiCurrentY)

                if ((x1 - x0) > 10 && (y1 - y0) > 10) {
                    atlasPayload.trackRoi(
                        x0 / width,
                        y0 / height,
                        (x1 - x0) / width,
                        (y1 - y0) / height
                    )
                }

                root.roiMode = false
                return
            }

            if (atlasPayload.trackingModeEnabled && root.clickTrackMode) {
                atlasPayload.trackPoint(mouse.x / width, mouse.y / height)
                root.clickTrackMode = false
                return
            }

            if (root.gimbalControlEnabled) {
                root.stopGimbal()
                return
            }

            mouse.accepted = false
        }

        onCanceled: {
            if (root.gimbalControlEnabled) {
                root.stopGimbal()
            }
            root.roiMode = false
        }

        onDoubleClicked: {
            if (root.gimbalControlEnabled) {
                root.stopGimbal()
                atlasPayload.gimbalCenter()
            }
        }

        onWheel: function(wheel) {
            if (!atlasPayload.a8Connected) {
                wheel.accepted = false
                return
            }

            root.setZoom(wheel.angleDelta.y > 0 ? 1 : -1)
            zoomStopTimer.restart()
            wheel.accepted = true
        }
    }

    Rectangle {
        id: roiRectangle
        z: 35
        visible: atlasPayload.trackingModeEnabled && root.roiMode && videoInteraction.pressed
        color: "#252B9FFF"
        border.width: 2
        border.color: root.accentColor

        x: videoInteraction.x + Math.min(root.roiStartX, root.roiCurrentX)
        y: videoInteraction.y + Math.min(root.roiStartY, root.roiCurrentY)
        width: Math.abs(root.roiCurrentX - root.roiStartX)
        height: Math.abs(root.roiCurrentY - root.roiStartY)
    }

    // ---------------------------------------------------------------------
    // VIDEO SELECTOR SOBRE O VÍDEO
    // Como no conceito: RAW / AI / RAW+AI + perfil, com fullscreen compacto.
    // ---------------------------------------------------------------------

    AtlasPanel {
        id: videoModePanel
        z: 52
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.top: parent.top
        anchors.topMargin: 8
        width: root.videoModeExpanded
               ? (atlasPayload.aiAvailable ? 780 : 463)
                 + (QGroundControl.videoManager.recording ? 18 : 0)
                 + ((atlasScreenRecorder.recording || atlasScreenRecorder.starting) ? 20 : 0)
               : 118
        height: 48

        Row {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 5
            visible: root.videoModeExpanded

            AtlasButton {
                width: 78
                height: parent.height
                label: "RAW"
                selected: atlasPayload.streamMode === "RAW"
                enabled: atlasPayload.connected
                onClicked: atlasPayload.setStreamMode("RAW")
            }

            AtlasButton {
                width: visible ? 78 : 0
                height: parent.height
                visible: atlasPayload.aiAvailable
                label: "AI"
                selected: atlasPayload.streamMode === "AI"
                enabled: atlasPayload.jetsonMode && atlasPayload.connected
                onClicked: atlasPayload.setStreamMode("AI")
            }

            AtlasButton {
                width: visible ? 92 : 0
                height: parent.height
                visible: atlasPayload.aiAvailable
                label: "RAW+AI"
                selected: atlasPayload.streamMode === "PIP"
                enabled: atlasPayload.jetsonMode && atlasPayload.connected
                onClicked: atlasPayload.setStreamMode("PIP")
            }

            ComboBox {
                id: videoProfileCombo
                width: visible ? 150 : 0
                height: parent.height
                visible: atlasPayload.aiAvailable
                model: ["FAST", "BALANCED", "STABLE"]
                currentIndex: root.videoProfileUiIndex()
                enabled: atlasPayload.jetsonMode && atlasPayload.connected
                onActivated: root.setVideoProfileFromUi(currentIndex)

                contentItem: QGCLabel {
                    leftPadding: 14
                    rightPadding: 30
                    verticalAlignment: Text.AlignVCenter
                    text: videoProfileCombo.displayText
                    color: videoProfileCombo.enabled ? "#10283A" : "#6F7D86"
                    font.bold: true
                    font.pointSize: ScreenTools.defaultFontPointSize * 0.92
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    radius: 6
                    color: videoProfileCombo.enabled ? "#EDF4F8" : "#C7D0D5"
                    border.width: 1
                    border.color: videoProfileCombo.activeFocus ? root.accentColor : "#A9BBC6"
                }

                indicator: QGCLabel {
                    x: videoProfileCombo.width - width - 12
                    y: (videoProfileCombo.height - height) / 2 - 1
                    text: "⌄"
                    color: "#193346"
                    font.bold: true
                    font.pointSize: ScreenTools.defaultFontPointSize * 1.05
                }
            }

            AtlasButton {
                width: QGroundControl.videoManager.recording ? 118 : 100
                height: parent.height
                label: QGroundControl.videoManager.recording
                       ? "● VIDEO " + root.formatDuration(root.videoRecordingSeconds)
                       : (root.videoRecordingPending ? "VIDEO..."
                          : (QGroundControl.videoManager.decoding ? "VIDEO REC" : "VIDEO WAIT"))
                selected: QGroundControl.videoManager.recording
                selectedColor: "#8E1C28"
                selectedBorderColor: root.dangerColor
                enabled: QGroundControl.videoManager.recording ||
                         (!root.videoRecordingPending && QGroundControl.videoManager.decoding)
                onClicked: {
                    if (QGroundControl.videoManager.recording) {
                        QGroundControl.videoManager.stopRecording()
                        return
                    }

                    if (!QGroundControl.videoManager.decoding) {
                        root.addLocalEvent("warning", "VIDEO REC unavailable - no decoded video stream")
                        return
                    }

                    root.videoRecordingPending = true
                    videoRecordingPendingTimer.restart()
                    QGroundControl.videoManager.startRecording("")
                }
            }

            AtlasButton {
                width: (atlasScreenRecorder.recording || atlasScreenRecorder.starting) ? 132 : 112
                height: parent.height
                label: atlasScreenRecorder.recording
                       ? "● SCREEN " + root.formatDuration(atlasScreenRecorder.elapsedSeconds)
                       : (atlasScreenRecorder.starting ? "SCREEN..." : "SCREEN REC")
                selected: atlasScreenRecorder.recording
                selectedColor: "#8E1C28"
                selectedBorderColor: root.dangerColor
                onClicked: atlasScreenRecorder.toggleRecording()
            }

            AtlasButton {
                width: 42
                height: parent.height
                iconSource: "qrc:/atlas/icons/settings.svg"
                onClicked: mediaSettingsPopup.open()
            }

            AtlasButton {
                width: 42
                height: parent.height
                iconSource: "qrc:/atlas/icons/fullscreen.svg"
                enabled: QGroundControl.videoManager.hasVideo
                onClicked: QGroundControl.videoManager.fullScreen = true
            }

            // Recolher a barra de modos de vídeo sem perder espaço de imagem.
            Rectangle {
                width: 28
                height: parent.height
                radius: 5
                color: videoCollapseMouse.containsMouse ? "#203B4C" : "#132B39"
                border.width: 1
                border.color: "#365367"

                QGCLabel {
                    anchors.centerIn: parent
                    text: "‹"
                    color: "#D7EFFB"
                    font.bold: true
                    font.pointSize: ScreenTools.defaultFontPointSize * 1.05
                }

                MouseArea {
                    id: videoCollapseMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.videoModeExpanded = false
                        root.saveLayoutState()
                    }
                }
            }
        }

        AtlasSectionHeader {
            anchors.fill: parent
            visible: !root.videoModeExpanded
            iconSource: "qrc:/atlas/icons/video.svg"
            title: "Video"
            expanded: false
            onToggleRequested: { root.videoModeExpanded = true; root.saveLayoutState() }
        }
    }

    Popup {
        id: mediaSettingsPopup
        parent: Overlay.overlay
        x: Math.max(12, (parent.width - width) / 2)
        y: Math.max(12, (parent.height - height) / 2)
        width: Math.min(620, parent.width - 24)
        height: Math.min(760, parent.height - 24)
        modal: false
        focus: true
        padding: 0

        background: Rectangle {
            radius: 12
            color: "#FA091722"
            border.width: 1
            border.color: root.borderColor
        }

        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            RowLayout {
                width: parent.width
                QGCLabel {
                    Layout.fillWidth: true
                    text: "MEDIA & CAMERA SETTINGS"
                    color: root.textColor
                    font.bold: true
                    font.pixelSize: 18
                }
                QGCButton {
                    text: "Close"
                    onClicked: mediaSettingsPopup.close()
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.borderColor }

            Flickable {
                width: parent.width
                height: parent.height - y
                clip: true
                contentWidth: width
                contentHeight: mediaSettingsColumn.implicitHeight
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                Column {
                    id: mediaSettingsColumn
                    width: parent.width - 10
                    spacing: 12

                    QGCLabel {
                        text: "SCREEN RECORDING"
                        color: root.accentColor
                        font.bold: true
                        font.pixelSize: 15
                    }

                    GridLayout {
                        width: parent.width
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8
                        enabled: !atlasScreenRecorder.recording && !atlasScreenRecorder.starting

                        QGCLabel { text: "Frame rate"; color: root.textColor }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [20, 30, 60, 120, 144]
                            currentIndex: root.screenFpsIndex()
                            onActivated: atlasScreenRecorder.frameRate = Number(currentText)
                        }

                        QGCLabel { text: "Resolution"; color: root.textColor }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["NATIVE", "1080P", "720P"]
                            currentIndex: root.screenResolutionIndex()
                            onActivated: atlasScreenRecorder.resolutionMode = currentText
                        }

                        QGCLabel { text: "Bitrate"; color: root.textColor }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [8, 12, 16, 20, 40, 60]
                            currentIndex: root.screenBitrateIndex()
                            onActivated: atlasScreenRecorder.videoBitRateMbps = Number(currentText)
                        }
                    }

                    QGCLabel {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: root.mutedColor
                        text: atlasScreenRecorder.recording
                              ? "Effective capture: " + atlasScreenRecorder.effectiveFps.toFixed(1) + " fps  •  requested " + atlasScreenRecorder.frameRate + " fps"
                              : "60 fps is the default. 120/144 fps are allowed, but effective FPS is shown because Qt grabWindow readback can become the bottleneck."
                    }

                    Rectangle { width: parent.width; height: 1; color: root.borderColor }

                    QGCLabel {
                        text: "VIDEO RECORDING (LOCAL STREAM)"
                        color: root.accentColor
                        font.bold: true
                        font.pixelSize: 15
                    }

                    GridLayout {
                        width: parent.width
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8
                        enabled: !QGroundControl.videoManager.recording

                        QGCLabel { text: "Container"; color: root.textColor }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["MP4", "MOV", "MKV"]
                            currentIndex: root.videoRecordFormatIndex()
                            onActivated: root.setVideoRecordFormat(currentIndex)
                        }

                        QGCLabel { text: "Storage limit"; color: root.textColor }
                        CheckBox {
                            checked: QGroundControl.settingsManager.videoSettings.enableStorageLimit.rawValue
                            text: checked ? "Enabled" : "Disabled"
                            onToggled: QGroundControl.settingsManager.videoSettings.enableStorageLimit.rawValue = checked
                        }

                        QGCLabel { text: "Maximum storage (MB)"; color: root.textColor }
                        SpinBox {
                            from: 100
                            to: 500000
                            stepSize: 500
                            editable: true
                            value: Number(QGroundControl.settingsManager.videoSettings.maxVideoSize.rawValue)
                            onValueModified: QGroundControl.settingsManager.videoSettings.maxVideoSize.rawValue = value
                        }
                    }

                    QGCLabel {
                        width: parent.width
                        wrapMode: Text.WrapAnywhere
                        color: root.mutedColor
                        text: "Save folder: " + QGroundControl.settingsManager.appSettings.videoSavePath +
                              "\nFPS and bitrate are preserved from the received stream; VIDEO REC does not invent extra frames."
                    }

                    Rectangle { width: parent.width; height: 1; color: root.borderColor }

                    QGCLabel {
                        text: "SIYI A8 CAMERA"
                        color: root.accentColor
                        font.bold: true
                        font.pixelSize: 15
                    }

                    QGCLabel {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: atlasPayload.directMode ? root.textColor : root.mutedColor
                        text: atlasPayload.directMode
                              ? "Direct A8 SDK settings. Current FPS is read from the camera; the verified 0x21 write command does not expose an FPS field."
                              : "Switch PAYLOAD to DIRECT A8 to configure the camera directly."
                    }

                    RowLayout {
                        width: parent.width
                        enabled: atlasPayload.directMode && atlasPayload.a8Connected
                        QGCButton {
                            text: "Refresh camera"
                            onClicked: atlasPayload.refreshA8CodecSpecs()
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            color: root.mutedColor
                            text: atlasPayload.a8MainStreamInfoValid
                                  ? "MAIN " + atlasPayload.a8MainStreamWidth + "×" + atlasPayload.a8MainStreamHeight +
                                    "  " + atlasPayload.a8MainStreamCodec + "  " + atlasPayload.a8MainStreamBitrateKbps + " kbps  " + atlasPayload.a8MainStreamFps + " fps"
                                  : "MAIN: waiting for camera"
                        }
                    }

                    GridLayout {
                        width: parent.width
                        columns: 4
                        columnSpacing: 7
                        enabled: atlasPayload.directMode && atlasPayload.a8Connected

                        ComboBox { id: a8MainCodec; model: ["H264", "H265"]; currentIndex: atlasPayload.a8MainStreamCodec === "H264" ? 0 : 1 }
                        ComboBox { id: a8MainResolution; model: ["1080p", "720p"]; currentIndex: atlasPayload.a8MainStreamWidth === 1280 ? 1 : 0 }
                        SpinBox { id: a8MainBitrate; from: 500; to: 30000; stepSize: 500; editable: true; value: atlasPayload.a8MainStreamBitrateKbps > 0 ? atlasPayload.a8MainStreamBitrateKbps : 6000 }
                        QGCButton {
                            text: "Apply MAIN"
                            onClicked: atlasPayload.configureA8MainStream(a8MainCodec.currentText,
                                                                         root.resolutionWidth(a8MainResolution.currentText),
                                                                         root.resolutionHeight(a8MainResolution.currentText),
                                                                         a8MainBitrate.value)
                        }
                    }

                    QGCLabel {
                        width: parent.width
                        color: root.mutedColor
                        text: atlasPayload.a8RecordStreamInfoValid
                              ? "RECORD " + atlasPayload.a8RecordStreamWidth + "×" + atlasPayload.a8RecordStreamHeight +
                                "  " + atlasPayload.a8RecordStreamCodec + "  " + atlasPayload.a8RecordStreamBitrateKbps + " kbps  " + atlasPayload.a8RecordStreamFps + " fps"
                              : "RECORD: waiting for camera"
                    }

                    GridLayout {
                        width: parent.width
                        columns: 3
                        columnSpacing: 7
                        enabled: atlasPayload.directMode && atlasPayload.a8Connected && atlasPayload.a8RecordStreamInfoValid

                        ComboBox { id: a8RecordResolution; model: ["1080p", "720p"]; currentIndex: atlasPayload.a8RecordStreamWidth === 1280 ? 1 : 0 }
                        SpinBox { id: a8RecordBitrate; from: 500; to: 30000; stepSize: 500; editable: true; value: atlasPayload.a8RecordStreamBitrateKbps > 0 ? atlasPayload.a8RecordStreamBitrateKbps : 15000 }
                        QGCButton {
                            text: "Apply REC"
                            onClicked: atlasPayload.configureA8RecordStream(root.resolutionWidth(a8RecordResolution.currentText),
                                                                           root.resolutionHeight(a8RecordResolution.currentText),
                                                                           a8RecordBitrate.value)
                        }
                    }

                    QGCLabel {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: root.warningColor
                        text: "UniGCS exposes A8 recording up to 2K/4K and Boot Record. v0.12.2 does not send guessed 2K/4K/Boot-Record packets: A8 SDK 0x21 documents 720p/1080p writes, and explicitly says the recording-stream codec is not currently writable, so ATLAS preserves the codec reported by the camera."
                    }

                    QGCLabel {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: root.mutedColor
                        text: "Still photo: SIYI publishes 4K JPG for the A8. No separate photo-resolution selector is exposed here until its Ethernet SDK command is verified."
                    }

                    RowLayout {
                        width: parent.width
                        enabled: atlasPayload.directMode && atlasPayload.a8Connected
                        QGCLabel { text: "Gimbal mode"; color: root.textColor }
                        ComboBox { id: a8GimbalMode; model: ["FOLLOW", "LOCK", "FPV"] }
                        QGCButton { text: "Apply"; onClicked: atlasPayload.setA8GimbalWorkingMode(a8GimbalMode.currentText) }
                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        width: parent.width
                        enabled: atlasPayload.directMode && atlasPayload.a8Connected
                        QGCLabel { text: "Video output"; color: root.textColor }
                        ComboBox { id: a8VideoOutput; model: ["ETHERNET", "HDMI", "CVBS"] }
                        QGCButton { text: "Apply"; onClicked: atlasPayload.setA8VideoOutput(a8VideoOutput.currentText) }
                        QGCLabel { text: "restart may be required"; color: root.mutedColor }
                        Item { Layout.fillWidth: true }
                    }

                    QGCLabel {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: root.mutedColor
                        text: "UniGCS also exposes ISO, shutter and white balance. Those controls are intentionally not sent yet because the public A8 external Ethernet SDK command for them has not been verified."
                    }

                    Item { width: 1; height: 12 }
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    // RIGHT SIDEBAR: só AI e, quando explicitamente ativado, Tracking.
    // ---------------------------------------------------------------------

    Item {
        id: rightPanel
        z: 50
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.rightMargin: 8
        width: atlasPayload.aiAvailable ? root.rightPanelWidth : 0
        visible: atlasPayload.aiAvailable
        enabled: visible
        height: visible ? Math.min(rightColumn.implicitHeight, parent.height - bottomDeck.height - 24) : 0

        Flickable {
            id: rightFlick
            anchors.fill: parent
            clip: true
            contentWidth: width
            contentHeight: rightColumn.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            ScrollBar.vertical: ScrollBar {
                policy: rightFlick.contentHeight > rightFlick.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }

            Column {
                id: rightColumn
                width: rightFlick.width - (rightFlick.contentHeight > rightFlick.height ? 8 : 0)
                spacing: 7

                AtlasPanel {
                    id: aiPanel
                    width: parent.width
                    height: root.aiPanelExpanded ? 270 : 36
                    visible: atlasPayload.aiAvailable

                    AtlasSectionHeader {
                        id: aiHeader
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        iconSource: "qrc:/atlas/icons/ai.svg"
                        title: "AI"
                        expanded: root.aiPanelExpanded
                        onToggleRequested: {
                            root.aiPanelExpanded = !root.aiPanelExpanded
                            root.saveLayoutState()
                        }
                    }

                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: aiHeader.bottom
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 9
                        anchors.rightMargin: 9
                        anchors.bottomMargin: 8
                        spacing: 5
                        visible: root.aiPanelExpanded

                        AtlasToggle {
                            width: parent.width
                            label: "YOLO"
                            checked: atlasPayload.aiEnabled
                            onToggled: function(value) { atlasPayload.setAiEnabled(value) }
                        }

                        AtlasToggle {
                            width: parent.width
                            label: "SAHI"
                            checked: atlasPayload.sahiEnabled
                            onToggled: function(value) { atlasPayload.setSahiEnabled(value) }
                        }

                        AtlasToggle {
                            width: parent.width
                            label: "Depth / Distance"
                            checked: atlasPayload.depthEnabled
                            onToggled: function(value) { atlasPayload.setDepthEnabled(value) }
                        }

                        AtlasToggle {
                            width: parent.width
                            label: "Tracking mode"
                            checked: atlasPayload.trackingModeEnabled
                            onToggled: function(value) {
                                root.clickTrackMode = false
                                root.roiMode = false
                                atlasPayload.setTrackingModeEnabled(value)
                            }
                        }

                        Row {
                            width: parent.width
                            height: 30
                            spacing: 5

                            QGCLabel {
                                width: 72
                                text: "Confidence"
                                color: "#B8CCD8"
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            AtlasSlider {
                                id: confidenceSlider
                                width: parent.width - 116
                                anchors.verticalCenter: parent.verticalCenter
                                from: 0.05
                                to: 0.95
                                stepSize: 0.05
                                value: atlasPayload.confidence
                                onPressedChanged: {
                                    if (!pressed) {
                                        atlasPayload.setConfidence(value)
                                    }
                                }
                            }

                            QGCLabel {
                                width: 36
                                text: confidenceSlider.value.toFixed(2)
                                color: root.textColor
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Row {
                            width: parent.width
                            height: 30
                            spacing: 5

                            QGCLabel {
                                width: 72
                                text: "AI FPS"
                                color: "#B8CCD8"
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            AtlasSlider {
                                id: fpsSlider
                                width: parent.width - 116
                                anchors.verticalCenter: parent.verticalCenter
                                from: 1
                                to: 30
                                stepSize: 1
                                value: atlasPayload.aiFpsTarget
                                onPressedChanged: {
                                    if (!pressed) {
                                        atlasPayload.setAiFpsTarget(Math.round(value))
                                    }
                                }
                            }

                            QGCLabel {
                                width: 36
                                text: Math.round(fpsSlider.value).toString()
                                color: root.textColor
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }

                AtlasPanel {
                    id: trackingPanel
                    width: parent.width
                    height: !atlasPayload.trackingModeEnabled ? 0 :
                            root.trackingPanelExpanded ? 122 : 36
                    visible: atlasPayload.trackingModeEnabled

                    AtlasSectionHeader {
                        id: trackingHeader
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        iconSource: "qrc:/atlas/icons/tracking.svg"
                        title: "Tracking"
                        expanded: root.trackingPanelExpanded
                        onToggleRequested: {
                            root.trackingPanelExpanded = !root.trackingPanelExpanded
                            root.saveLayoutState()
                        }
                    }

                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: trackingHeader.bottom
                        anchors.bottom: parent.bottom
                        anchors.margins: 7
                        spacing: 5
                        visible: root.trackingPanelExpanded

                        Row {
                            width: parent.width
                            spacing: 5

                            AtlasButton {
                                width: (parent.width - 15) / 4
                                height: 46
                                iconSource: "qrc:/atlas/icons/track_click.svg"
                                label: "Click"
                                selected: root.clickTrackMode
                                onClicked: {
                                    root.clickTrackMode = !root.clickTrackMode
                                    root.roiMode = false
                                    root.gimbalControlEnabled = false
                                }
                            }

                            AtlasButton {
                                width: (parent.width - 15) / 4
                                height: 46
                                iconSource: "qrc:/atlas/icons/roi.svg"
                                label: "ROI"
                                selected: root.roiMode
                                onClicked: {
                                    root.roiMode = !root.roiMode
                                    root.clickTrackMode = false
                                    root.gimbalControlEnabled = false
                                }
                            }

                            AtlasButton {
                                width: (parent.width - 15) / 4
                                height: 46
                                iconSource: "qrc:/atlas/icons/auto.svg"
                                label: "Auto"
                                selected: atlasPayload.autoGimbal
                                enabled: atlasPayload.targetValid
                                onClicked: atlasPayload.setAutoGimbal(!atlasPayload.autoGimbal)
                            }

                            AtlasButton {
                                width: (parent.width - 15) / 4
                                height: 46
                                iconSource: "qrc:/atlas/icons/clear.svg"
                                label: "Clear"
                                enabled: atlasPayload.tracking || atlasPayload.targetValid
                                onClicked: {
                                    root.clickTrackMode = false
                                    root.roiMode = false
                                    atlasPayload.clearTracking()
                                }
                            }
                        }

                        QGCLabel {
                            width: parent.width
                            text: atlasPayload.targetValid
                                  ? atlasPayload.targetClass + " #" + atlasPayload.targetId +
                                    "  •  " + atlasPayload.targetDistanceM.toFixed(1) + " m  •  " +
                                    atlasPayload.targetConfidence.toFixed(2)
                                  : "No target selected"
                            color: atlasPayload.targetValid ? root.goodColor : "#8FA4B1"
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    // BOTTOM DECK
    // Os cartões recolhidos ficam SEMPRE encostados à base inferior, mesmo
    // quando os cartões vizinhos continuam expandidos.
    // Ordem: Telemetry | Attitude/Compass | Gimbal | Events | System Status.
    // ---------------------------------------------------------------------

    Item {
        id: bottomDeck
        z: 48
        anchors.left: parent.left
        anchors.leftMargin: root.bottomDeckLeft
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8

        property bool anyExpanded: root.telemetryPanelExpanded ||
                                   root.attitudePanelExpanded ||
                                   root.gimbalPanelExpanded ||
                                   root.eventPanelExpanded ||
                                   root.systemPanelExpanded

        height: anyExpanded ? 154 : 36

        RowLayout {
            anchors.fill: parent
            spacing: 6

            AtlasPanel {
                id: telemetryPanel
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: root.telemetryPanelExpanded
                Layout.preferredWidth: root.telemetryPanelExpanded ? 300 : 112
                Layout.minimumWidth: root.telemetryPanelExpanded ? 238 : 112
                Layout.horizontalStretchFactor: 5
                Layout.preferredHeight: root.telemetryPanelExpanded ? bottomDeck.height : 36
                Layout.maximumHeight: Layout.preferredHeight

                AtlasSectionHeader {
                    id: telemetryHeader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    iconSource: "qrc:/atlas/icons/telemetry.svg"
                    title: "Telemetry"
                    expanded: root.telemetryPanelExpanded
                    onToggleRequested: {
                        root.telemetryPanelExpanded = !root.telemetryPanelExpanded
                        root.saveLayoutState()
                    }
                }

                Grid {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: telemetryHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.margins: 6
                    columns: 3
                    rows: 2
                    spacing: 4
                    visible: root.telemetryPanelExpanded

                    property real cellWidth: (width - 8) / 3
                    property real cellHeight: (height - 4) / 2

                    TelemetryCell { width: parent.cellWidth; height: parent.cellHeight; title: "Alt"; value: root._activeVehicle ? root.factValueWithUnits(root._activeVehicle.vehicle.altitudeRelative, "--") : "--" }
                    TelemetryCell { width: parent.cellWidth; height: parent.cellHeight; title: "Speed"; value: root._activeVehicle ? root.factValueWithUnits(root._activeVehicle.vehicle.groundSpeed, "--") : "--" }
                    TelemetryCell { width: parent.cellWidth; height: parent.cellHeight; title: "Heading"; value: root._activeVehicle ? root.factValueWithUnits(root._activeVehicle.vehicle.heading, "--") : "--" }
                    TelemetryCell { width: parent.cellWidth; height: parent.cellHeight; title: "Dist to Home"; value: root._activeVehicle ? root.factValueWithUnits(root._activeVehicle.vehicle.distanceToHome, "--") : "--" }
                    TelemetryCell { width: parent.cellWidth; height: parent.cellHeight; title: "Battery"; value: root._battery0 ? root.factValueWithUnits(root._battery0.voltage, "--") : "--" }
                    TelemetryCell { width: parent.cellWidth; height: parent.cellHeight; title: "GPS"; value: root._activeVehicle ? root._activeVehicle.gps.count.valueString + " SAT" : "--" }
                }
            }

            AtlasPanel {
                id: attitudePanel
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: root.attitudePanelExpanded
                Layout.preferredWidth: root.attitudePanelExpanded ? 225 : 112
                Layout.minimumWidth: root.attitudePanelExpanded ? 198 : 112
                Layout.horizontalStretchFactor: 4
                Layout.preferredHeight: root.attitudePanelExpanded ? bottomDeck.height : 36
                Layout.maximumHeight: Layout.preferredHeight

                AtlasSectionHeader {
                    id: attitudeHeader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    iconSource: "qrc:/atlas/icons/attitude.svg"
                    title: "Attitude / Compass"
                    expanded: root.attitudePanelExpanded
                    onToggleRequested: {
                        root.attitudePanelExpanded = !root.attitudePanelExpanded
                        root.saveLayoutState()
                    }
                }

                Row {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: attitudeHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.margins: 6
                    spacing: 6
                    visible: root.attitudePanelExpanded

                    QGCAttitudeWidget {
                        size: Math.min(parent.height - 2, (parent.width - 6) / 2)
                        anchors.verticalCenter: parent.verticalCenter
                        vehicle: root._activeVehicle
                        showHeading: false
                    }

                    QGCCompassWidget {
                        size: Math.min(parent.height - 2, (parent.width - 6) / 2)
                        anchors.verticalCenter: parent.verticalCenter
                        vehicle: root._activeVehicle
                    }
                }
            }

            // Gimbal: layout compacto e equilibrado como no conceito aprovado.
            AtlasPanel {
                id: gimbalPanel
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: root.gimbalPanelExpanded
                Layout.preferredWidth: root.gimbalPanelExpanded ? 270 : 112
                Layout.minimumWidth: root.gimbalPanelExpanded ? 235 : 112
                Layout.horizontalStretchFactor: 5
                Layout.preferredHeight: root.gimbalPanelExpanded ? bottomDeck.height : 36
                Layout.maximumHeight: Layout.preferredHeight

                AtlasSectionHeader {
                    id: gimbalHeader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    iconSource: "qrc:/atlas/icons/gimbal.svg"
                    title: "Gimbal"
                    expanded: root.gimbalPanelExpanded
                    onToggleRequested: {
                        root.gimbalPanelExpanded = !root.gimbalPanelExpanded
                        root.saveLayoutState()
                    }
                }

                Item {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: gimbalHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.bottomMargin: 6
                    visible: root.gimbalPanelExpanded

                    Row {
                        id: gimbalTopRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: 54
                        spacing: 8

                        Column {
                            width: parent.width - gimbalDial.width - 10
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 5

                            Row {
                                width: parent.width
                                QGCLabel { width: 44; text: "Yaw"; color: "#9CB5C4"; font.pointSize: ScreenTools.smallFontPointSize * 0.78 }
                                QGCLabel { text: (atlasPayload.gimbalYawDeg >= 0 ? "+" : "") + atlasPayload.gimbalYawDeg.toFixed(1) + "°"; color: root.textColor; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize * 0.92 }
                            }
                            Row {
                                width: parent.width
                                QGCLabel { width: 44; text: "Pitch"; color: "#9CB5C4"; font.pointSize: ScreenTools.smallFontPointSize * 0.78 }
                                QGCLabel { text: (atlasPayload.gimbalPitchDeg >= 0 ? "+" : "") + atlasPayload.gimbalPitchDeg.toFixed(1) + "°"; color: root.textColor; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize * 0.92 }
                            }
                        }

                        Rectangle {
                            id: gimbalDial
                            width: 58
                            height: 58
                            radius: width / 2
                            color: "#43102735"
                            border.width: 1
                            border.color: root.accentColor

                            Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 1; height: parent.height - 10; color: "#5C8296" }
                            Rectangle { anchors.verticalCenter: parent.verticalCenter; height: 1; width: parent.width - 10; color: "#5C8296" }
                            Image { anchors.centerIn: parent; width: 24; height: 24; source: "qrc:/atlas/icons/gimbal.svg"; fillMode: Image.PreserveAspectFit }
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: "#F5FBFF"; border.width: 2; border.color: root.accentColor
                                x: Math.max(3, Math.min(parent.width - width - 3, parent.width / 2 + (atlasPayload.gimbalYawDeg / 90.0) * (parent.width / 2 - 8) - width / 2))
                                y: Math.max(3, Math.min(parent.height - height - 3, parent.height / 2 + (atlasPayload.gimbalPitchDeg / 90.0) * (parent.height / 2 - 8) - height / 2))
                            }
                        }
                    }

                    Row {
                        id: sensitivityRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: gimbalTopRow.bottom
                        anchors.topMargin: 1
                        height: 24
                        spacing: 5

                        QGCLabel {
                            width: 58
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Sensitivity"
                            color: "#9CB5C4"
                            font.pointSize: ScreenTools.smallFontPointSize * 0.72
                        }
                        AtlasSlider {
                            id: gimbalSensitivitySlider
                            width: parent.width - 100
                            anchors.verticalCenter: parent.verticalCenter
                            from: 0.25; to: 2.50; stepSize: 0.05
                            value: root.sensitivity
                            onMoved: root.sensitivity = value
                        }
                        QGCLabel {
                            width: 32
                            anchors.verticalCenter: parent.verticalCenter
                            text: gimbalSensitivitySlider.value.toFixed(1) + "x"
                            color: root.textColor
                            horizontalAlignment: Text.AlignRight
                            font.pointSize: ScreenTools.smallFontPointSize * 0.72
                        }
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: sensitivityRow.bottom
                        anchors.topMargin: 1
                        height: 25
                        spacing: 3
                        Repeater {
                            model: ["FWD", "DOWN", "LEFT", "RIGHT", "TGT"]
                            AtlasButton {
                                width: (parent.width - 12) / 5
                                height: 25
                                label: modelData
                                enabled: atlasPayload.a8Connected && (modelData !== "TGT" || (atlasPayload.trackingModeEnabled && atlasPayload.targetValid))
                                onClicked: atlasPayload.gimbalPreset(modelData === "FWD" ? "FORWARD" : modelData === "TGT" ? "TARGET" : modelData)
                            }
                        }
                    }
                }
            }

            AtlasPanel {
                id: eventPanel
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: root.eventPanelExpanded
                Layout.preferredWidth: root.eventPanelExpanded ? 220 : 112
                Layout.minimumWidth: root.eventPanelExpanded ? 182 : 112
                Layout.horizontalStretchFactor: 4
                Layout.preferredHeight: root.eventPanelExpanded ? bottomDeck.height : 36
                Layout.maximumHeight: Layout.preferredHeight

                AtlasSectionHeader {
                    id: eventHeader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    iconSource: "qrc:/atlas/icons/events.svg"
                    title: "Events (" + eventModel.count + ")"
                    expanded: root.eventPanelExpanded
                    onToggleRequested: {
                        root.eventPanelExpanded = !root.eventPanelExpanded
                        root.saveLayoutState()
                    }
                }

                Item {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: eventHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.margins: 6
                    visible: root.eventPanelExpanded

                    ListView {
                        anchors.fill: parent
                        clip: true
                        model: eventModel
                        spacing: 2

                        delegate: Row {
                            width: ListView.view.width
                            spacing: 4
                            Rectangle {
                                width: 7; height: 7; radius: 4
                                anchors.verticalCenter: parent.verticalCenter
                                color: level === "warning" || level === "error" ? root.warningColor : root.goodColor
                            }
                            QGCLabel { width: 48; text: time; color: "#8FA6B5"; font.pointSize: ScreenTools.smallFontPointSize * 0.84 }
                            QGCLabel { width: parent.width - 63; text: message; color: "#D5E1E7"; font.pointSize: ScreenTools.smallFontPointSize * 0.86; elide: Text.ElideRight }
                        }
                    }
                }
            }

            AtlasPanel {
                id: systemPanel
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: root.systemPanelExpanded
                Layout.preferredWidth: root.systemPanelExpanded ? 285 : 112
                Layout.minimumWidth: root.systemPanelExpanded ? 255 : 112
                Layout.horizontalStretchFactor: 4
                Layout.preferredHeight: root.systemPanelExpanded ? bottomDeck.height : 36
                Layout.maximumHeight: Layout.preferredHeight

                AtlasSectionHeader {
                    id: systemHeader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    iconSource: "qrc:/atlas/icons/system.svg"
                    title: "System Status"
                    expanded: root.systemPanelExpanded
                    onToggleRequested: {
                        root.systemPanelExpanded = !root.systemPanelExpanded
                        root.saveLayoutState()
                    }
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: systemHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 9
                    anchors.rightMargin: 8
                    anchors.topMargin: 2
                    anchors.bottomMargin: 6
                    spacing: 5
                    visible: root.systemPanelExpanded

                    Row {
                        width: parent.width; height: 25; spacing: 7
                        Rectangle {
                            width: 10; height: 10; radius: 5; anchors.verticalCenter: parent.verticalCenter
                            color: atlasPayload.directMode ? "#46A8E5" : (atlasPayload.connected ? root.goodColor : root.mutedColor)
                        }
                        QGCLabel { width: 50; anchors.verticalCenter: parent.verticalCenter; text: "Jetson"; color: root.textColor; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize * 0.92 }
                        QGCLabel {
                            width: parent.width - 67
                            anchors.verticalCenter: parent.verticalCenter
                            text: atlasPayload.directMode
                                  ? "BYPASSED — Direct A8 route"
                                  : (atlasPayload.connected
                                     ? "CPU " + atlasPayload.jetsonCpuPercent.toFixed(0) + "%  |  GPU " + atlasPayload.jetsonGpuPercent.toFixed(0) + "%  |  " + atlasPayload.jetsonTempC.toFixed(0) + "°C"
                                     : "OFFLINE")
                            color: atlasPayload.directMode ? "#A9D8F3" : "#D6E3EA"
                            font.pointSize: ScreenTools.smallFontPointSize * 0.86
                            elide: Text.ElideRight
                        }
                    }
                    Row {
                        width: parent.width; height: 25; spacing: 7
                        Rectangle {
                            width: 10; height: 10; radius: 5; anchors.verticalCenter: parent.verticalCenter
                            color: atlasPayload.directMode
                                   ? (atlasPayload.a8Connected ? root.goodColor : root.mutedColor)
                                   : (atlasPayload.hm30Connected ? root.goodColor : root.mutedColor)
                        }
                        QGCLabel { width: 50; anchors.verticalCenter: parent.verticalCenter; text: "HM30"; color: root.textColor; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize * 0.92 }
                        QGCLabel {
                            width: parent.width - 67
                            anchors.verticalCenter: parent.verticalCenter
                            text: atlasPayload.directMode
                                  ? (atlasPayload.a8Connected ? "IP path OK  |  RF stats unavailable" : "NO A8 LINK  |  RF stats unavailable")
                                  : (atlasPayload.hm30Connected
                                     ? atlasPayload.hm30RssiDbm.toFixed(0) + " dBm  |  Loss " + atlasPayload.hm30PacketLossPercent.toFixed(1) + "%  |  " + atlasPayload.hm30LatencyMs.toFixed(0) + " ms"
                                     : "OFFLINE")
                            color: "#D6E3EA"
                            font.pointSize: ScreenTools.smallFontPointSize * 0.86
                            elide: Text.ElideRight
                        }
                    }
                    Row {
                        width: parent.width; height: 25; spacing: 7
                        Rectangle { width: 10; height: 10; radius: 5; anchors.verticalCenter: parent.verticalCenter; color: atlasPayload.a8Connected ? root.goodColor : root.mutedColor }
                        QGCLabel { width: 50; anchors.verticalCenter: parent.verticalCenter; text: "A8"; color: root.textColor; font.bold: true; font.pointSize: ScreenTools.smallFontPointSize * 0.92 }
                        QGCLabel {
                            width: parent.width - 67
                            anchors.verticalCenter: parent.verticalCenter
                            text: atlasPayload.a8Connected
                                  ? (atlasPayload.directMode ? "Direct  |  " : "Ready  |  ") + atlasPayload.zoomLevel.toFixed(1) + "x  |  Y " + atlasPayload.gimbalYawDeg.toFixed(0) + "°  P " + atlasPayload.gimbalPitchDeg.toFixed(0) + "°"
                                  : "OFFLINE"
                            color: "#D6E3EA"
                            font.pointSize: ScreenTools.smallFontPointSize * 0.86
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }

}
