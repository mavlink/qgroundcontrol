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
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.MultiVehicleManager
import QGroundControl.Palette
import QGroundControl.Controllers

SettingsPage {
    property var    _settingsManager:                   QGroundControl.settingsManager
    property var    _flyViewSettings:                   _settingsManager.flyViewSettings
    property var    _mavlinkActionsSettings:            _settingsManager.mavlinkActionsSettings
    property Fact   _virtualJoystick:                   _settingsManager.appSettings.virtualJoystick
    property Fact   _virtualJoystickAutoCenterThrottle: _settingsManager.appSettings.virtualJoystickAutoCenterThrottle
    property Fact   _enableMultiVehiclePanel:           _settingsManager.appSettings.enableMultiVehiclePanel
    property Fact   _showAdditionalIndicatorsCompass:   _flyViewSettings.showAdditionalIndicatorsCompass
    property Fact   _lockNoseUpCompass:                 _flyViewSettings.lockNoseUpCompass
    property Fact   _guidedMinimumAltitude:             _flyViewSettings.guidedMinimumAltitude
    property Fact   _guidedMaximumAltitude:             _flyViewSettings.guidedMaximumAltitude
    property Fact   _maxGoToLocationDistance:           _flyViewSettings.maxGoToLocationDistance
    property var    _viewer3DSettings:                  _settingsManager.viewer3DSettings
    property Fact   _viewer3DEnabled:                   _viewer3DSettings.enabled
    property Fact   _viewer3DOsmFilePath:               _viewer3DSettings.osmFilePath
    property Fact   _viewer3DBuildingLevelHeight:       _viewer3DSettings.buildingLevelHeight
    property Fact   _viewer3DAltitudeBias:              _viewer3DSettings.altitudeBias

    QGCFileDialogController { id: fileController }

    function mavlinkActionList() {
        var fileModel = fileController.getFiles(_settingsManager.appSettings.mavlinkActionsSavePath, "*.json")
        fileModel.unshift(qsTr("<None>"))
        return fileModel
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("General")

        FactCheckBoxSlider {
            id:                 useCheckList
            Layout.fillWidth:   true
            text:               qsTr("Use Preflight Checklist")
            fact:               _useChecklist
            visible:            _useChecklist.visible && QGroundControl.corePlugin.options.preFlightChecklistUrl.toString().length
            property Fact _useChecklist:      _settingsManager.appSettings.useChecklist
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Enforce Preflight Checklist")
            fact:               _enforceChecklist
            enabled:            _settingsManager.appSettings.useChecklist.value
            visible:            useCheckList.visible && _enforceChecklist.visible
            property Fact _enforceChecklist: _settingsManager.appSettings.enforceChecklist
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Enable Multi-Vehicle Panel")
            fact:               _enableMultiVehiclePanel
            visible:            _enableMultiVehiclePanel.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Keep Map Centered On Vehicle")
            fact:               _keepMapCenteredOnVehicle
            visible:            _keepMapCenteredOnVehicle.visible
            property Fact _keepMapCenteredOnVehicle: _flyViewSettings.keepMapCenteredOnVehicle
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Show Telemetry Log Replay Status Bar")
            fact:               _showLogReplayStatusBar
            visible:            _showLogReplayStatusBar.visible
            property Fact _showLogReplayStatusBar: _flyViewSettings.showLogReplayStatusBar
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Show simple camera controls (DIGICAM_CONTROL)")
            visible:            _showDumbCameraControl.visible
            fact:               _showDumbCameraControl

            property Fact _showDumbCameraControl: _flyViewSettings.showSimpleCameraControl
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Update return to home position based on device location.")
            fact:               _updateHomePosition
            visible:            _updateHomePosition.visible
            property Fact _updateHomePosition: _flyViewSettings.updateHomePosition
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Guided Commands")
        visible:            _guidedMinimumAltitude.visible || _guidedMaximumAltitude.visible || _maxGoToLocationDistance.visible

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Minimum Altitude")
            fact:               _guidedMinimumAltitude
            visible:            fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Maximum Altitude")
            fact:               _guidedMaximumAltitude
            visible:            fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Go To Location Max Distance")
            fact:               _maxGoToLocationDistance
            visible:            fact.visible
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:       true
        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 35
        heading:                qsTr("MAVLink Actions")
        headingDescription:     qsTr("Action JSON files should be created in the '%1' folder.").arg(QGroundControl.settingsManager.appSettings.mavlinkActionsSavePath)

        LabelledComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Fly View Actions")
            model:              mavlinkActionList()
            onActivated:        (index) => index == 0 ? _mavlinkActionsSettings.flyViewActionsFile.rawValue = "" : _mavlinkActionsSettings.flyViewActionsFile.rawValue = comboBox.currentText
            enabled:            model.length > 1

            Component.onCompleted: {
                var index = comboBox.find(_mavlinkActionsSettings.flyViewActionsFile.valueString)
                comboBox.currentIndex = index == -1 ? 0 : index
            }
        }

        LabelledComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Joystick Actions")
            model:              mavlinkActionList()
            onActivated:        (index) => index == 0 ? _mavlinkActionsSettings.joystickActionsFile.rawValue = "" : _mavlinkActionsSettings.joystickActionsFile.rawValue = comboBox.currentText
            enabled:            model.length > 1

            Component.onCompleted: {
                var index = comboBox.find(_mavlinkActionsSettings.joystickActionsFile.valueString)
                comboBox.currentIndex = index == -1 ? 0 : index
            }
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Virtual Joystick")
        visible:            _virtualJoystick.visible || _virtualJoystickAutoCenterThrottle.visible

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Enabled")
            visible:            _virtualJoystick.visible
            fact:               _virtualJoystick
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Auto-Center Throttle")
            visible:            _virtualJoystickAutoCenterThrottle.visible
            enabled:            _virtualJoystick.rawValue
            fact:               _virtualJoystickAutoCenterThrottle
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Instrument Panel")
        visible:            _showAdditionalIndicatorsCompass.visible || _lockNoseUpCompass.visible

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Show additional heading indicators on Compass")
            visible:            _showAdditionalIndicatorsCompass.visible
            fact:               _showAdditionalIndicatorsCompass
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Lock Compass Nose-Up")
            visible:            _lockNoseUpCompass.visible
            fact:               _lockNoseUpCompass
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("3D View")
        visible:            _viewer3DSettings.visible

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Enabled")
            fact:               _viewer3DEnabled
            visible:            _viewer3DEnabled.visible
        }

        ColumnLayout{
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth
            enabled:            _viewer3DEnabled.rawValue
            visible:            _viewer3DOsmFilePath.rawValue

            RowLayout{
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    wrapMode:   Text.WordWrap
                    visible:    true
                    text:       qsTr("3D Map File:")
                }

                QGCTextField {
                    id:                 osmFileTextField
                    height:             ScreenTools.defaultFontPixelWidth * 4.5
                    unitsLabel:         ""
                    showUnits:          false
                    visible:            true
                    Layout.fillWidth:   true
                    readOnly:           true
                    text:               _viewer3DOsmFilePath.rawValue
                }
            }

            RowLayout{
                Layout.alignment:   Qt.AlignRight
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text: qsTr("Clear")

                    onClicked: {
                        osmFileTextField.text = "Please select an OSM file"
                        _viewer3DOsmFilePath.value = osmFileTextField.text
                    }
                }

                QGCButton {
                    text: qsTr("Select File")

                    onClicked: {
                        var filename = _viewer3DOsmFilePath.rawValue;
                        const found = filename.match(/(.*)[\/\\]/);
                        if(found){
                            filename = found[1]||''; // extracting the directory from the file path
                            fileDialog.folder = (filename[0] === "/")?(filename.slice(1)):(filename);
                        }
                        fileDialog.openForLoad()
                    }

                    QGCFileDialog {
                        id:             fileDialog
                        nameFilters:    [qsTr("OpenStreetMap files (*.osm)")]
                        title:          qsTr("Select map file")

                        onAcceptedForLoad: (file) => {
                                               osmFileTextField.text = file
                                               _viewer3DOsmFilePath.value = osmFileTextField.text
                        }
                    }
                }
            }
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Average Building Level Height")
            fact:               _viewer3DBuildingLevelHeight
            enabled:            _viewer3DEnabled.rawValue
            visible:            _viewer3DBuildingLevelHeight.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Vehicles Altitude Bias")
            fact:               _viewer3DAltitudeBias
            enabled:            _viewer3DEnabled.rawValue
            visible:            _viewer3DAltitudeBias.visible
        }
    }
}
