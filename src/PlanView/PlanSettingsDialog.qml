/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Vehicle
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.SettingsManager
import QGroundControl.Controllers

// Editor for Mission Settings
QGCPopupDialog {
    id:         root
    title:      qsTr("Plan Settings")
    buttons:    Dialog.Ok

    property var planMasterController
    
    property var    _masterControler:               planMasterController
    property var    _missionController:             _masterControler.missionController
    property var    missionItem:                     _missionController.visualItems.get(0)
    property var    _controllerVehicle:             _masterControler.controllerVehicle
    property bool   _vehicleHasHomePosition:        _controllerVehicle.homePosition.isValid
    property bool   _showCruiseSpeed:               !_controllerVehicle.multiRotor
    property bool   _showHoverSpeed:                _controllerVehicle.multiRotor || _controllerVehicle.vtol
    property bool   _multipleFirmware:              !QGroundControl.singleFirmwareSupport
    property bool   _multipleVehicleTypes:          !QGroundControl.singleVehicleSupport
    property real   _fieldWidth:                    ScreenTools.defaultFontPixelWidth * 16
    property bool   _mobile:                        ScreenTools.isMobile
    property var    _savePath:                      QGroundControl.settingsManager.appSettings.planSavePath
    property var    _fileExtension:                 QGroundControl.settingsManager.appSettings.missionFileExtension
    property var    _appSettings:                   QGroundControl.settingsManager.appSettings
    property bool   _waypointsOnlyMode:             QGroundControl.corePlugin.options.missionWaypointsOnly
    property bool   _simpleMissionStart:            QGroundControl.corePlugin.options.showSimpleMissionStart
    property bool   _showFlightSpeed:               !_controllerVehicle.vtol && !_simpleMissionStart && !_controllerVehicle.apmFirmware
    property bool   _noMissionItemsAdded:           !_masterControler.containsItems
    property bool   _allowFWVehicleTypeSelection:   _noMissionItemsAdded && !globals.activeVehicle

    property real _horizontalSpacing:   ScreenTools.defaultFontPixelWidth
    property real _verticalSpacing:     ScreenTools.defaultFontPixelHeight / 2
    property real _maxTextWidth:        ScreenTools.defaultFontPixelWidth * 40


    QGCPalette { id: qgcPal }
    Component { id: altModeDialogComponent; AltModeDialog { } }

    Connections {
        target: _controllerVehicle
        function onSupportsTerrainFrameChanged() {
            if (!_controllerVehicle.supportsTerrainFrame && _missionController.globalAltitudeMode === QGroundControl.AltitudeModeTerrainFrame) {
                _missionController.globalAltitudeMode = QGroundControl.AltitudeModeCalcAboveTerrain
            }
        }
    }

    RowLayout {
        spacing: ScreenTools.defaultFontPixeHeight / 2

        SettingsGroupLayout2 {
            Layout.fillWidth:   true
            Layout.alignment:   Qt.AlignTop

            LabelledButton {
                label:      planMasterController.planName 
                buttonText: qsTr("Rename")
                onClicked:  renamePlanDialogComponent.createObject(mainWindow).open()
            }

            LabelledFactTextField {
                fact: QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            _horizontalSpacing

                ColumnLayout {
                    Layout.fillWidth:   true
                    spacing:            0

                    QGCLabel {
                        text: qsTr("Altitude Mode") + " - " + QGroundControl.altitudeModeShortDescription(_missionController.globalAltitudeMode)
                    }

                    QGCLabel {
                        Layout.fillWidth:       true
                        Layout.maximumWidth:   _maxTextWidth
                        font.pointSize:         ScreenTools.smallFontPointSize
                        text:                   QGroundControl.altitudeModeLongDescription(_missionController.globalAltitudeMode)
                        wrapMode:               Text.WordWrap
                    }
                }

                QGCButton {
                    text: qsTr("Configure")

                    onClicked: {
                        var removeModes = []
                        var updateFunction = function(altMode){ _missionController.globalAltitudeMode = altMode }
                        if (!_controllerVehicle.supportsTerrainFrame) {
                            removeModes.push(QGroundControl.AltitudeModeTerrainFrame)
                        }
                        altModeDialogComponent.createObject(mainWindow, { rgRemoveModes: removeModes, updateAltModeFn: updateFunction }).open()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            _horizontalSpacing

                QGCLabel {
                    text:               qsTr("Firmware")
                    Layout.fillWidth:   true
                    visible:            _multipleFirmware
                }
                FactComboBox {
                    fact:                   QGroundControl.settingsManager.appSettings.offlineEditingFirmwareClass
                    indexModel:             false
                    visible:                _multipleFirmware && _allowFWVehicleTypeSelection
                }
                QGCLabel {
                    text:       _controllerVehicle.firmwareTypeString
                    visible:    _multipleFirmware && !_allowFWVehicleTypeSelection
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            _horizontalSpacing

                QGCLabel {
                    text:               qsTr("Vehicle")
                    Layout.fillWidth:   true
                    visible:            _multipleVehicleTypes
                }
                FactComboBox {
                    fact:                   QGroundControl.settingsManager.appSettings.offlineEditingVehicleClass
                    indexModel:             false
                    visible:                _multipleVehicleTypes && _allowFWVehicleTypeSelection
                }
                QGCLabel {
                    text:       _controllerVehicle.vehicleTypeString
                    visible:    _multipleVehicleTypes && !_allowFWVehicleTypeSelection
                }
            }

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            0
                visible: _showCruiseSpeed || _showHoverSpeed

                QGCLabel {
                    text: qsTr("Default Speeds")
                }

                QGCLabel {
                    Layout.fillWidth:       true
                    Layout.maximumWidth:   _maxTextWidth
                    wrapMode:               Text.WordWrap
                    font.pointSize:         ScreenTools.smallFontPointSize
                    text:                   qsTr("These values are used to calculate total mission time when the plan uses the default vehicle speed.")
                }
            }

            LabelledFactTextField {
                fact:       QGroundControl.settingsManager.appSettings.offlineEditingCruiseSpeed
                visible:    _showCruiseSpeed
            }

            LabelledFactTextField {
                fact:       QGroundControl.settingsManager.appSettings.offlineEditingHoverSpeed
                visible:    _showHoverSpeed
            }
        }

        SettingsGroupLayout2 {
            heading:            qsTr("Start Actions")
            headingDescription: qsTr("Changes that occur when the plan starts.")
            Layout.fillWidth:   true
            Layout.alignment:   Qt.AlignTop

            RowLayout {
                Layout.fillWidth:   true
                spacing:            _horizontalSpacing
                visible:            _showFlightSpeed

                QGCCheckBoxSlider {
                    id:         flightSpeedCheckBox
                    checked:    missionItem.speedSection.specifyFlightSpeed
                    onClicked:  missionItem.speedSection.specifyFlightSpeed = checked
                }
                QGCLabel {
                    Layout.fillWidth:   true
                    text:               qsTr("Flight speed")
                    enabled:            flightSpeedCheckBox.checked
                }
                FactTextField {
                    Layout.alignment:   Qt.AlignRight
                    fact:               missionItem.speedSection.flightSpeed
                    enabled:            flightSpeedCheckBox.checked
                }
            }

            CameraSection2 {
                cameraSection: missionItem.cameraSection
            }
        }
    }

    Component {
        id: renamePlanDialogComponent

        QGCPopupDialog {
            id:         renamePlanDialog
            title:      qsTr("Rename Plan")
            buttons:    Dialog.Ok | Dialog.Cancel

            property string originalPlanName
    
            Component.onCompleted: {
                originalPlanName = planMasterController.planName
            }

            onAccepted: {
                if (planNameTextField.text === originalPlanName) {
                    return
                }
                
                var usedPlanNames = planMasterController.availablePlanNames()
                if (usedPlanNames.includes(planNameTextField.text)) {
                    renamePlanDialog.preventClose = true
                    mainWindow.showMessageDialog(
                        qsTr("Overwrite?"), 
                        qsTr("Plan already exists. Do you want to overwrite it?"), 
                        Dialog.Yes | Dialog.No, 
                        function accept() { 
                            planMasterController.renamePlan(planNameTextField.text)
                            renamePlaneDialog.close()
                        })
                } else {
                    planMasterController.renamePlan(planNameTextField.text)
                }
            }

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixeHeight / 2

                QGCLabel {
                    text: qsTr("Enter new plan name")
                }

                TextField {
                    id:     planNameTextField
                    text:   planMasterController.planName
                }
            }
        }
    }
}
