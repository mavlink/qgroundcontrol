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
                        checked:                _selectedFormation === index
                        checkable:              true

                        onClicked: _selectedFormation = index
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

    property int _selectedFormation: 0

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
}
