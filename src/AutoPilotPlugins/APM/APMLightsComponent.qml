import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id:                 lightsPage
    pageComponent:      lightsPageComponent

    Component {
        id: lightsPageComponent

        Column {
            spacing:    _margins
            width:      availableWidth

            FactPanelController { id: controller; }

            property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
            property bool _oldFW:               _activeVehicle.versionCompare(3, 5, 2) < 0
            property Fact _rc5Function:         controller.getParameterFact(-1, "SERVO5_FUNCTION")
            property Fact _rc6Function:         controller.getParameterFact(-1, "SERVO6_FUNCTION")
            property Fact _rc7Function:         controller.getParameterFact(-1, "SERVO7_FUNCTION")
            property Fact _rc8Function:         controller.getParameterFact(-1, "SERVO8_FUNCTION")
            property Fact _rc9Function:         controller.getParameterFact(-1, "SERVO9_FUNCTION")
            property Fact _rc10Function:        controller.getParameterFact(-1, "SERVO10_FUNCTION")
            property Fact _rc11Function:        controller.getParameterFact(-1, "SERVO11_FUNCTION")
            property Fact _rc12Function:        controller.getParameterFact(-1, "SERVO12_FUNCTION")
            property Fact _rc13Function:        controller.getParameterFact(-1, "SERVO13_FUNCTION")
            property Fact _rc14Function:        controller.getParameterFact(-1, "SERVO14_FUNCTION")
            property Fact _rc15Function:        controller.getParameterFact(-1, "SERVO15_FUNCTION")
            property Fact _rc16Function:        controller.getParameterFact(-1, "SERVO16_FUNCTION")
            property Fact _stepSize:            _oldFW ? controller.getParameterFact(-1, "JS_LIGHTS_STEP") : null // v3.5.1 and prior
            property Fact _numSteps:            _oldFW ? null : controller.getParameterFact(-1, "JS_LIGHTS_STEPS") // v3.5.2 and up

            readonly property real  _margins:                       ScreenTools.defaultFontPixelHeight
            readonly property real  _comboWidth:                    ScreenTools.defaultFontPixelWidth * 30
            readonly property int   _rcFunctionDisabled:            0
            readonly property int   _rcFunctionRCIN9:               59
            readonly property int   _rcFunctionRCIN10:              60
            readonly property int   _firstLightsOutChannel:         5
            readonly property int   _lastLightsOutChannel:          16

            Component.onCompleted: {
                calcLightOutValues()
                calcCurrentStep()
            }

            /// Light output channels are stored in SERVO#_FUNCTION parameters. We need to loop through those
            /// to find them and setup the ui accordindly.
            function calcLightOutValues() {
                lightsLoader.lights1OutIndex = 0
                lightsLoader.lights2OutIndex = 0
                for (let channel=_firstLightsOutChannel; channel<=_lastLightsOutChannel; channel++) {
                    let functionFact = controller.getParameterFact(-1, "SERVO" + channel + "_FUNCTION")
                    if (functionFact.value == _rcFunctionRCIN9) {
                        lightsLoader.lights1OutIndex = channel - 4
                    } else if (functionFact.value == _rcFunctionRCIN10) {
                        lightsLoader.lights2OutIndex = channel - 4
                    }
                }
            }

            function setRCFunction(channel, rcFunction) {
                // First clear any previous settings for this function
                for (let index=_firstLightsOutChannel; index<=_lastLightsOutChannel; index++) {
                    let functionFact = controller.getParameterFact(-1, "SERVO" + index + "_FUNCTION")
                    if (functionFact.value != _rcFunctionDisabled && functionFact.value == rcFunction) {
                        functionFact.value = _rcFunctionDisabled
                    }
                }

                // Now set the function into the new channel
                if (channel != 0) {
                    let functionFact = controller.getParameterFact(-1, "SERVO" + channel + "_FUNCTION")
                    functionFact.value = rcFunction
                }
            }

            function calcCurrentStep() {
                if (_oldFW) {
                    let i = 1
                    for(i; i <= 10; i++) {
                        let stepSize = (1900-1100)/i
                        if(_stepSize.value >= stepSize) {
                            _stepSize.value = stepSize;
                            break;
                        }
                    }
                    if (_stepSize.value < 80) {
                        _stepSize.value = 80;
                    }
                    lightsLoader.lightsSteps = i
                } else {
                    lightsLoader.lightsSteps = _numSteps.value
                }
            }

            function calcStepSize(steps) {
                if (_oldFW) {
                    _stepSize.value = (1900-1100)/steps
                } else {
                    _numSteps.value = steps
                }
            }

            // Whenever any SERVO#_FUNCTION parameters chagnes we need to go looking for light output channels again
            Connections { target: _rc5Function; onValueChanged: calcLightOutValues() }
            Connections { target: _rc6Function; onValueChanged: calcLightOutValues() }
            Connections { target: _rc7Function; onValueChanged: calcLightOutValues() }
            Connections { target: _rc8Function; onValueChanged: calcLightOutValues() }
            Connections { target: _rc9Function; onValueChanged: calcLightOutValues() }
            Connections { target: _rc10Function; onValueChanged: calcLightOutValues() }
            Connections { target: _rc11Function; onValueChanged: calcLightOutValues() }
            Connections { target: _rc12Function; onValueChanged: calcLightOutValues() }
            Connections { target: _rc13Function; onValueChanged: calcLightOutValues() }
            Connections { target: _rc14Function; onValueChanged: calcLightOutValues() }

            ListModel {
                id: lightsOutModel
                // It appears that QGCComboBox can't handle models that don't have a initial item
                // after onModelChanged
                ListElement { text: qsTr("Disabled"); value: 0 }

                function update(number) {
                    // Not enough channels
                    if(number < 6) {
                        return
                    }
                    for(let i = 5; i <= number; i++) {
                        let text = qsTr("Channel ") + i
                        append({"text": text, "value": i})
                    }
                }

                Component.onCompleted: {
                    // Number of main outputs plus extra auxiliary outputs
                    // http://ardupilot.org/copter/docs/parameters.html#brd-pwm-count-auxiliary-pin-config
                    let totalChannels = _lastLightsOutChannel
                    if (controller.parameterExists(-1, "BRD_PWM_COUNT")) {
                        let brd_pwm_count_value = controller.getParameterFact(-1, "BRD_PWM_COUNT").value
                        totalChannels = 8 + (brd_pwm_count_value == 7 ? 3 : brd_pwm_count_value)
                    }
                    update(totalChannels)
                }
            }

            Component {
                id: lightSettings

                QGCGroupBox {
                    title: qsTr("Light Output Channels")

                    GridLayout {
                        columns:        2
                        rowSpacing:     _margins / 2
                        columnSpacing:  _margins / 2

                        QGCLabel { text: qsTr("Lights 1") }
                        QGCComboBox {
                            id:                  lights1Combo
                            sizeToContents:      true
                            Layout.maximumWidth: _comboWidth
                            model:              lightsOutModel
                            textRole:           "text"
                            currentIndex:       lights1OutIndex

                            onActivated: (index) => { setRCFunction(lightsOutModel.get(index).value, lights1Function) }
                        }

                        QGCLabel { text: qsTr("Lights 2") }
                        QGCComboBox {
                            id:                  lights2Combo
                            sizeToContents:      true
                            Layout.maximumWidth: _comboWidth
                            model:              lightsOutModel
                            textRole:           "text"
                            currentIndex:       lights2OutIndex

                            onActivated: (index) => { setRCFunction(lightsOutModel.get(index).value, lights2Function) }
                        }

                        QGCLabel { text: qsTr("Brightness Steps") }
                        QGCComboBox {
                            id:                  lightsStepCombo
                            sizeToContents:      true
                            Layout.maximumWidth: _comboWidth
                            model:              [1,2,3,4,5,6,7,8,9,10]
                            currentIndex:       lightsSteps-1

                            onActivated: (index) => { calcStepSize(index+1) }
                        }
                    } // GridLayout
                } // QGCGroupBox
            } // Component - lightSettings

            Loader {
                id:                 lightsLoader
                sourceComponent:    lightSettings

                property int    lights1OutIndex:         0
                property int    lights2OutIndex:         0
                property int    lights1Function:         _rcFunctionRCIN9
                property int    lights2Function:         _rcFunctionRCIN10
                property int    lightsSteps:             1
            }
        } // Column
    } // Component
} // SetupPage
