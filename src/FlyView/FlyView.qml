import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap
import QGroundControl.Toolbar
// import QGroundControl.Viewer3D   // commented out - remove if not needed

Item {
    id: _root

    readonly property bool _is3DMode: (typeof QGCViewer3DManager !== "undefined") &&
            QGCViewer3DManager.displayMode === QGCViewer3DManager.View3D

    // These should only be used by MainRootWindow
    property var planController: _planController
    property var guidedController: _guidedController

    PlanMasterController {
        id: _planController
        flyView: true
        Component.onCompleted: start()
    }

    property bool _mainWindowIsMap: !QGroundControl.videoManager.hasVideo
    property bool _isFullWindowItemDark: _mainWindowIsMap ? (typeof mapControl !== "undefined" ? mapControl.isSatelliteMap : true) : true
    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _missionController: _planController.missionController
    property var _geoFenceController: _planController.geoFenceController
    property var _rallyPointController: _planController.rallyPointController
    property real _margins: ScreenTools.defaultFontPixelWidth / 2
    property var _guidedController: (typeof guidedActionsController !== "undefined") ? guidedActionsController : null
    property var _guidedValueSlider: (typeof guidedValueSlider !== "undefined") ? guidedValueSlider : null
    property var _widgetLayer: (typeof widgetLayer !== "undefined") ? widgetLayer : null
    property real _toolsMargin: ScreenTools.defaultFontPixelWidth * 0.75
    property rect _centerViewport: Qt.rect(0, 0, width, height)
    property real _rightPanelWidth: ScreenTools.defaultFontPixelWidth * 30
    property var _mapControl: (typeof mapControl !== "undefined") ? mapControl : null
    property var _startRecording: (typeof startRecording !== "undefined") ? startRecording : null
    property var _stopRecording: (typeof stopRecording !== "undefined") ? stopRecording : null
    property var _takeScreenshot: (typeof takeScreenshot !== "undefined") ? takeScreenshot : null

    function _calcCenterViewPort() {
        var newToolInset = Qt.rect(0, 0, width, height)
        if (typeof toolstrip !== "undefined" && toolstrip !== null) {
            toolstrip.adjustToolInset(newToolInset)
        }
    }

    function dropMainStatusIndicatorTool() {
        if (typeof toolbar !== "undefined" && toolbar !== null) {
            toolbar.dropMainStatusIndicatorTool();
        }
    }

    QGCToolInsets {
        id: _toolInsets
        topEdgeLeftInset: toolbar.height
        topEdgeCenterInset: topEdgeLeftInset
        topEdgeRightInset: topEdgeLeftInset
        leftEdgeBottomInset: 0
        bottomEdgeLeftInset: 0
    }

    // ──────────────────────────────────────────────────────────────
    //               MAIN SPLIT-SCREEN LAYOUT
    // ──────────────────────────────────────────────────────────────

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // LEFT HALF: Video feed (or map if no video)
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            FlyViewVideo {
                id: videoControl
                anchors.fill: parent
                visible: QGroundControl.videoManager.hasVideo
            }

            FlyViewMap {
                id: mapControl
                planMasterController: _planController
                rightPanelWidth: 0
                mapName: "FlightDisplayView"
                enabled: !_is3DMode && !QGroundControl.videoManager.hasVideo
                visible: !_is3DMode && !QGroundControl.videoManager.hasVideo
                anchors.fill: parent
            }

            Loader {
                id: viewer3DLoader
                z: 1
                anchors.fill: parent
                active: _is3DMode
                onActiveChanged: {
                    if (active) {
                        setSource("qrc:/qml/QGroundControl/Viewer3D/Models3D/Viewer3DModel.qml")
                    }
                }
            }
        }

        // RIGHT HALF: Controller panel
        Rectangle {
            Layout.preferredWidth: 380
            Layout.fillHeight: true
            color: "#121212"
            border.color: "#333333"
            border.width: 1
            Flickable {
                anchors.fill: parent
                anchors.margins: 16
                contentHeight: rightColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { }

                ColumnLayout {
                    id: rightColumn
                    width: parent.width
                    spacing: 16

                Text {
                    text: "Vehicle Controls"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                QGCButton {
                    text: _activeVehicle ? (_activeVehicle.armed ? "DISARM" : "ARM") : "No Vehicle"
                    enabled: _activeVehicle
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    font.pixelSize: 18
                    background: Rectangle {
                        color: _activeVehicle && _activeVehicle.armed ? "#c0392b" : "#27ae60"
                        radius: 12
                    }
                    onClicked: _activeVehicle.armed ? _activeVehicle.disarm() : _activeVehicle.arm()
                }
                QGCButton {
                    text: QGroundControl.videoManager.recording ? "Stop Recording" : "Start Recording"
                    enabled: QGroundControl.videoManager.hasVideo
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    onClicked: {
                        if (QGroundControl.videoManager.recording) {
                            QGroundControl.videoManager.stopRecording()
                        } else {
                            QGroundControl.videoManager.startRecording()
                        }
                    }
                    background: Rectangle {
                        color: QGroundControl.videoManager.recording ? "#e74c3c" : "#3498db"
                        radius: 8
                    }
                }

                FlightModeMenu {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    currentVehicle: _activeVehicle
                }

                Text {
                    text: "Joystick: " + (QGroundControl.joystickManager.activeJoystick ? "Connected" : "Disconnected")
                    color: QGroundControl.joystickManager.activeJoystick ? "#2ecc71" : "#e74c3c"
                    Layout.alignment: Qt.AlignHCenter
                }

                Loader {
                    id: virtualJoystickLoader
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    active: QGroundControl.settingsManager.appSettings.virtualJoystick.rawValue && _activeVehicle
                    source: "qrc:/qml/QGroundControl/FlyView/VirtualJoystick.qml"
                    property bool autoCenterThrottle: QGroundControl.settingsManager.appSettings.virtualJoystickAutoCenterThrottle.rawValue
                    property bool leftHandedMode: QGroundControl.settingsManager.appSettings.virtualJoystickLeftHandedMode.rawValue
                }

                Text {
                    text: "Guided Actions"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    QGCButton {
                        text: "Takeoff"
                        enabled: _activeVehicle && guidedActionsController.showTakeoff
                        Layout.fillWidth: true
                        onClicked: guidedActionsController.confirmAction(guidedActionsController.actionTakeoff)
                    }

                    QGCButton {
                        text: "Land"
                        enabled: _activeVehicle && guidedActionsController.showLand
                        Layout.fillWidth: true
                        onClicked: guidedActionsController.confirmAction(guidedActionsController.actionLand)
                    }

                    QGCButton {
                        text: "RTL"
                        enabled: _activeVehicle && guidedActionsController.showRTL
                        Layout.fillWidth: true
                        onClicked: guidedActionsController.confirmAction(guidedActionsController.actionRTL)
                    }
                }

                ComboBox {
                    id: altitudeCombo
                    Layout.fillWidth: true
                    model: ["10m", "20m", "50m", "100m"]
                    currentIndex: 0
                    onCurrentTextChanged: {
                        if (_activeVehicle) {
                            var alt = parseFloat(currentText.replace('m', ''))
                            guidedValueSlider.setValue(alt)
                        }
                    }
                }

                QGCButton {
                    text: "Pause"
                    enabled: _activeVehicle && guidedActionsController.showPause
                    Layout.fillWidth: true
                    onClicked: guidedActionsController.confirmAction(guidedActionsController.actionPause)
                }

                QGCButton {
                    text: "EMERGENCY STOP"
                    enabled: _activeVehicle
                    Layout.fillWidth: true
                    background: Rectangle { color: "red" }
                    onClicked: guidedActionsController.confirmAction(guidedActionsController.actionEmergencyStop)
                }

                Text {
                    text: "Camera Controls"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    QGCButton {
                        text: "Screenshot"
                        enabled: QGroundControl.videoManager.hasVideo
                        Layout.fillWidth: true
                        onClicked: QGroundControl.videoManager.grabImage()
                    }
                }

                ComboBox {
                    id: speedCombo
                    Layout.fillWidth: true
                    model: ["1 m/s", "2 m/s", "5 m/s", "10 m/s"]
                    currentIndex: 1
                    onCurrentTextChanged: {
                        if (_activeVehicle) {
                            var speed = parseFloat(currentText.replace(' m/s', ''))
                            // Set max speed or cruise speed, depending on mode
                            // For simplicity, set a parameter if available
                        }
                    }
                }

                Text {
                    text: "Vehicle Status"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                GridLayout {
                    columns: 2
                    Layout.fillWidth: true

                    Text { text: "Battery:"; color: "white" }
                    Text {
                        text: _activeVehicle ? (_activeVehicle.battery.percentRemaining.valueString + "%") : "N/A"
                        color: _activeVehicle && _activeVehicle.battery.percentRemaining.value > 20 ? "green" : "red"
                    }

                    Text { text: "Altitude:"; color: "white" }
                    Text {
                        text: _activeVehicle ? (_activeVehicle.altitudeRelative.valueString + "m") : "N/A"
                        color: "cyan"
                    }

                    Text { text: "Speed:"; color: "white" }
                    Text {
                        text: _activeVehicle ? (_activeVehicle.groundSpeed.valueString + "m/s") : "N/A"
                        color: "yellow"
                    }

                    Text { text: "GPS:"; color: "white" }
                    Text {
                        text: _activeVehicle ? (_activeVehicle.gps.count.value + " sats") : "N/A"
                        color: _activeVehicle && _activeVehicle.gps.count.value > 5 ? "green" : "red"
                    }
                }

                QGCButton {
                    text: "Start Mission"
                    enabled: _activeVehicle && _missionController.readyForMission
                    Layout.fillWidth: true
                    onClicked: _missionController.startMission()
                }

                Text {
                    text: "Advanced Controls"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    QGCButton {
                        text: "Orbit"
                        enabled: _activeVehicle && guidedActionsController.showOrbit
                        Layout.fillWidth: true
                        onClicked: guidedActionsController.confirmAction(guidedActionsController.actionOrbit)
                    }

                    QGCButton {
                        text: "Change Alt"
                        enabled: _activeVehicle && guidedActionsController.showChangeAlt
                        Layout.fillWidth: true
                        onClicked: guidedActionsController.confirmAction(guidedActionsController.actionChangeAlt)
                    }
                }

                Item { Layout.fillHeight: true } // spacer
                }
            }
        }
    }

    // ── Keep original overlays and toolbar on top ────────────────────────────
    FlyViewToolBar {
        id: toolbar
        guidedValueSlider: _guidedValueSlider
        visible: !QGroundControl.videoManager.fullScreen
    }

    FlyViewWidgetLayer {
        id: widgetLayer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: _widgetMargin
        anchors.topMargin: toolbar.height + _widgetMargin
        anchors.rightMargin: 380 + _widgetMargin  // Leave space for right panel
        z: _fullItemZorder + 2
        parentToolInsets: _toolInsets
        mapControl: _mapControl
        visible: !QGroundControl.videoManager.fullScreen && _mainWindowIsMap
    }

    FlyViewCustomLayer {
        id: customOverlay
        anchors.fill: widgetLayer
        z: _fullItemZorder + 2
        parentToolInsets: widgetLayer.totalToolInsets
        mapControl: _mapControl
        visible: !QGroundControl.videoManager.fullScreen && _mainWindowIsMap
    }

    FlyViewInsetViewer {
        id: widgetLayerInsetViewer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: widgetLayer.right
        z: widgetLayer.z + 1
        insetsToView: widgetLayer.totalToolInsets
        visible: false
    }

    GuidedActionsController {
        id: guidedActionsController
        missionController: _missionController
        guidedValueSlider: _guidedValueSlider
    }

    GuidedValueSlider {
        id: guidedValueSlider
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: toolbar.height
        z: QGroundControl.zOrderTopMost
        visible: false
    }
}
