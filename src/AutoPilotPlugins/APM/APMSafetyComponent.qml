import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id:             safetyPage
    pageComponent:  safetyPageComponent

    Component {
        id: safetyPageComponent

        Flow {
            id:         flowLayout
            width:      availableWidth
            spacing:    ScreenTools.defaultFontPixelHeight

            FactPanelController { id: controller; }

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            property Fact _batt1Monitor:                    controller.getParameterFact(-1, "BATT_MONITOR")
            property Fact _batt2Monitor:                    controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
            property bool _batt2MonitorAvailable:           controller.parameterExists(-1, "BATT2_MONITOR")
            property bool _batt1MonitorEnabled:             _batt1Monitor.rawValue !== 0
            property bool _batt2MonitorEnabled:             _batt2MonitorAvailable ? _batt2Monitor.rawValue !== 0 : false
            property bool _batt1ParamsAvailable:            controller.parameterExists(-1, "BATT_CAPACITY")
            property bool _batt2ParamsAvailable:            controller.parameterExists(-1, "BATT2_CAPACITY")

            property Fact _failsafeBatt1LowAct:             controller.getParameterFact(-1, "BATT_FS_LOW_ACT", false /* reportMissing */)
            property Fact _failsafeBatt2LowAct:             controller.getParameterFact(-1, "BATT2_FS_LOW_ACT", false /* reportMissing */)
            property Fact _failsafeBatt1CritAct:            controller.getParameterFact(-1, "BATT_FS_CRT_ACT", false /* reportMissing */)
            property Fact _failsafeBatt2CritAct:            controller.getParameterFact(-1, "BATT2_FS_CRT_ACT", false /* reportMissing */)
            property Fact _failsafeBatt1LowMah:             controller.getParameterFact(-1, "BATT_LOW_MAH", false /* reportMissing */)
            property Fact _failsafeBatt2LowMah:             controller.getParameterFact(-1, "BATT2_LOW_MAH", false /* reportMissing */)
            property Fact _failsafeBatt1CritMah:            controller.getParameterFact(-1, "BATT_CRT_MAH", false /* reportMissing */)
            property Fact _failsafeBatt2CritMah:            controller.getParameterFact(-1, "BATT2_CRT_MAH", false /* reportMissing */)
            property Fact _failsafeBatt1LowVoltage:         controller.getParameterFact(-1, "BATT_LOW_VOLT", false /* reportMissing */)
            property Fact _failsafeBatt2LowVoltage:         controller.getParameterFact(-1, "BATT2_LOW_VOLT", false /* reportMissing */)
            property Fact _failsafeBatt1CritVoltage:        controller.getParameterFact(-1, "BATT_CRT_VOLT", false /* reportMissing */)
            property Fact _failsafeBatt2CritVoltage:        controller.getParameterFact(-1, "BATT2_CRT_VOLT", false /* reportMissing */)

            // Older firmwares use ARMING_CHECK. Newer firmwares use ARMING_SKIPCHK.
            property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK", false /* reportMissing */)
            property Fact _armingSkipCheck: controller.getParameterFact(-1, "ARMING_SKIPCHK", false /* reportMissing */)

            property real _margins:         ScreenTools.defaultFontPixelWidth / 2
            property bool _showIcon:        !ScreenTools.isTinyScreen
            property bool _roverFirmware:   controller.parameterExists(-1, "MODE1") // This catches all usage of ArduRover firmware vehicle types: Rover, Boat...

            property string _restartRequired: qsTr("Requires vehicle reboot")

            Component {
                id: batteryFailsafeComponent

                ColumnLayout {
                    spacing: _margins

                    LabelledFactComboBox {
                        label:              qsTr("Low action")
                        fact:               failsafeBattLowAct
                        indexModel:         false
                        Layout.fillWidth:   true
                    }

                    LabelledFactComboBox {
                        label:              qsTr("Critical action")
                        fact:               failsafeBattCritAct
                        indexModel:         false
                        Layout.fillWidth:   true
                    }

                    LabelledFactTextField {
                        label:              qsTr("Low voltage threshold")
                        fact:               failsafeBattLowVoltage
                        textFieldShowUnits: true
                        Layout.fillWidth:   true
                    }

                    LabelledFactTextField {
                        label:              qsTr("Critical voltage threshold")
                        fact:               failsafeBattCritVoltage
                        textFieldShowUnits: true
                        Layout.fillWidth:   true
                    }

                    LabelledFactTextField {
                        label:              qsTr("Low mAh threshold")
                        fact:               failsafeBattLowMah
                        textFieldShowUnits: true
                        Layout.fillWidth:   true
                    }

                    LabelledFactTextField {
                        label:              qsTr("Critical mAh threshold")
                        fact:               failsafeBattCritMah
                        textFieldShowUnits: true
                        Layout.fillWidth:   true
                    }
                }
            }

            Component {
                id: restartRequiredComponent

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text: _restartRequired
                    }

                    QGCButton {
                        text:       qsTr("Reboot vehicle")
                        onClicked:  controller.vehicle.rebootVehicle()
                    }
                }
            }

            QGCGroupBox {
                title:   qsTr("Battery1 Failsafe Triggers")
                visible: _batt1MonitorEnabled

                Loader {
                    id:              battery1FailsafeLoader
                    sourceComponent: _batt1ParamsAvailable ? batteryFailsafeComponent : restartRequiredComponent

                    property Fact battMonitor:              _batt1Monitor
                    property bool battParamsAvailable:      _batt1ParamsAvailable
                    property Fact failsafeBattLowAct:       _failsafeBatt1LowAct
                    property Fact failsafeBattCritAct:      _failsafeBatt1CritAct
                    property Fact failsafeBattLowMah:       _failsafeBatt1LowMah
                    property Fact failsafeBattCritMah:      _failsafeBatt1CritMah
                    property Fact failsafeBattLowVoltage:   _failsafeBatt1LowVoltage
                    property Fact failsafeBattCritVoltage:  _failsafeBatt1CritVoltage
                }
            }

            QGCGroupBox {
                title:   qsTr("Battery2 Failsafe Triggers")
                visible: _batt2MonitorEnabled

                Loader {
                    id:              battery2FailsafeLoader
                    sourceComponent: _batt2ParamsAvailable ? batteryFailsafeComponent : restartRequiredComponent

                    property Fact battMonitor:              _batt2Monitor
                    property bool battParamsAvailable:      _batt2ParamsAvailable
                    property Fact failsafeBattLowAct:       _failsafeBatt2LowAct
                    property Fact failsafeBattCritAct:      _failsafeBatt2CritAct
                    property Fact failsafeBattLowMah:       _failsafeBatt2LowMah
                    property Fact failsafeBattCritMah:      _failsafeBatt2CritMah
                    property Fact failsafeBattLowVoltage:   _failsafeBatt2LowVoltage
                    property Fact failsafeBattCritVoltage:  _failsafeBatt2CritVoltage
                }
            }

            Component {
                id: planeGeneralFS

                QGCGroupBox {
                    title: qsTr("Failsafe Triggers")

                    property Fact _failsafeThrEnable:      controller.getParameterFact(-1, "THR_FAILSAFE")
                    property Fact _failsafeThrValue:       controller.getParameterFact(-1, "THR_FS_VALUE")
                    property Fact _failsafeGCSEnable:      controller.getParameterFact(-1, "FS_GCS_ENABL")
                    property Fact _failsafeShortAction:    controller.getParameterFact(-1, "FS_SHORT_ACTN")
                    property Fact _failsafeLongAction:     controller.getParameterFact(-1, "FS_LONG_ACTN")
                    property Fact _failsafeLongTimeout:    controller.getParameterFact(-1, "FS_LONG_TIMEOUT")
                    property bool _isQuadPlane:            controller.parameterExists(-1, "Q_TRANS_FAIL")
                    property Fact _transFailTimeout:       controller.getParameterFact(-1, "Q_TRANS_FAIL", false /* reportMissing */)
                    property Fact _transFailAction:        controller.getParameterFact(-1, "Q_TRANS_FAIL_ACT", false /* reportMissing */)

                    ColumnLayout {
                        spacing: _margins

                        LabelledFactComboBox {
                            label:            qsTr("Ground Station failsafe")
                            fact:             _failsafeGCSEnable
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            QGCCheckBox {
                                id:                 throttleEnableCheckBox
                                text:               qsTr("Throttle PWM threshold")
                                checked:            _failsafeThrEnable.value === 1
                                Layout.fillWidth:   true

                                onClicked: _failsafeThrEnable.value = (checked ? 1 : 0)
                            }

                            FactTextField {
                                fact:               _failsafeThrValue
                                showUnits:          true
                                enabled:            throttleEnableCheckBox.checked
                            }
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Short failsafe action")
                            fact:             _failsafeShortAction
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Long failsafe action")
                            fact:             _failsafeLongAction
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:              qsTr("Long failsafe timeout")
                            fact:               _failsafeLongTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("VTOL transition failure action")
                            fact:             _transFailAction
                            indexModel:       false
                            Layout.fillWidth: true
                            visible:          _isQuadPlane
                        }

                        LabelledFactTextField {
                            label:              qsTr("VTOL transition failure timeout")
                            fact:               _transFailTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                            visible:            _isQuadPlane
                        }
                    }
                }
            }

            Loader {
                sourceComponent: controller.vehicle.fixedWing ? planeGeneralFS : undefined
            }

            Component {
                id: roverGeneralFS

                QGCGroupBox {
                    title: qsTr("Failsafe Triggers")

                    property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABLE")
                    property Fact _failsafeGCSTimeout:  controller.getParameterFact(-1, "FS_GCS_TIMEOUT")
                    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "FS_THR_ENABLE")
                    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "FS_THR_VALUE")
                    property Fact _failsafeAction:      controller.getParameterFact(-1, "FS_ACTION")
                    property Fact _failsafeTimeout:     controller.getParameterFact(-1, "FS_TIMEOUT")
                    property Fact _failsafeCrashCheck:  controller.getParameterFact(-1, "FS_CRASH_CHECK")
                    property Fact _failsafeEkfAction:   controller.getParameterFact(-1, "FS_EKF_ACTION")
                    property Fact _failsafeEkfThresh:   controller.getParameterFact(-1, "FS_EKF_THRESH")

                    ColumnLayout {
                        spacing: _margins

                        LabelledFactComboBox {
                            label:            qsTr("Ground Station failsafe")
                            fact:             _failsafeGCSEnable
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:              qsTr("GCS failsafe timeout")
                            fact:               _failsafeGCSTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Throttle failsafe")
                            fact:             _failsafeThrEnable
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:            qsTr("PWM threshold")
                            fact:             _failsafeThrValue
                            Layout.fillWidth: true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Failsafe action")
                            fact:             _failsafeAction
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:              qsTr("Failsafe timeout")
                            fact:               _failsafeTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Crash check")
                            fact:             _failsafeCrashCheck
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("EKF failsafe action")
                            fact:             _failsafeEkfAction
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:            qsTr("EKF failsafe threshold")
                            fact:             _failsafeEkfThresh
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            Loader {
                sourceComponent: _roverFirmware ? roverGeneralFS : undefined
            }

            Component {
                id: failsafeOptionsComponent

                QGCGroupBox {
                    title: qsTr("Failsafe Options")

                    property Fact _failsafeOptions: controller.getParameterFact(-1, "FS_OPTIONS")

                    ColumnLayout {
                        spacing: _margins

                        FactBitmask {
                            fact: _failsafeOptions
                            Layout.preferredWidth:  safetyPage.availableWidth * 0.75
                        }
                    }
                }
            }

            Loader {
                sourceComponent: _roverFirmware ? failsafeOptionsComponent : undefined
            }

            Component {
                id: copterGeneralFS

                QGCGroupBox {
                    title: qsTr("General Failsafe Triggers")

                    property Fact _failsafeGCSEnable:     controller.getParameterFact(-1, "FS_GCS_ENABLE")
                    property Fact _failsafeGCSTimeout:    controller.getParameterFact(-1, "FS_GCS_TIMEOUT")
                    property Fact _failsafeThrEnable:     controller.getParameterFact(-1, "FS_THR_ENABLE")
                    property Fact _failsafeThrValue:      controller.getParameterFact(-1, "FS_THR_VALUE")
                    property Fact _failsafeEkfAction:     controller.getParameterFact(-1, "FS_EKF_ACTION")
                    property Fact _failsafeEkfThresh:     controller.getParameterFact(-1, "FS_EKF_THRESH")
                    property Fact _failsafeEkfFilt:       controller.getParameterFact(-1, "FS_EKF_FILT")
                    property Fact _failsafeCrashCheck:    controller.getParameterFact(-1, "FS_CRASH_CHECK")
                    property Fact _failsafeVibeEnable:    controller.getParameterFact(-1, "FS_VIBE_ENABLE")
                    property Fact _failsafeDREnable:      controller.getParameterFact(-1, "FS_DR_ENABLE")
                    property Fact _failsafeDRTimeout:     controller.getParameterFact(-1, "FS_DR_TIMEOUT")

                    ColumnLayout {
                        spacing: _margins

                        LabelledFactComboBox {
                            label:            qsTr("Ground Station failsafe")
                            fact:             _failsafeGCSEnable
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:              qsTr("GCS failsafe timeout")
                            fact:               _failsafeGCSTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Throttle failsafe")
                            fact:             _failsafeThrEnable
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:              qsTr("PWM threshold")
                            fact:               _failsafeThrValue
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("EKF failsafe action")
                            fact:             _failsafeEkfAction
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:            qsTr("EKF failsafe threshold")
                            fact:             _failsafeEkfThresh
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:              qsTr("EKF failsafe filter")
                            fact:               _failsafeEkfFilt
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Crash check")
                            fact:             _failsafeCrashCheck
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Vibration failsafe")
                            fact:             _failsafeVibeEnable
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Dead reckoning failsafe")
                            fact:             _failsafeDREnable
                            indexModel:       false
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:              qsTr("Dead reckoning timeout")
                            fact:               _failsafeDRTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                        }
                    }
                }
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterGeneralFS : undefined
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? failsafeOptionsComponent : undefined
            }

            Component {
                id: copterGeoFence

                QGCGroupBox {
                    title: qsTr("GeoFence")

                    property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
                    property Fact _fenceAltMax: controller.getParameterFact(-1, "FENCE_ALT_MAX")
                    property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
                    property Fact _fenceMargin: controller.getParameterFact(-1, "FENCE_MARGIN")
                    property Fact _fenceRadius: controller.getParameterFact(-1, "FENCE_RADIUS")
                    property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

                    readonly property int _maxAltitudeFenceBitMask: 1
                    readonly property int _circleFenceBitMask:      2
                    readonly property int _polygonFenceBitMask:     4

                    ColumnLayout {
                        spacing: ScreenTools.defaultFontPixelHeight / 2

                        FactCheckBox {
                            id:     enabledCheckBox
                            text:   qsTr("Enabled")
                            fact:   _fenceEnable
                        }

                        ColumnLayout {
                            enabled: enabledCheckBox.checked

                            RowLayout {
                                QGCCheckBox {
                                    text:    qsTr("Maximum Altitude")
                                    checked: _fenceType.rawValue & _maxAltitudeFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _maxAltitudeFenceBitMask
                                        } else {
                                            _fenceType.value &= ~_maxAltitudeFenceBitMask
                                        }
                                    }
                                }

                                FactTextField {
                                    fact: _fenceAltMax
                                }
                            }

                            RowLayout {
                                QGCCheckBox {
                                    text:    qsTr("Circle centered on Home")
                                    checked: _fenceType.rawValue & _circleFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _circleFenceBitMask
                                        } else {
                                            _fenceType.value &= ~_circleFenceBitMask
                                        }
                                    }
                                }

                                FactTextField {
                                    fact:      _fenceRadius
                                    showUnits: true
                                }
                            }

                            QGCCheckBox {
                                text:    qsTr("Inclusion/Exclusion Circles+Polygons")
                                checked: _fenceType.rawValue & _polygonFenceBitMask

                                onClicked: {
                                    if (checked) {
                                        _fenceType.rawValue |= _polygonFenceBitMask
                                    } else {
                                        _fenceType.value &= ~_polygonFenceBitMask
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            enabled: enabledCheckBox.checked

                            LabelledFactComboBox {
                                label:            qsTr("Breach action")
                                fact:             _fenceAction
                                Layout.fillWidth: true
                            }

                            LabelledFactTextField {
                                label:            qsTr("Fence margin")
                                fact:             _fenceMargin
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterGeoFence : undefined
            }

            Component {
                id: copterRTL

                QGCGroupBox {
                    title: qsTr("Return to Launch")

                    property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPD_MS")
                    property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT_M")
                    property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
                    property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL_M")
                    // RTL_ALT_M (4.7+) is in meters, RTL_ALT (pre-4.7) is in centimeters
                    property bool _rtlAltIsMeters:  controller.parameterExists(-1, "noremap.RTL_ALT_M")

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelHeight

                        QGCColoredImage {
                            id:                     icon
                            visible:                _showIcon
                            Layout.alignment:       Qt.AlignTop
                            Layout.preferredHeight: ScreenTools.defaultFontPixelWidth * 20
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 20
                            color:                  qgcPal.text
                            sourceSize.width:       width
                            mipmap:                 true
                            fillMode:               Image.PreserveAspectFit
                            source:                 "/qmlimages/ReturnToHomeAltitude.svg"
                        }

                        ColumnLayout {
                            spacing: _margins

                            QGCRadioButton {
                                text:    qsTr("Return at current altitude")
                                checked: _rtlAltFact.value == 0

                                onClicked: _rtlAltFact.value = 0
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: _margins

                                QGCRadioButton {
                                    Layout.fillWidth: true
                                    text:    qsTr("Return at specified altitude")
                                    checked: _rtlAltFact.value != 0

                                    // RTL_ALT_M (4.7+) is in meters, RTL_ALT (pre-4.7) is in centimeters
                                    onClicked: _rtlAltFact.value = _rtlAltIsMeters ? 15 : 1500
                                }

                                FactTextField {
                                    fact:      _rtlAltFact
                                    showUnits: true
                                    enabled:   _rtlAltFact.value != 0
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: _margins

                                QGCCheckBox {
                                    id:      homeLoiterCheckbox
                                    Layout.fillWidth: true
                                    checked: _rtlLoitTimeFact.value > 0
                                    text:    qsTr("Loiter above Home for")

                                    onClicked: _rtlLoitTimeFact.value = (checked ? 60 : 0)
                                }

                                FactTextField {
                                    fact:      _rtlLoitTimeFact
                                    showUnits: true
                                    enabled:   homeLoiterCheckbox.checked
                                }
                            }

                            LabelledFactTextField {
                                label:              qsTr("Final land stage altitude")
                                fact:               _rtlAltFinalFact
                                textFieldShowUnits: true
                                Layout.fillWidth:   true
                            }

                            LabelledFactTextField {
                                label:              qsTr("Final land stage descent speed")
                                fact:               _landSpeedFact
                                textFieldShowUnits: true
                                Layout.fillWidth:   true
                            }
                        }
                    }
                }
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterRTL : undefined
            }

            Component {
                id: planeRTL

                QGCGroupBox {
                    title: qsTr("Return to Launch")

                    property Fact _rtlAltFact: controller.getParameterFact(-1, "RTL_ALTITUDE")

                    ColumnLayout {
                        spacing: _margins

                        QGCRadioButton {
                            text:    qsTr("Return at current altitude")
                            checked: _rtlAltFact.value < 0

                            onClicked: _rtlAltFact.value = -1
                        }

                        RowLayout {
                            spacing: _margins

                            QGCRadioButton {
                                text:    qsTr("Return at specified altitude")
                                checked: _rtlAltFact.value >= 0

                                onClicked: _rtlAltFact.value = 10000
                            }

                            FactTextField {
                                fact:      _rtlAltFact
                                showUnits: true
                                enabled:   _rtlAltFact.value >= 0
                            }
                        }
                    }
                }
            }

            Loader {
                sourceComponent: controller.vehicle.fixedWing ? planeRTL : undefined
            }

            QGCGroupBox {
                title: _armingCheck ? qsTr("Arming Checks") : qsTr("Skip Arming Checks")

                ColumnLayout {
                    spacing: _margins

                    FactBitmask {
                        firstEntryIsAll:    _armingCheck ? true : false
                        fact:               _armingCheck ? _armingCheck : _armingSkipCheck
                        Layout.preferredWidth:  safetyPage.availableWidth * 0.75
                    }

                    QGCLabel {
                        wrapMode:           Text.WordWrap
                        color:              qgcPal.warningText
                        text:               qsTr("Warning: Turning off arming checks can lead to loss of Vehicle control.")
                        visible:            _armingCheck ? _armingCheck.value != 1 : _armingSkipCheck.value != 0
                        Layout.fillWidth:   true
                    }
                }
            }
        }
    }
}
