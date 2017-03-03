/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.7
import QtQuick.Controls     2.1

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:                 lightsPage
    pageComponent:      lightsPageComponent

    Component {
        id: lightsPageComponent

        Column {
            spacing:    _margins
            width:      availableWidth

            FactPanelController { id: controller; factPanel: lightsPage.viewPanel }

            QGCPalette { id: palette; colorGroupEnabled: true }

            property Fact _rc5Function:         controller.getParameterFact(-1, "r.SERVO5_FUNCTION")
            property Fact _rc6Function:         controller.getParameterFact(-1, "r.SERVO6_FUNCTION")
            property Fact _rc7Function:         controller.getParameterFact(-1, "r.SERVO7_FUNCTION")
            property Fact _rc8Function:         controller.getParameterFact(-1, "r.SERVO8_FUNCTION")
            property Fact _rc9Function:         controller.getParameterFact(-1, "r.SERVO9_FUNCTION")
            property Fact _rc10Function:        controller.getParameterFact(-1, "r.SERVO10_FUNCTION")
            property Fact _rc11Function:        controller.getParameterFact(-1, "r.SERVO11_FUNCTION")
            property Fact _rc12Function:        controller.getParameterFact(-1, "r.SERVO12_FUNCTION")
            property Fact _rc13Function:        controller.getParameterFact(-1, "r.SERVO13_FUNCTION")
            property Fact _rc14Function:        controller.getParameterFact(-1, "r.SERVO14_FUNCTION")
            property Fact _stepSize:            controller.getParameterFact(-1, "JS_LIGHTS_STEP")

            readonly property real  _margins:                       ScreenTools.defaultFontPixelHeight
            readonly property int   _rcFunctionDisabled:            0
            readonly property int   _rcFunctionRCIN9:               59
            readonly property int   _rcFunctionRCIN10:              60
            readonly property int   _firstLightsOutChannel:         5
            readonly property int   _lastLightsOutChannel:          14

            Component.onCompleted: {
                calcLightOutValues()
                calcCurrentStep()
            }

            /// Light output channels are stored in SERVO#_FUNCTION parameters. We need to loop through those
            /// to find them and setup the ui accordindly.
            function calcLightOutValues() {
                lightsLoader.lights1OutIndex = 0
                lightsLoader.lights2OutIndex = 0
                for (var channel=_firstLightsOutChannel; channel<=_lastLightsOutChannel; channel++) {
                    var functionFact = controller.getParameterFact(-1, "r.SERVO" + channel + "_FUNCTION")
                    if (functionFact.value == _rcFunctionRCIN9) {
                        lightsLoader.lights1OutIndex = channel - 4
                    } else if (functionFact.value == _rcFunctionRCIN10) {
                        lightsLoader.lights2OutIndex = channel - 4
                    }
                }
            }

            function setRCFunction(channel, rcFunction) {
                // First clear any previous settings for this function
                for (var index=_firstLightsOutChannel; index<=_lastLightsOutChannel; index++) {
                    var functionFact = controller.getParameterFact(-1, "r.SERVO" + index + "_FUNCTION")
                    if (functionFact.value != _rcFunctionDisabled && functionFact.value == rcFunction) {
                        functionFact.value = _rcFunctionDisabled
                    }
                }

                // Now set the function into the new channel
                if (channel != 0) {
                    var functionFact = controller.getParameterFact(-1, "r.SERVO" + channel + "_FUNCTION")
                    functionFact.value = rcFunction
                }
            }

            function calcCurrentStep() {
                var i = 1
                for(i; i <= 10; i++) {
                    var stepSize = (1900-1100)/i
                    if(_stepSize.value >= stepSize) {
                        _stepSize.value = stepSize;
                        break;
                    }
                }
                if (_stepSize.value < 80) {
                    _stepSize.value = 80;
                }
                lightsLoader.lightsSteps = i
            }

            function calcStepSize(steps) {
                _stepSize.value = (1900-1100)/steps
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
                ListElement { text: qsTr("Disabled"); value: 0 }
                ListElement { text: qsTr("Channel 5"); value: 5 }
                ListElement { text: qsTr("Channel 6"); value: 6 }
                ListElement { text: qsTr("Channel 7"); value: 7 }
                ListElement { text: qsTr("Channel 8"); value: 8 }
                ListElement { text: qsTr("Channel 9"); value: 9 }
                ListElement { text: qsTr("Channel 10"); value: 10 }
                ListElement { text: qsTr("Channel 11"); value: 11 }
                ListElement { text: qsTr("Channel 12"); value: 12 }
                ListElement { text: qsTr("Channel 13"); value: 13 }
                ListElement { text: qsTr("Channel 14"); value: 14 }
            }

            Component {
                id: lightSettings

                Item {
                    width:  rectangle.x + rectangle.width
                    height: rectangle.y + rectangle.height

                    QGCLabel {
                        id:             settingsLabel
                        text:           qsTr("Light Output Channels")
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        id:                 rectangle
                        anchors.topMargin:  _margins / 2
                        anchors.top:        settingsLabel.bottom
                        width:              lights1Combo.x + lights1Combo.width + lightsStepCombo.width + _margins
                        height:             lights2Combo.y + lights2Combo.height + lightsStepCombo.height + 2*_margins
                        color:              palette.windowShade

                        QGCLabel {
                            id:                 lights1Label
                            anchors.margins:    _margins
                            anchors.right:      lights1Combo.left
                            anchors.baseline:   lights1Combo.baseline
                            text:               qsTr("Lights 1:")
                        }

                        QGCComboBox {
                            id:                 lights1Combo
                            anchors.margins:    _margins
                            anchors.top:        parent.top
                            anchors.left:       lightsStepLabel.right
                            width:              ScreenTools.defaultFontPixelWidth * 15
                            model:              lightsOutModel
                            currentIndex:       lights1OutIndex

                            onActivated: setRCFunction(lightsOutModel.get(index).value, lights1Function)
                        }

                        QGCLabel {
                            id:                 lights2Label
                            anchors.margins:    _margins
                            anchors.right:      lights2Combo.left
                            anchors.baseline:   lights2Combo.baseline
                            text:               qsTr("Lights 2:")
                        }

                        QGCComboBox {
                            id:                 lights2Combo
                            anchors.margins:    _margins
                            anchors.top:        lights1Combo.bottom
                            anchors.left:       lightsStepLabel.right
                            width:              lights1Combo.width
                            model:              lightsOutModel
                            currentIndex:       lights2OutIndex

                            onActivated: setRCFunction(lightsOutModel.get(index).value, lights2Function)
                        }

                        QGCLabel {
                            id:                 lightsStepLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.baseline:   lightsStepCombo.baseline
                            text:               qsTr("Brightness Steps:")
                        }

                        QGCComboBox {
                            id:                 lightsStepCombo
                            anchors.margins:    _margins
                            anchors.top:        lights2Combo.bottom
                            anchors.left:       lightsStepLabel.right
                            width:              lights2Combo.width
                            model:              [1,2,3,4,5,6,7,8,9,10]
                            currentIndex:       lightsSteps-1

                            onActivated: calcStepSize(index+1)
                        }
                    } // Rectangle
                } // Item
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
