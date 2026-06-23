import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id:                 safetyPage
    pageComponent:      safetyPageComponent

    Component {
        id: safetyPageComponent

        ColumnLayout {
            id:         flowLayout
            spacing:    _margins

            FactPanelController { id: controller; }

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            property bool _firmware34:       globals.activeVehicle.versionCompare(3, 5, 0) < 0

            // Enable/Action parameters
            property Fact _failsafeBatteryEnable:     controller.getParameterFact(-1, "BATT_FS_LOW_ACT", false)
            property Fact _failsafeEKFEnable:         controller.getParameterFact(-1, "FS_EKF_ACTION")
            property Fact _failsafeGCSEnable:         controller.getParameterFact(-1, "FS_GCS_ENABLE")
            property Fact _failsafeLeakEnable:        controller.getParameterFact(-1, "FS_LEAK_ENABLE")
            property Fact _failsafePilotEnable:       _firmware34 ? null : controller.getParameterFact(-1, "FS_PILOT_INPUT")
            property Fact _failsafePressureEnable:    controller.getParameterFact(-1, "FS_PRESS_ENABLE")
            property Fact _failsafeTemperatureEnable: controller.getParameterFact(-1, "FS_TEMP_ENABLE")

            // Threshold parameters
            property Fact _failsafePressureThreshold:    controller.getParameterFact(-1, "FS_PRESS_MAX")
            property Fact _failsafeTemperatureThreshold: controller.getParameterFact(-1, "FS_TEMP_MAX")
            property Fact _failsafePilotTimeout:         _firmware34 ? null : controller.getParameterFact(-1, "FS_PILOT_TIMEOUT")
            property Fact _failsafeLeakPin:              controller.getParameterFact(-1, "LEAK1_PIN")
            property Fact _failsafeLeakLogic:            controller.getParameterFact(-1, "LEAK1_LOGIC")
            property Fact _failsafeEKFThreshold:         controller.getParameterFact(-1, "FS_EKF_THRESH")
            property Fact _failsafeBatteryVoltage:       controller.getParameterFact(-1, "BATT_LOW_VOLT", false)
            property Fact _failsafeBatteryCapacity:      controller.getParameterFact(-1, "BATT_LOW_MAH", false)
            property bool _batteryDetected:              controller.parameterExists(-1, "BATT_LOW_MAH")

            property Fact _armingCheck:     controller.getParameterFact(-1, "ARMING_CHECK", false)
            property Fact _armingSkipCheck: controller.getParameterFact(-1, "ARMING_SKIPCHK", false)

            property real _margins:         ScreenTools.defaultFontPixelHeight
            property real _textFieldWidth:  ScreenTools.defaultFontPixelWidth * 15
            property real _comboWidth:      ScreenTools.defaultFontPixelWidth * 30

            QGCGroupBox {
                title:              qsTr("Failsafe Actions")

                GridLayout {
                    columns:        2
                    rowSpacing:     _margins / 2
                    columnSpacing:  _margins / 2

                    QGCLabel { text: qsTr("GCS Heartbeat") }
                    FactComboBox {
                        Layout.maximumWidth: _comboWidth
                        fact:               _failsafeGCSEnable
                        indexModel:         false
                        sizeToContents:     true
                    }

                    QGCLabel { text: qsTr("Leak") }
                    FactComboBox {
                        id:                  leakEnableCombo
                        Layout.maximumWidth: _comboWidth
                        fact:               _failsafeLeakEnable
                        indexModel:         false
                        sizeToContents:     true
                    }

                    QGCLabel {
                        text:    qsTr("Detector Pin")
                        visible: leakEnableCombo.currentIndex !== 0
                    }
                    FactComboBox {
                        Layout.maximumWidth: _comboWidth
                        visible:            leakEnableCombo.currentIndex !== 0
                        fact:               _failsafeLeakPin
                        indexModel:         false
                        sizeToContents:     true
                    }

                    QGCLabel {
                        text:    qsTr("Logic when Dry")
                        visible: leakEnableCombo.currentIndex !== 0
                    }
                    FactComboBox {
                        Layout.maximumWidth: _comboWidth
                        visible:            leakEnableCombo.currentIndex !== 0
                        fact:               _failsafeLeakLogic
                        indexModel:         false
                        sizeToContents:     true
                    }

                    QGCLabel {
                        text:    qsTr("Battery")
                        visible: !_firmware34
                    }
                    FactComboBox {
                        id:                  batteryEnableCombo
                        Layout.maximumWidth: _comboWidth
                        enabled:            _batteryDetected
                        visible:            !_firmware34
                        fact:               _failsafeBatteryEnable
                        indexModel:         false
                        sizeToContents:     true
                    }

                    QGCLabel {
                        Layout.columnSpan:  2
                        text:               qsTr("Power module not set up")
                        color:              qgcPal.warningText
                        visible:            !_firmware34 && !_batteryDetected
                    }

                    QGCLabel {
                        text:    qsTr("Voltage")
                        visible: !_firmware34 && batteryEnableCombo.currentIndex !== 0
                    }
                    FactTextField {
                        Layout.preferredWidth: _textFieldWidth
                        visible:              !_firmware34 && batteryEnableCombo.currentIndex !== 0
                        fact:                 _failsafeBatteryVoltage
                    }

                    QGCLabel {
                        text:    qsTr("Remaining Capacity")
                        visible: !_firmware34 && batteryEnableCombo.currentIndex !== 0
                    }
                    FactTextField {
                        Layout.preferredWidth: _textFieldWidth
                        visible:              !_firmware34 && batteryEnableCombo.currentIndex !== 0
                        fact:                 _failsafeBatteryCapacity
                    }

                    QGCLabel {
                        text:    qsTr("EKF")
                        visible: !_firmware34
                    }
                    FactComboBox {
                        id:                  ekfEnableCombo
                        Layout.maximumWidth: _comboWidth
                        visible:            !_firmware34
                        fact:               _failsafeEKFEnable
                        indexModel:         false
                        sizeToContents:     true
                    }

                    QGCLabel {
                        text:    qsTr("Threshold")
                        visible: !_firmware34 && ekfEnableCombo.currentIndex !== 0
                    }
                    FactTextField {
                        Layout.preferredWidth: _textFieldWidth
                        visible:              !_firmware34 && ekfEnableCombo.currentIndex !== 0
                        fact:                 _failsafeEKFThreshold
                    }

                    QGCLabel {
                        text:    qsTr("Pilot Input")
                        visible: !_firmware34
                    }
                    FactComboBox {
                        id:                  pilotEnableCombo
                        Layout.maximumWidth: _comboWidth
                        visible:            !_firmware34
                        fact:               _failsafePilotEnable
                        indexModel:         false
                        sizeToContents:     true
                    }

                    QGCLabel {
                        text:    qsTr("Timeout")
                        visible: !_firmware34 && pilotEnableCombo.currentIndex !== 0
                    }
                    FactTextField {
                        Layout.preferredWidth: _textFieldWidth
                        visible:              !_firmware34 && pilotEnableCombo.currentIndex !== 0
                        fact:                 _failsafePilotTimeout
                    }

                    QGCLabel { text: qsTr("Internal Temperature") }
                    FactComboBox {
                        id:                  temperatureEnableCombo
                        Layout.maximumWidth: _comboWidth
                        fact:               _failsafeTemperatureEnable
                        indexModel:         false
                        sizeToContents:     true
                    }

                    QGCLabel {
                        text:    qsTr("Threshold")
                        visible: temperatureEnableCombo.currentIndex !== 0
                    }
                    FactTextField {
                        Layout.preferredWidth: _textFieldWidth
                        visible:              temperatureEnableCombo.currentIndex !== 0
                        fact:                 _failsafeTemperatureThreshold
                    }

                    QGCLabel { text: qsTr("Internal Pressure") }
                    FactComboBox {
                        id:                  pressureEnableCombo
                        Layout.maximumWidth: _comboWidth
                        fact:               _failsafePressureEnable
                        indexModel:         false
                        sizeToContents:     true
                    }

                    QGCLabel {
                        text:    qsTr("Threshold")
                        visible: pressureEnableCombo.currentIndex !== 0
                    }
                    FactTextField {
                        Layout.preferredWidth: _textFieldWidth
                        visible:              pressureEnableCombo.currentIndex !== 0
                        fact:                 _failsafePressureThreshold
                    }
                } // GridLayout
            } // QGCGroupBox - Failsafe Actions

            QGCGroupBox {
                title:              _armingCheck ? qsTr("Arming Checks") : qsTr("Skip Arming Checks")

                ColumnLayout {
                    spacing:        _margins

                    FactBitmask {
                        Layout.preferredWidth:  safetyPage.availableWidth * 0.75
                        firstEntryIsAll:        _armingCheck ? true : false
                        fact:                   _armingCheck ? _armingCheck : _armingSkipCheck
                    }

                    QGCLabel {
                        Layout.fillWidth:   true
                        wrapMode:           Text.WordWrap
                        color:              qgcPal.warningText
                        text:               qsTr("Warning: Turning off arming checks can lead to loss of Vehicle control.")
                        visible:            _armingCheck ? _armingCheck.value !== 1 : _armingSkipCheck.value !== 0
                    }
                }
            } // QGCGroupBox - Arming Checks
        } // ColumnLayout
    } // Component - safetyPageComponent
} // SetupPage
