import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Plan tab: plan info, templates for a new plan, mission defaults and tools
QGCFlickable {
    id: root

    required property var planMasterController
    required property var editorMap

    signal missionTabRequested

    readonly property var   _missionController:     planMasterController.missionController
    readonly property var   _visualItems:           _missionController.visualItems
    readonly property var   _settingsItem:          _visualItems && _visualItems.count > 0 ? _visualItems.get(0) : null
    readonly property var   _controllerVehicle:     planMasterController.controllerVehicle
    readonly property bool  _noMissionItemsAdded:   _visualItems ? _visualItems.count <= 1 : true
    readonly property bool  _waypointsOnlyMode:     QGroundControl.corePlugin.options.missionWaypointsOnly
    readonly property bool  _multipleFirmware:      !QGroundControl.singleFirmwareSupport
    readonly property bool  _multipleVehicleTypes:  !QGroundControl.singleVehicleSupport
    readonly property bool  _allowVehicleSelection: _noMissionItemsAdded && !globals.activeVehicle
    readonly property real  _margin:                ScreenTools.defaultFontPixelWidth
    readonly property real  _labelWidth:            ScreenTools.defaultFontPixelWidth * 12

    objectName:         "planPanel_page_plan"
    contentHeight:      column.height + _margin * 2
    flickableDirection: Flickable.VerticalFlick
    clip:               true

    QGCPalette { id: qgcPal }

    QGCPopupDialogFactory {
        id: transformDialogFactory

        dialogComponent: transformDialogComponent
    }

    Component {
        id: transformDialogComponent

        QGCPopupDialog {
            title:      qsTr("Transform Mission")
            buttons:    Dialog.Close

            TransformEditor {
                missionController:  root._missionController
                width:              ScreenTools.defaultFontPixelWidth * 50
            }
        }
    }

    ColumnLayout {
        id: column

        x:          root._margin
        y:          root._margin
        width:      root.width - root._margin * 2
        spacing:    ScreenTools.defaultFontPixelHeight * 0.5

        PlanPanelGroupHeader {
            text: qsTr("Plan")
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            root._margin

            QGCLabel {
                Layout.preferredWidth:  root._labelWidth
                text:                   qsTr("File")
                color:                  qgcPal.textSecondary
            }

            QGCLabel {
                objectName:         "planPanel_planFileName"
                Layout.fillWidth:   true
                text:               root.planMasterController.currentPlanFileName === "" ? qsTr("<Untitled>") : root.planMasterController.currentPlanFileName
                textFormat:         Text.PlainText
                elide:              Text.ElideMiddle
            }
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            root._margin
            visible:            !root._waypointsOnlyMode && root._multipleFirmware

            QGCLabel {
                Layout.preferredWidth:  root._labelWidth
                text:                   qsTr("Firmware")
                color:                  qgcPal.textSecondary
            }

            FactComboBox {
                objectName:         "planPanel_firmwareCombo"
                Layout.fillWidth:   true
                fact:               QGroundControl.settingsManager.appSettings.offlineEditingFirmwareClass
                indexModel:         false
                visible:            root._allowVehicleSelection
            }

            QGCLabel {
                objectName:         "planPanel_firmwareLabel"
                Layout.fillWidth:   true
                text:               root._controllerVehicle ? root._controllerVehicle.firmwareTypeString : ""
                visible:            !root._allowVehicleSelection
            }
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            root._margin
            visible:            !root._waypointsOnlyMode && root._multipleVehicleTypes

            QGCLabel {
                Layout.preferredWidth:  root._labelWidth
                text:                   qsTr("Vehicle")
                color:                  qgcPal.textSecondary
            }

            FactComboBox {
                objectName:         "planPanel_vehicleTypeCombo"
                Layout.fillWidth:   true
                fact:               QGroundControl.settingsManager.appSettings.offlineEditingVehicleClass
                indexModel:         false
                visible:            root._allowVehicleSelection
            }

            QGCLabel {
                objectName:         "planPanel_vehicleTypeLabel"
                Layout.fillWidth:   true
                text:               root._controllerVehicle ? root._controllerVehicle.vehicleTypeString : ""
                visible:            !root._allowVehicleSelection
            }
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            root._margin

            QGCLabel {
                Layout.preferredWidth:  root._labelWidth
                text:                   qsTr("Home")
                color:                  qgcPal.textSecondary
            }

            Rectangle {
                Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 0.5
                Layout.preferredHeight: Layout.preferredWidth
                radius:                 width / 2
                color:                  root._missionController.homePositionSet ? qgcPal.colorGreen : "transparent"
                border.color:           root._missionController.homePositionSet ? qgcPal.colorGreen : qgcPal.textSecondary
            }

            QGCLabel {
                objectName:         "planPanel_homeStatus"
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                text: {
                    if (!root._missionController.homePositionSet) {
                        return qsTr("Not set. Click in the map to set it.")
                    }
                    if (root._settingsItem) {
                        if (root._settingsItem.terrainQueryFailed) {
                            return qsTr("Set, altitude (AMSL)")
                        }
                        return qsTr("Set, %1 %2 AMSL").arg(root._settingsItem.plannedHomePositionAltitude.valueString).arg(root._settingsItem.plannedHomePositionAltitude.units)
                    }
                    return qsTr("Set")
                }
            }

            Loader {
                active: root._missionController.homePositionSet && root._settingsItem !== null && root._settingsItem.terrainQueryFailed

                sourceComponent: FactTextField {
                    objectName: "planPanel_homeAltitude"
                    fact:       root._settingsItem.plannedHomePositionAltitude
                }
            }
        }

        PlanPanelGroupHeader {
            objectName: "planPanel_templatesHeader"
            text:       qsTr("Start from a template")
            summary:    qsTr("optional")
            visible:    root.planMasterController.showCreateFromTemplate
        }

        ColumnLayout {
            objectName:         "planPanel_templatesColumn"
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            visible:            root.planMasterController.showCreateFromTemplate
            enabled:            root._missionController.homePositionSet
            opacity:            enabled ? 1.0 : 0.5

            Repeater {
                model: root.planMasterController.planCreators

                QGCButton {
                    required property var object

                    objectName:         "planCreator_" + object.name
                    Layout.fillWidth:   true
                    text:               object.name

                    onClicked: {
                        if (!mainWindow.allowViewSwitch()) {
                            return
                        }
                        if (object.blankPlan) {
                            root.planMasterController.userSelectedManualCreation = true
                            root.missionTabRequested()
                        } else {
                            object.createPlan(root.editorMap.center)
                        }
                    }
                }
            }
        }

        PlanPanelGroupHeader {
            text: qsTr("Defaults")
        }

        Item {
            Layout.fillWidth:       true
            Layout.preferredHeight: defaultsLoader.item ? defaultsLoader.item.height : 0

            // Only the width is set so the editor keeps the height of its own content
            Loader {
                id: defaultsLoader

                width:  parent.width
                active: root._settingsItem !== null

                sourceComponent: MissionDefaultsEditor {
                    objectName:             "planPanel_defaults"
                    missionController:      root._missionController
                    planMasterController:   root.planMasterController
                }
            }
        }

        PlanPanelGroupHeader {
            text: qsTr("Tools")
        }

        QGCButton {
            objectName:         "planPanel_transformButton"
            Layout.fillWidth:   true
            text:               qsTr("Transform mission...")
            onClicked:          transformDialogFactory.open()
        }
    }
}
