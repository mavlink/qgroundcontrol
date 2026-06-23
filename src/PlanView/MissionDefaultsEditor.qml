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
    property bool _flightSpeedSpecified: _settingsItem ? _settingsItem.speedSection.specifyFlightSpeed : false
    property bool _showCruiseSpeed: _controllerVehicle ? !_controllerVehicle.multiRotor : false
    property bool _showHoverSpeed: _controllerVehicle ? (_controllerVehicle.multiRotor || _controllerVehicle.vtol) : false
    property bool _showAscentDescentSpeed: _controllerVehicle ? (_controllerVehicle.multiRotor || _controllerVehicle.vtol) : false
    property bool _isVtol: _controllerVehicle ? _controllerVehicle.vtol : false

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
            text: qsTr("Expected Vehicle Speeds")
            visible: _root._showCruiseSpeed || _root._showHoverSpeed || _root._showAscentDescentSpeed
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight * 0.5
            visible: vehicleSpeedsSectionHeader.visible && vehicleSpeedsSectionHeader.checked

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pointSize: ScreenTools.smallFontPointSize
                text: qsTr("The following speed values are used to calculate total mission time. They do not affect the flight speed for the mission.")
            }

            FactTextFieldSlider {
                Layout.fillWidth: true
                label: _root._isVtol ? qsTr("FW - Flight speed") : qsTr("Flight speed")
                fact: QGroundControl.settingsManager.appSettings.offlineEditingCruiseSpeed
                visible: _root._showCruiseSpeed
                enabled: !_root._flightSpeedSpecified
            }

            FactTextFieldSlider {
                Layout.fillWidth: true
                label: _root._isVtol ? qsTr("MR - Flight speed") : qsTr("Flight speed")
                fact: QGroundControl.settingsManager.appSettings.offlineEditingHoverSpeed
                visible: _root._showHoverSpeed
                enabled: !_root._flightSpeedSpecified
            }

            FactTextFieldSlider {
                Layout.fillWidth: true
                label: _root._isVtol ? qsTr("MR - Ascent speed") : qsTr("Ascent speed")
                fact: QGroundControl.settingsManager.appSettings.offlineEditingAscentSpeed
                visible: _root._showAscentDescentSpeed
            }

            FactTextFieldSlider {
                Layout.fillWidth: true
                label: _root._isVtol ? qsTr("MR - Descent speed") : qsTr("Descent speed")
                fact: QGroundControl.settingsManager.appSettings.offlineEditingDescentSpeed
                visible: _root._showAscentDescentSpeed
            }
        }
    }
}
