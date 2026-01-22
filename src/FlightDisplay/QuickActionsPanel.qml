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

/// Quick Actions Panel
Item {
    id: root

    property bool isActiveVehicleOnly: true
    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _multiVehicleManager: QGroundControl.multiVehicleManager
    property var guidedController

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    property real takeoffAltitude: 10    // meters (adjust if needed)

    // Vehicle state
    readonly property bool _vehicleArmed: _activeVehicle ? _activeVehicle.armed : false
    readonly property bool _vehicleFlying: _activeVehicle ? _activeVehicle.flying : false
    readonly property bool _canArm: guidedController ? guidedController.showArm : false
    readonly property bool _canDisarm: guidedController ? guidedController.showDisarm : false
    readonly property bool _canTakeoff: guidedController ? guidedController.showTakeoff : false
    readonly property bool _canLand: guidedController ? guidedController.showLand : false
    readonly property bool _canRTL: guidedController ? guidedController.showRTL : false

    // =====================================================
    // Toast Message
    // =====================================================
    Rectangle {
        id: actionToast
        visible: false
        opacity: 0.0

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 2

        radius: ScreenTools.defaultFontPixelWidth * 0.5
        color: qgcPal.window
        border.color: qgcPal.colorBlue
        border.width: 1

        width: toastText.implicitWidth + ScreenTools.defaultFontPixelWidth * 2
        height: toastText.implicitHeight + ScreenTools.defaultFontPixelHeight

        QGCLabel {
            id: toastText
            anchors.centerIn: parent
            font.pointSize: ScreenTools.smallFontPointSize
        }

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }

        Timer {
            id: toastTimer
            interval: 2000
            onTriggered: actionToast.opacity = 0.0
        }

        onOpacityChanged: {
            if (opacity === 0.0)
                visible = false
        }
    }

    function showActionMessage(text) {
        toastText.text = text
        actionToast.visible = true
        actionToast.opacity = 1.0
        toastTimer.restart()
    }

    // =====================================================
    // Fleet helpers
    // =====================================================
    function executeOnAllVehicles(fn) {
        var vehicles = _multiVehicleManager.vehicles
        for (var i = 0; i < vehicles.count; i++) {
            fn(vehicles.get(i))
        }
    }

    function changeModeActiveVehicle(modeName) {
        if (_activeVehicle && _activeVehicle.flightMode !== modeName) {
            _activeVehicle.flightMode = modeName
            showActionMessage("Mode set to " + modeName)
        }
    }

    function changeModeAllVehicles(modeName) {
        executeOnAllVehicles(function(v) {
            if (v.flightModes.indexOf(modeName) !== -1)
                v.flightMode = modeName
        })
        showActionMessage("Fleet mode set to " + modeName)
    }

    // =====================================================
    // UI
    // =====================================================
    GridLayout {
        anchors.fill: parent
        columns: 2
        rowSpacing: ScreenTools.defaultFontPixelHeight
        columnSpacing: ScreenTools.defaultFontPixelWidth

        // ARM
        QGCButton {
            text: "ARM"
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
            enabled: isActiveVehicleOnly ? _canArm : (_activeVehicle && !_vehicleArmed)

            onClicked: {
                if (isActiveVehicleOnly && _activeVehicle) {
                    _activeVehicle.armed = true
                    showActionMessage("Vehicle ARMED")
                } else {
                    executeOnAllVehicles(function(v) { v.armed = true })
                    showActionMessage("Fleet ARMED")
                }
            }
        }

        // DISARM
        QGCButton {
            text: "DISARM"
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
            enabled: isActiveVehicleOnly ? _canDisarm : (_activeVehicle && _vehicleArmed && !_vehicleFlying)

            onClicked: {
                if (isActiveVehicleOnly && _activeVehicle) {
                    _activeVehicle.armed = false
                    showActionMessage("Vehicle DISARMED")
                } else {
                    executeOnAllVehicles(function(v) { v.armed = false })
                    showActionMessage("Fleet DISARMED")
                }
            }
        }

        //===========================================
        // TAKEOFF Button (FIXED)
        //===========================================
        QGCButton {
            text: "TAKEOFF"
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
            enabled: isActiveVehicleOnly
                     ? (_activeVehicle && _vehicleArmed && !_vehicleFlying)
                     : (_activeVehicle && _vehicleArmed)

            onClicked: {
                if (isActiveVehicleOnly) {
                    if (_activeVehicle && _activeVehicle.guidedModeSupported) {
                        console.log("Takeoff initiated to", takeoffAltitude, "meters")
                        _activeVehicle.guidedModeTakeoff(takeoffAltitude)
                    }
                } else {
                    var vehicles = _multiVehicleManager.vehicles
                    for (var i = 0; i < vehicles.count; i++) {
                        var v = vehicles.get(i)
                        if (v.armed && v.guidedModeSupported) {
                            v.guidedModeTakeoff(takeoffAltitude)
                        }
                    }
                }
            }
        }


        // LAND
        QGCButton {
            text: "LAND"
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
            enabled: _activeVehicle && _vehicleFlying

            onClicked: {
                if (isActiveVehicleOnly && _activeVehicle) {
                    _activeVehicle.guidedModeLand()
                    showActionMessage("Landing initiated")
                } else {
                    executeOnAllVehicles(function(v) {
                        if (v.guidedModeSupported) v.guidedModeLand()
                    })
                    showActionMessage("Fleet landing initiated")
                }
            }
        }

        // RTL
        QGCButton {
            text: "RTL"
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
            enabled: _activeVehicle && _vehicleFlying

            onClicked: {
                if (isActiveVehicleOnly && _activeVehicle) {
                    _activeVehicle.guidedModeRTL(false)
                    showActionMessage("RTL initiated")
                } else {
                    executeOnAllVehicles(function(v) {
                        if (v.guidedModeSupported) v.guidedModeRTL(false)
                    })
                    showActionMessage("Fleet RTL initiated")
                }
            }
        }

        // PAUSE
        QGCButton {
            text: "PAUSE"
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
            enabled: _activeVehicle && _vehicleFlying
            visible: guidedController ? guidedController.showPause : true

            onClicked: {
                if (isActiveVehicleOnly && _activeVehicle) {
                    _activeVehicle.pauseVehicle()
                    showActionMessage("Vehicle paused")
                } else {
                    executeOnAllVehicles(function(v) { v.pauseVehicle() })
                    showActionMessage("Fleet paused")
                }
            }
        }

        // EMERGENCY STOP
        QGCButton {
            text: "EMERGENCY STOP"
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
            enabled: _activeVehicle && _vehicleArmed
            visible: guidedController ? guidedController.showEmergenyStop : true

            palette.buttonText: qgcPal.colorRed

            onClicked: {
                if (_activeVehicle) {
                    _activeVehicle.emergencyStop()
                    showActionMessage("EMERGENCY STOP SENT")
                }
            }
        }

        // FLIGHT MODE
        ColumnLayout {
            Layout.columnSpan: 2
            Layout.fillWidth: true

            QGCLabel {
                text: "FLIGHT MODE"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }

            ComboBox {
                id: modeCombo
                Layout.fillWidth: true
                enabled: _activeVehicle !== null
                model: _activeVehicle ? _activeVehicle.flightModes : []

                onActivated: {
                    var mode = model[index]
                    isActiveVehicleOnly
                        ? changeModeActiveVehicle(mode)
                        : changeModeAllVehicles(mode)
                }
            }
        }

        Connections {
            target: _multiVehicleManager
            function onActiveVehicleChanged(vehicle) {
                if (!vehicle) return
                var idx = vehicle.flightModes.indexOf(vehicle.flightMode)
                if (idx >= 0)
                    modeCombo.currentIndex = idx
            }
        }
    }
}

