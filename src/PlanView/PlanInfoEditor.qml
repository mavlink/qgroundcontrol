import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Rectangle {
    id: _root

    required property var planMasterController
    required property var missionController
    required property var editorMap

    property var _controllerVehicle: planMasterController.controllerVehicle
    property var _visualItems: missionController.visualItems
    property bool _noMissionItemsAdded: _visualItems ? _visualItems.count <= 1 : true
    property var _settingsItem: _visualItems && _visualItems.count > 0 ? _visualItems.get(0) : null
    property bool _multipleFirmware: !QGroundControl.singleFirmwareSupport
    property bool _multipleVehicleTypes: !QGroundControl.singleVehicleSupport
    property bool _allowFWVehicleTypeSelection: _noMissionItemsAdded && !globals.activeVehicle
    property bool _waypointsOnlyMode: QGroundControl.corePlugin.options.missionWaypointsOnly
    property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 16

    width:  parent ? parent.width : 0
    height: mainColumn.height + ScreenTools.defaultFontPixelHeight
    color:  qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: _root.enabled }

    ColumnLayout {
        id: mainColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: ScreenTools.defaultFontPixelWidth
        spacing: ScreenTools.defaultFontPixelHeight * 0.25

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            QGCLabel {
                text: qsTr("Plan File")
            }

            QGCTextField {
                id: planNameField
                placeholderText: qsTr("Untitled")
                Layout.fillWidth: true

                Component.onCompleted: text = _root.planMasterController.currentPlanFileName

                Connections {
                    target: _root.planMasterController
                    function onCurrentPlanFileNameChanged() {
                        if (!planNameField.activeFocus) {
                            planNameField.text = _root.planMasterController.currentPlanFileName
                        }
                    }
                }

                onEditingFinished: _root.planMasterController.currentPlanFileName = text
            }
        }

        // ── Vehicle Info ──
        SectionHeader {
            id: vehicleInfoSectionHeader
            Layout.fillWidth: true
            text: qsTr("Vehicle Info")
            visible: !_root._waypointsOnlyMode
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth
            visible: vehicleInfoSectionHeader.visible && vehicleInfoSectionHeader.checked

            FactComboBox {
                fact: QGroundControl.settingsManager.appSettings.offlineEditingFirmwareClass
                indexModel: false
                Layout.fillWidth: true
                visible: _root._multipleFirmware && _root._allowFWVehicleTypeSelection
            }
            QGCLabel {
                text: _root._controllerVehicle ? _root._controllerVehicle.firmwareTypeString : ""
                Layout.fillWidth: true
                visible: _root._multipleFirmware && !_root._allowFWVehicleTypeSelection
            }

            FactComboBox {
                fact: QGroundControl.settingsManager.appSettings.offlineEditingVehicleClass
                indexModel: false
                Layout.fillWidth: true
                visible: _root._multipleVehicleTypes && _root._allowFWVehicleTypeSelection
            }
            QGCLabel {
                text: _root._controllerVehicle ? _root._controllerVehicle.vehicleTypeString : ""
                Layout.fillWidth: true
                visible: _root._multipleVehicleTypes && !_root._allowFWVehicleTypeSelection
            }
        }

        // ── Expected Home Position ──
        SectionHeader {
            id: plannedHomePositionSection
            Layout.fillWidth: true
            text: qsTr("Expected Home Position")
        }

        GridLayout {
            Layout.fillWidth: true
            columnSpacing: ScreenTools.defaultFontPixelWidth
            columns: 2
            visible: plannedHomePositionSection.checked

            QGCLabel {
                text: qsTr("Altitude (AMSL)")
            }
            FactTextField {
                fact: _root._settingsItem ? _root._settingsItem.plannedHomePositionAltitude : null
                Layout.fillWidth: true
                visible: _root._settingsItem && _root._settingsItem.terrainQueryFailed
            }
            QGCLabel {
                text: _root._settingsItem ? _root._settingsItem.plannedHomePositionAltitude.valueString + " " + _root._settingsItem.plannedHomePositionAltitude.units : ""
                Layout.fillWidth: true
                visible: !_root._settingsItem || !_root._settingsItem.terrainQueryFailed
            }
        }

        QGCLabel {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
            text: qsTr("Actual position/alt set by vehicle at flight time.")
            horizontalAlignment: Text.AlignHCenter
            visible: plannedHomePositionSection.checked
        }

        QGCButton {
            text: qsTr("Move To Map Center")
            Layout.alignment: Qt.AlignHCenter
            visible: plannedHomePositionSection.checked
            onClicked: {
                if (_root._settingsItem) {
                    _root._settingsItem.coordinate = _root.editorMap.center
                }
            }
        }
    }
}
