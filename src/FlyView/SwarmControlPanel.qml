import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

ColumnLayout {
    id: _root

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    _guidedController:  globals.guidedControllerFlyView
    property var    _vehicles:          QGroundControl.multiVehicleManager.vehicles
    property var    _selectedVehicles:  QGroundControl.multiVehicleManager.selectedVehicles
    property int    _selectedCount:     _selectedVehicles ? _selectedVehicles.count : 0
    property bool   _hasSelection:      _selectedCount > 0
    property real   _spacing:           ScreenTools.defaultFontPixelHeight / 2

    // Expose formation settings so other components can read them
    property int    selectedFormation:  0
    property real   formationSpacing:   spacingSlider.value

    spacing: _spacing

    // --- Swarm Status ---
    QGCLabel {
        text:               qsTr("Swarm Control")
        font.pointSize:     ScreenTools.mediumFontPointSize
        font.bold:          true
        Layout.alignment:   Qt.AlignHCenter
    }

    QGCLabel {
        text:               qsTr("%1 of %2 vehicles selected").arg(_selectedCount).arg(_vehicles ? _vehicles.count : 0)
        Layout.alignment:   Qt.AlignHCenter
    }

    // --- Emergency Stop (prominent, always visible) ---
    Rectangle {
        Layout.fillWidth:   true
        height:             emergencyStopBtn.implicitHeight + ScreenTools.defaultFontPixelHeight
        color:              "#40FF0000"
        radius:             ScreenTools.defaultFontPixelHeight / 4
        border.color:       "red"
        border.width:       2

        QGCButton {
            id:                     emergencyStopBtn
            anchors.fill:           parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 4
            text:                   qsTr("EMERGENCY STOP ALL")
            enabled:                _hasSelection && _anyArmed()

            background: Rectangle {
                color:  emergencyStopBtn.enabled ? (emergencyStopBtn.pressed ? "#CC0000" : "red") : qgcPal.windowShade
                radius: ScreenTools.defaultFontPixelHeight / 4
            }

            contentItem: QGCLabel {
                text:                   emergencyStopBtn.text
                font.bold:              true
                font.pointSize:         ScreenTools.mediumFontPointSize
                color:                  emergencyStopBtn.enabled ? "white" : qgcPal.text
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
            }

            onClicked: _guidedController.confirmAction(_guidedController.actionMVEmergencyStop)
        }
    }

    // --- Formation Selector ---
    Rectangle {
        Layout.fillWidth:   true
        height:             formationColumn.implicitHeight + ScreenTools.defaultFontPixelHeight
        color:              qgcPal.windowShade
        radius:             ScreenTools.defaultFontPixelHeight / 4

        ColumnLayout {
            id:                     formationColumn
            anchors.fill:           parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
            spacing:                _spacing

            QGCLabel {
                text:               qsTr("Formation Pattern")
                Layout.alignment:   Qt.AlignHCenter
            }

            RowLayout {
                Layout.alignment:   Qt.AlignHCenter
                spacing:            ScreenTools.defaultFontPixelWidth

                Repeater {
                    model: [
                        { name: qsTr("Line"),    icon: "━━━" },
                        { name: qsTr("V-Shape"), icon: "  V  " },
                        { name: qsTr("Circle"),  icon: "  ◯  " },
                        { name: qsTr("Grid"),    icon: " ⊞ " }
                    ]

                    QGCButton {
                        text:                   modelData.icon + "\n" + modelData.name
                        Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 3
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
                        checked:                selectedFormation === index
                        checkable:              true

                        onClicked: selectedFormation = index
                    }
                }
            }

            RowLayout {
                Layout.alignment:   Qt.AlignHCenter
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Spacing:")
                }

                QGCSlider {
                    id:                     spacingSlider
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 12
                    from:                   2
                    to:                     50
                    value:                  10
                    stepSize:               1
                }

                QGCLabel {
                    text: spacingSlider.value.toFixed(0) + " m"
                }
            }
        }
    }

    // --- Swarm Action Buttons ---
    Rectangle {
        Layout.fillWidth:   true
        height:             actionsColumn.implicitHeight + ScreenTools.defaultFontPixelHeight
        color:              qgcPal.windowShade
        radius:             ScreenTools.defaultFontPixelHeight / 4

        ColumnLayout {
            id:                     actionsColumn
            anchors.fill:           parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
            spacing:                _spacing

            QGCLabel {
                text:               qsTr("Swarm Actions")
                Layout.alignment:   Qt.AlignHCenter
            }

            GridLayout {
                columns:            2
                Layout.alignment:   Qt.AlignHCenter
                rowSpacing:         _spacing
                columnSpacing:      ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:               qsTr("Takeoff All")
                    Layout.fillWidth:   true
                    enabled:            _hasSelection && _anyNotFlying()
                    onClicked:          _guidedController.confirmAction(_guidedController.actionMVTakeoff)
                }

                QGCButton {
                    text:               qsTr("Land All")
                    Layout.fillWidth:   true
                    enabled:            _hasSelection && _anyFlying()
                    onClicked:          _guidedController.confirmAction(_guidedController.actionMVLand)
                }

                QGCButton {
                    text:               qsTr("RTL All")
                    Layout.fillWidth:   true
                    enabled:            _hasSelection && _anyFlying()
                    onClicked:          _guidedController.confirmAction(_guidedController.actionMVRTL)
                }

                QGCButton {
                    text:               qsTr("Pause All")
                    Layout.fillWidth:   true
                    enabled:            _hasSelection && _anyFlying()
                    onClicked:          _guidedController.confirmAction(_guidedController.actionMVPause)
                }

                QGCButton {
                    text:               qsTr("Arm All")
                    Layout.fillWidth:   true
                    enabled:            _hasSelection && _anyDisarmed()
                    onClicked:          _guidedController.confirmAction(_guidedController.actionMVArm)
                }

                QGCButton {
                    text:               qsTr("Disarm All")
                    Layout.fillWidth:   true
                    enabled:            _hasSelection && _anyArmedNotFlying()
                    onClicked:          _guidedController.confirmAction(_guidedController.actionMVDisarm)
                }
            }

            // Follow Leader button - full width
            QGCButton {
                text:               qsTr("Follow Leader (Vehicle %1)").arg(_activeVehicle ? _activeVehicle.id : "-")
                Layout.fillWidth:   true
                enabled:            _hasSelection && _activeVehicle && _anyFlying()
                onClicked:          _guidedController.confirmAction(_guidedController.actionMVFollowLeader)
            }

            QGCButton {
                text:               qsTr("Start Mission All")
                Layout.fillWidth:   true
                enabled:            _hasSelection && _anyArmed()
                onClicked:          _guidedController.confirmAction(_guidedController.actionMVStartMission)
            }
        }
    }

    // --- Altitude Sync Control ---
    Rectangle {
        Layout.fillWidth:   true
        height:             altSyncColumn.implicitHeight + ScreenTools.defaultFontPixelHeight
        color:              qgcPal.windowShade
        radius:             ScreenTools.defaultFontPixelHeight / 4

        ColumnLayout {
            id:                     altSyncColumn
            anchors.fill:           parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
            spacing:                _spacing

            QGCLabel {
                text:               qsTr("Altitude Sync")
                font.bold:          true
                Layout.alignment:   Qt.AlignHCenter
            }

            RowLayout {
                Layout.alignment:   Qt.AlignHCenter
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Target:")
                }

                QGCSlider {
                    id:                     altSyncSlider
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 10
                    from:                   5
                    to:                     200
                    value:                  30
                    stepSize:               1
                }

                QGCLabel {
                    text: altSyncSlider.value.toFixed(0) + " m"
                }
            }

            QGCButton {
                text:               qsTr("Set Altitude for All Selected")
                Layout.fillWidth:   true
                enabled:            _hasSelection && _anyFlying()
                onClicked:          _guidedController.confirmAction(_guidedController.actionMVChangeAlt, altSyncSlider.value)
            }
        }
    }

    // --- Speed Control ---
    Rectangle {
        Layout.fillWidth:   true
        height:             speedColumn.implicitHeight + ScreenTools.defaultFontPixelHeight
        color:              qgcPal.windowShade
        radius:             ScreenTools.defaultFontPixelHeight / 4

        ColumnLayout {
            id:                     speedColumn
            anchors.fill:           parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
            spacing:                _spacing

            QGCLabel {
                text:               qsTr("Speed Control")
                font.bold:          true
                Layout.alignment:   Qt.AlignHCenter
            }

            RowLayout {
                Layout.alignment:   Qt.AlignHCenter
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Max:")
                }

                QGCSlider {
                    id:                     speedSlider
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 10
                    from:                   1
                    to:                     30
                    value:                  5
                    stepSize:               0.5
                }

                QGCLabel {
                    text: speedSlider.value.toFixed(1) + " m/s"
                }
            }

            QGCButton {
                text:               qsTr("Set Speed for All Selected")
                Layout.fillWidth:   true
                enabled:            _hasSelection && _anyFlying()
                onClicked:          _guidedController.confirmAction(_guidedController.actionMVChangeSpeed, speedSlider.value)
            }
        }
    }

    // --- Helper Functions ---
    function _anyFlying() {
        if (!_selectedVehicles) return false
        for (var i = 0; i < _selectedVehicles.count; i++) {
            if (_selectedVehicles.get(i).flying) return true
        }
        return false
    }

    function _anyNotFlying() {
        if (!_selectedVehicles) return false
        for (var i = 0; i < _selectedVehicles.count; i++) {
            if (!_selectedVehicles.get(i).flying) return true
        }
        return false
    }

    function _anyArmed() {
        if (!_selectedVehicles) return false
        for (var i = 0; i < _selectedVehicles.count; i++) {
            if (_selectedVehicles.get(i).armed) return true
        }
        return false
    }

    function _anyDisarmed() {
        if (!_selectedVehicles) return false
        for (var i = 0; i < _selectedVehicles.count; i++) {
            if (!_selectedVehicles.get(i).armed) return true
        }
        return false
    }

    function _anyArmedNotFlying() {
        if (!_selectedVehicles) return false
        for (var i = 0; i < _selectedVehicles.count; i++) {
            var vehicle = _selectedVehicles.get(i)
            if (vehicle.armed && !vehicle.flying) return true
        }
        return false
    }

    // --- Formation Offset Calculator ---
    // Calculates position offsets for each vehicle in the formation relative to the leader
    // Returns an array of {latOffset, lonOffset} in meters
    function calculateFormationOffsets(vehicleCount, formationType, spacing) {
        var offsets = []
        for (var i = 0; i < vehicleCount; i++) {
            var offset = { latOffset: 0, lonOffset: 0 }
            switch (formationType) {
            case 0: // Line
                offset.lonOffset = i * spacing
                break
            case 1: // V-Shape
                if (i > 0) {
                    var side = (i % 2 === 0) ? 1 : -1
                    var row = Math.ceil(i / 2)
                    offset.latOffset = -row * spacing * 0.7071  // cos(45)
                    offset.lonOffset = side * row * spacing * 0.7071  // sin(45)
                }
                break
            case 2: // Circle
                if (i > 0 && vehicleCount > 1) {
                    var angle = (2 * Math.PI * i) / vehicleCount
                    offset.latOffset = spacing * Math.cos(angle)
                    offset.lonOffset = spacing * Math.sin(angle)
                }
                break
            case 3: // Grid
                if (i > 0) {
                    var cols = Math.ceil(Math.sqrt(vehicleCount))
                    offset.latOffset = -Math.floor(i / cols) * spacing
                    offset.lonOffset = (i % cols) * spacing
                }
                break
            }
            offsets.push(offset)
        }
        return offsets
    }
}
