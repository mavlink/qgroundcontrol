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

import QGroundControl.FactControls

import QGroundControl.Controls



SetupPage {
    id:             tuningPage
    pageComponent:  tuningPageComponent

    Component {
        id: tuningPageComponent

        ColumnLayout {
            width:  availableWidth

            FactPanelController { id: controller; }

            property bool _atcInputTCAvailable: controller.parameterExists(-1, "ATC_INPUT_TC")
            property Fact _atcInputTC:          controller.getParameterFact(-1, "ATC_INPUT_TC", false)
            property Fact _rateRollP:           controller.getParameterFact(-1, "ATC_RAT_RLL_P")
            property Fact _rateRollI:           controller.getParameterFact(-1, "ATC_RAT_RLL_I")
            property Fact _ratePitchP:          controller.getParameterFact(-1, "ATC_RAT_PIT_P")
            property Fact _ratePitchI:          controller.getParameterFact(-1, "ATC_RAT_PIT_I")
            property Fact _rateClimbP:          controller.getParameterFact(-1, "PSC_ACCZ_P")
            property Fact _rateClimbI:          controller.getParameterFact(-1, "PSC_ACCZ_I")
            property Fact _motSpinArm:          controller.getParameterFact(-1, "MOT_SPIN_ARM")
            property Fact _motSpinMin:          controller.getParameterFact(-1, "MOT_SPIN_MIN")

            property Fact _ch7Opt:  controller.getParameterFact(-1, "r.RC7_OPTION")
            property Fact _ch8Opt:  controller.getParameterFact(-1, "r.RC8_OPTION")
            property Fact _ch9Opt:  controller.getParameterFact(-1, "r.RC9_OPTION")
            property Fact _ch10Opt: controller.getParameterFact(-1, "r.RC10_OPTION")
            property Fact _ch11Opt: controller.getParameterFact(-1, "r.RC11_OPTION")
            property Fact _ch12Opt: controller.getParameterFact(-1, "r.RC12_OPTION")

            readonly property int   _firstOptionChannel:    7
            readonly property int   _lastOptionChannel:     12

            property Fact   _autoTuneAxes:                  controller.getParameterFact(-1, "AUTOTUNE_AXES")
            property int    _autoTuneSwitchChannelIndex:    0
            readonly property int _autoTuneOption:          17

            property real _margins: ScreenTools.defaultFontPixelHeight

            property bool _loadComplete: false

            Component.onCompleted: {
                // We use QtCharts only on Desktop platforms
                showAdvanced = !ScreenTools.isMobile
                _loadComplete = true
                calcAutoTuneChannel()
            }

            /// The AutoTune switch is stored in one of the RC#_OPTION parameters. We need to loop through those
            /// to find them and setup the ui accordindly.
            function calcAutoTuneChannel() {
                _autoTuneSwitchChannelIndex = 0
                for (var channel=_firstOptionChannel; channel<=_lastOptionChannel; channel++) {
                    var optionFact = controller.getParameterFact(-1, "r.RC" + channel + "_OPTION")
                    if (optionFact.value == _autoTuneOption) {
                        _autoTuneSwitchChannelIndex = channel - _firstOptionChannel + 1
                        break
                    }
                }
            }

            /// We need to clear AutoTune from any previous channel before setting it to a new one
            function setChannelAutoTuneOption(channel) {
                // First clear any previous settings for AutTune
                for (var optionChannel=_firstOptionChannel; optionChannel<=_lastOptionChannel; optionChannel++) {
                    var optionFact = controller.getParameterFact(-1, "r.RC" + optionChannel + "_OPTION")
                    if (optionFact.value == _autoTuneOption) {
                        optionFact.value = 0
                    }
                }

                // Now set the function into the new channel
                if (channel != 0) {
                    var optionFact = controller.getParameterFact(-1, "r.RC" + channel + "_OPTION")
                    optionFact.value = _autoTuneOption
                }
            }

            Connections { target: _ch7Opt; function onValueChanged(value) { calcAutoTuneChannel() } }
            Connections { target: _ch8Opt; function onValueChanged(value) { calcAutoTuneChannel() } }
            Connections { target: _ch9Opt; function onValueChanged(value) { calcAutoTuneChannel() } }
            Connections { target: _ch10Opt; function onValueChanged(value) { calcAutoTuneChannel() } }
            Connections { target: _ch11Opt; function onValueChanged(value) { calcAutoTuneChannel() } }
            Connections { target: _ch12Opt; function onValueChanged(value) { calcAutoTuneChannel() } }

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            _margins
                visible:            !advanced

                SettingsGroupLayout {
                    Layout.fillWidth:   true
                    heading:            qsTr("Roll/Pitch Sensitivity")
                    headingDescription: qsTr("Slide to the right if the copter is sluggish or slide to the left if the copter is twitchy")

                    FactSlider {
                        id:                 rollPitch
                        Layout.fillWidth:   true
                        from:               0.08
                        to:                 0.4
                        majorTickStepSize:  0.02
                        decimalPlaces:      3
                        fact:              _rateRollP

                        onValueChanged: {
                            _rateRollI.rawValue = value
                            _ratePitchP.rawValue = value
                            _ratePitchI.rawValue = value
                        }
                    }
                }

                SettingsGroupLayout {
                    Layout.fillWidth:   true
                    heading:            qsTr("Climb Sensitivity")
                    headingDescription: qsTr("Slide to the right to climb more aggressively or slide to the left to climb more gently")

                    FactSlider {
                        id:                 climb
                        Layout.fillWidth:   true
                        from:               0.03
                        to:                 1.0
                        majorTickStepSize:  0.05
                        decimalPlaces:      3
                        fact:               _rateClimbP
                        onValueChanged:     _rateClimbI.rawValue = value * 2
                    }
                }

                SettingsGroupLayout {
                    Layout.fillWidth:   true
                    heading:            qsTr("RC Roll/Pitch Feel")
                    headingDescription: qsTr("Slide to the left for soft control, slide to the right for crisp control")

                    FactSlider {
                        id:                 atcInputTC
                        Layout.fillWidth:   true
                        from:               _atcInputTC.min
                        to:                 _atcInputTC.max
                        majorTickStepSize:  _atcInputTC.increment
                        decimalPlaces:      2
                        unitsString:        ""
                        fact:              _atcInputTC
                    }
                }

                SettingsGroupLayout {
                    Layout.fillWidth:   true
                    heading:            qsTr("Spin While Armed")
                    headingDescription: qsTr("Adjust the amount the motors spin to indicate armed. Should be lower than 'Minimum Thrust'")

                    FactSlider {
                        Layout.fillWidth:   true
                        from:               0
                        to:                 0.3
                        majorTickStepSize:  0.1
                        decimalPlaces:      1
                        fact:              _motSpinArm
                    }
                }

                SettingsGroupLayout {
                    Layout.fillWidth:   true
                    heading:            qsTr("Minimum Thrust")
                    headingDescription: qsTr("Adjust the minimum amount of thrust require for the vehicle to move. Should be higher than 'Spin While Armed")

                    FactSlider {
                        Layout.fillWidth:   true
                        from:               0
                        to:                 0.3
                        majorTickStepSize:  0.1
                        decimalPlaces:      1
                        fact:              _motSpinArm
                    }
                }
 
                Flow {
                    id:                 flowLayout
                    Layout.fillWidth:   true
                    spacing:            _margins

                    SettingsGroupLayout {
                        heading: qsTr("AutoTune")

                        ColumnLayout {
                            RowLayout {
                                spacing: _margins

                                QGCLabel { text: qsTr("Axes to AutoTune:") }
                                FactBitmask { fact: _autoTuneAxes }
                            }

                            LabelledComboBox {
                                id:             autoTuneChannelCombo
                                width:          ScreenTools.defaultFontPixelWidth * 14
                                label:          qsTr("Channel for AutoTune switch:")
                                model:          [qsTr("None"), qsTr("Channel 7"), qsTr("Channel 8"), qsTr("Channel 9"), qsTr("Channel 10"), qsTr("Channel 11"), qsTr("Channel 12") ]
                                currentIndex:   _autoTuneSwitchChannelIndex

                                onActivated: (index) => {
                                    var channel = index

                                    if (channel > 0) {
                                        channel += 6
                                    }
                                    setChannelAutoTuneOption(channel)
                                }
                            }
                        }
                    }

                    SettingsGroupLayout {
                        heading: qsTr("In Flight Tuning")
                        
                        ColumnLayout {
                            id:     channel6TuningOptColumn
                            spacing: ScreenTools.defaultFontPixelHeight

                            LabelledFactComboBox {
                                id:         optCombo
                                width:      ScreenTools.defaultFontPixelWidth * 15
                                label:      qsTr("RC Channel 6 Option (Tuning):")
                                fact:       controller.getParameterFact(-1, "TUNE")
                                indexModel: false
                            }

                            RowLayout {
                                spacing: ScreenTools.defaultFontPixelWidth

                                LabelledFactTextField {
                                    id:                     tuneMinField
                                    textField.validator:    DoubleValidator {bottom: 0; top: 32767;}
                                    label:                  qsTr("Min:")
                                    fact:                   controller.getParameterFact(-1, "r.TUNE_MIN")
                                }

                                LabelledFactTextField {
                                    id:                     tuneMaxField
                                    textField.validator:    DoubleValidator {bottom: 0; top: 32767;}
                                    label:                  qsTr("Max:")
                                    fact:                   controller.getParameterFact(-1, "r.TUNE_MAX")
                                }
                            }
                        }
                    }
                }
            }

            Loader {
                Layout.fillWidth:   true
                sourceComponent:    advanced ? advancePageComponent : undefined
            }

            Component {
                id: advancePageComponent

                PIDTuning {
                    property bool useAutoTuning: false

                    property var roll: QtObject {
                        property string name: qsTr("Roll")
                        property var plot: [
                            { name: "Response", value: globals.activeVehicle.rollRate.value },
                            { name: "Setpoint", value: globals.activeVehicle.setpoint.rollRate.value }
                        ]
                        property var params: ListModel {
                            ListElement {
                                title:          qsTr("Roll axis angle controller P gain")
                                param:          "ATC_ANG_RLL_P"
                                description:    ""
                                min:            3
                                max:            12
                                step:           1
                            }
                            ListElement {
                                title:          qsTr("Roll axis rate controller P gain")
                                param:          "ATC_RAT_RLL_P"
                                description:    ""
                                min:            0.001
                                max:            0.5
                                step:           0.025
                            }
                            ListElement {
                                title:          qsTr("Roll axis rate controller I gain")
                                param:          "ATC_RAT_RLL_I"
                                description:    ""
                                min:            0.01
                                max:            2
                                step:           0.05
                            }
                            ListElement {
                                title:          qsTr("Roll axis rate controller D gain")
                                param:          "ATC_RAT_RLL_D"
                                description:    ""
                                min:            0.0
                                max:            0.05
                                step:           0.001
                            }
                        }
                    }
                    property var pitch: QtObject {
                        property string name: qsTr("Pitch")
                        property var plot: [
                            { name: "Response", value: globals.activeVehicle.pitchRate.value },
                            { name: "Setpoint", value: globals.activeVehicle.setpoint.pitchRate.value }
                        ]
                        property var params: ListModel {
                            ListElement {
                                title:          qsTr("Pitch axis angle controller P gain")
                                param:          "ATC_ANG_PIT_P"
                                description:    ""
                                min:            3
                                max:            12
                                step:           1
                            }
                            ListElement {
                                title:          qsTr("Pitch axis rate controller P gain")
                                param:          "ATC_RAT_PIT_P"
                                description:    ""
                                min:            0.001
                                max:            0.5
                                step:           0.025
                            }
                            ListElement {
                                title:          qsTr("Pitch axis rate controller I gain")
                                param:          "ATC_RAT_PIT_I"
                                description:    ""
                                min:            0.01
                                max:            2
                                step:           0.05
                            }
                            ListElement {
                                title:          qsTr("Pitch axis rate controller D gain")
                                param:          "ATC_RAT_PIT_D"
                                description:    ""
                                min:            0.0
                                max:            0.05
                                step:           0.001
                            }
                        }
                    }
                    property var yaw: QtObject {
                        property string name: qsTr("Yaw")
                        property var plot: [
                            { name: "Response", value: globals.activeVehicle.yawRate.value },
                            { name: "Setpoint", value: globals.activeVehicle.setpoint.yawRate.value }
                        ]
                        property var params: ListModel {
                            ListElement {
                                title:          qsTr("Yaw axis angle controller P gain")
                                param:          "ATC_ANG_YAW_P"
                                description:    ""
                                min:            3
                                max:            12
                                step:           1
                            }
                            ListElement {
                                title:          qsTr("Yaw axis rate controller P gain")
                                param:          "ATC_RAT_YAW_P"
                                description:    ""
                                min:            0.1
                                max:            2.5
                                step:           0.05
                            }
                            ListElement {
                                title:          qsTr("Yaw axis rate controller I gain")
                                param:          "ATC_RAT_YAW_I"
                                description:    ""
                                min:            0.01
                                max:            1
                                step:           0.05
                            }
                        }
                    }
                    title: "Rate"
                    tuningMode: Vehicle.ModeDisabled
                    unit: "deg/s"
                    axis: [ roll, pitch, yaw ]
                    chartDisplaySec: 3
                }
            } // Component - Advanced Page
        } // Column
    } // Component
} // SetupView
