import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.PlanView

Rectangle {
    id: _root

    required property var missionController
    required property var planMasterController

    property var _controllerVehicle: planMasterController.controllerVehicle
    property var _visualItems: missionController.visualItems
    property bool _noMissionItemsAdded: _visualItems ? _visualItems.count <= 1 : true
    property var _settingsItem: _visualItems && _visualItems.count > 0 ? _visualItems.get(0) : null
    property bool _showCruiseSpeed: _controllerVehicle ? !_controllerVehicle.multiRotor : false
    property bool _showHoverSpeed: _controllerVehicle ? (_controllerVehicle.multiRotor || _controllerVehicle.vtol) : false
    property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 16

    width:  parent ? parent.width : 0
    height: mainColumn.height + ScreenTools.defaultFontPixelHeight
    color:  qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: _root.enabled }

    Connections {
        target: _root._controllerVehicle
        function onFirmwareTypeChanged() {
            if (!_root._controllerVehicle.supports.terrainFrame
                    && _root.missionController.globalAltitudeFrame === QGroundControl.AltitudeFrameTerrain) {
                _root.missionController.globalAltitudeFrame = QGroundControl.AltitudeFrameCalcAboveTerrain
            }
        }
    }

    Component { id: altFrameDialogComponent; AltFrameDialog { } }

    QGCPopupDialogFactory {
        id: altFrameDialogFactory
        dialogComponent: altFrameDialogComponent
    }

    ColumnLayout {
        id: mainColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: ScreenTools.defaultFontPixelWidth
        spacing: ScreenTools.defaultFontPixelHeight * 0.5

        LabelledButton {
            Layout.fillWidth: true
            label: qsTr("Alt Frame")
            buttonText: QGroundControl.altitudeFrameExtraUnits(_root.missionController.globalAltitudeFrame)

            onClicked: {
                let removeModes = []
                let updateFunction = function(altFrame) { _root.missionController.globalAltitudeFrame = altFrame }
                if (!_root._controllerVehicle.supports.terrainFrame) {
                    removeModes.push(QGroundControl.AltitudeFrameTerrain)
                }
                if (!_root._noMissionItemsAdded) {
                    if (_root.missionController.globalAltitudeFrame !== QGroundControl.AltitudeFrameRelative) {
                        removeModes.push(QGroundControl.AltitudeFrameRelative)
                    }
                    if (_root.missionController.globalAltitudeFrame !== QGroundControl.AltitudeFrameAbsolute) {
                        removeModes.push(QGroundControl.AltitudeFrameAbsolute)
                    }
                    if (_root.missionController.globalAltitudeFrame !== QGroundControl.AltitudeFrameCalcAboveTerrain) {
                        removeModes.push(QGroundControl.AltitudeFrameCalcAboveTerrain)
                    }
                    if (_root.missionController.globalAltitudeFrame !== QGroundControl.AltitudeFrameTerrain) {
                        removeModes.push(QGroundControl.AltitudeFrameTerrain)
                    }
                }
                altFrameDialogFactory.open({ currentAltFrame: _root.missionController.globalAltitudeFrame, rgRemoveModes: removeModes, updateAltFrameFn: updateFunction })
            }
        }

        FactTextFieldSlider {
            Layout.fillWidth: true
            label: qsTr("Waypoints Altitude")
            fact: QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude
        }

        FactTextFieldSlider {
            Layout.fillWidth: true
            label: qsTr("Flight Speed")
            fact: _root._settingsItem ? _root._settingsItem.speedSection.flightSpeed : null
            showEnableCheckbox: true
            enableCheckBoxChecked: _root._settingsItem ? _root._settingsItem.speedSection.specifyFlightSpeed : false
            visible: _root._settingsItem ? _root._settingsItem.speedSection.available : false

            onEnableCheckboxClicked: {
                if (_root._settingsItem) {
                    _root._settingsItem.speedSection.specifyFlightSpeed = enableCheckBoxChecked
                }
            }
        }

        // ── Vehicle Speeds ──
        SectionHeader {
            id: vehicleSpeedsSectionHeader
            Layout.fillWidth: true
            text: qsTr("Vehicle Speeds")
            visible: _root._showCruiseSpeed || _root._showHoverSpeed
            checked: false
        }

        GridLayout {
            Layout.fillWidth: true
            columnSpacing: ScreenTools.defaultFontPixelWidth
            rowSpacing: columnSpacing
            columns: 2
            visible: vehicleSpeedsSectionHeader.visible && vehicleSpeedsSectionHeader.checked

            QGCLabel {
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pointSize: ScreenTools.smallFontPointSize
                text: qsTr("The following speed values are used to calculate total mission time. They do not affect the flight speed for the mission.")
            }

            QGCLabel {
                text: qsTr("Cruise speed")
                visible: _root._showCruiseSpeed
                Layout.fillWidth: true
            }
            FactTextField {
                fact: QGroundControl.settingsManager.appSettings.offlineEditingCruiseSpeed
                visible: _root._showCruiseSpeed
                Layout.preferredWidth: _root._fieldWidth
            }

            QGCLabel {
                text: qsTr("Hover speed")
                visible: _root._showHoverSpeed
                Layout.fillWidth: true
            }
            FactTextField {
                fact: QGroundControl.settingsManager.appSettings.offlineEditingHoverSpeed
                visible: _root._showHoverSpeed
                Layout.preferredWidth: _root._fieldWidth
            }
        }
    }
}
