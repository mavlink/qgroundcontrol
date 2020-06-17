/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
import QtQuick.Controls     1.2
import QtQuick.Controls.Styles  1.4

import QGroundControl                 1.0
import QGroundControl.FactSystem      1.0
import QGroundControl.FactControls    1.0
import QGroundControl.Palette         1.0
import QGroundControl.Controls        1.0
import QGroundControl.ScreenTools     1.0
import QGroundControl.SettingsManager 1.0

SetupPage {
    id:             cameraPage
    pageComponent:  cameraPageComponent

    Component {
        id: cameraPageComponent

        Column {
            spacing:    _margins
            width:      availableWidth

            FactPanelController { id: controller; }

            QGCPalette { id: palette; colorGroupEnabled: true }

            property bool _oldFW:               !(activeVehicle.firmwareMajorVersion > 3 || activeVehicle.firmwareMinorVersion > 5 || activeVehicle.firmwarePatchVersion >= 2)

            property Fact _mountRetractX:       controller.getParameterFact(-1, "MNT_RETRACT_X")
            property Fact _mountRetractY:       controller.getParameterFact(-1, "MNT_RETRACT_Y")
            property Fact _mountRetractZ:       controller.getParameterFact(-1, "MNT_RETRACT_Z")

            property Fact _mountNeutralX:       controller.getParameterFact(-1, "MNT_NEUTRAL_X")
            property Fact _mountNeutralY:       controller.getParameterFact(-1, "MNT_NEUTRAL_Y")
            property Fact _mountNeutralZ:       controller.getParameterFact(-1, "MNT_NEUTRAL_Z")

            property Fact _mountRCInTilt:       controller.getParameterFact(-1, "MNT_RC_IN_TILT")
            property Fact _mountStabTilt:       controller.getParameterFact(-1, "MNT_STAB_TILT")
            property Fact _mountAngMinTilt:     controller.getParameterFact(-1, "MNT_ANGMIN_TIL")
            property Fact _mountAngMaxTilt:     controller.getParameterFact(-1, "MNT_ANGMAX_TIL")

            property Fact _mountRCInRoll:       controller.getParameterFact(-1, "MNT_RC_IN_ROLL")
            property Fact _mountStabRoll:       controller.getParameterFact(-1, "MNT_STAB_ROLL")
            property Fact _mountAngMinRoll:     controller.getParameterFact(-1, "MNT_ANGMIN_ROL")
            property Fact _mountAngMaxRoll:     controller.getParameterFact(-1, "MNT_ANGMAX_ROL")

            property Fact _mountRCInPan:        controller.getParameterFact(-1, "MNT_RC_IN_PAN")
            property Fact _mountStabPan:        controller.getParameterFact(-1, "MNT_STAB_PAN")
            property Fact _mountAngMinPan:      controller.getParameterFact(-1, "MNT_ANGMIN_PAN")
            property Fact _mountAngMaxPan:      controller.getParameterFact(-1, "MNT_ANGMAX_PAN")

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

            // These enable/disable the options for setting up each axis
            property bool _tiltEnabled:         false
            property bool _panEnabled:          false
            property bool _rollEnabled:         false

            property bool _servoReverseIsBool:  controller.parameterExists(-1, "RC5_REVERSED")

            // Gimbal Settings not available on older firmware
            property bool _showGimbaLSettings:  controller.parameterExists(-1, "MNT_DEFLT_MODE")

            readonly property real  _margins:                       ScreenTools.defaultFontPixelHeight
            readonly property int   _rcFunctionDisabled:            0
            readonly property int   _rcFunctionMountPan:            6
            readonly property int   _rcFunctionMountTilt:           7
            readonly property int   _rcFunctionMountRoll:           8
            readonly property int   _firstGimbalOutChannel:         5
            readonly property int   _lastGimbalOutChannel:          14
            readonly property int   _mountDefaultModeRCTargetting:  3

            Component.onCompleted: {
                if (_showGimbaLSettings) {
                    gimbalSettingsLoader.sourceComponent = gimbalSettings
                }
                calcGimbalOutValues()
                slide.minimumValue = 10
                slide.maximumValue = 127
                slide.value = slide._fact.value
                slide._loadComplete = true
            }

            function setGimbalSettingsServoInfo(loader, channel) {
                var rcPrefix = "r.SERVO" + channel + "_"

                loader.gimbalOutIndex = channel - 4
                loader.servoPWMMinFact = controller.getParameterFact(-1, rcPrefix + "MIN")
                loader.servoPWMMaxFact = controller.getParameterFact(-1, rcPrefix + "MAX")
                loader.servoReverseFact = controller.getParameterFact(-1, rcPrefix + "REVERSED")
            }

            /// Gimbal output channels are stored in SERVO#_FUNCTION parameters. We need to loop through those
            /// to find them and setup the ui accordindly.
            function calcGimbalOutValues() {
                gimbalDirectionTiltLoader.gimbalOutIndex = 0
                gimbalDirectionPanLoader.gimbalOutIndex = 0
                gimbalDirectionRollLoader.gimbalOutIndex = 0
                _tiltEnabled = false
                _panEnabled = false
                _rollEnabled = false
                for (var channel=_firstGimbalOutChannel; channel<=_lastGimbalOutChannel; channel++) {
                    var functionFact = controller.getParameterFact(-1, "r.SERVO" + channel + "_FUNCTION")
                    if (functionFact.value === _rcFunctionMountTilt) {
                        _tiltEnabled = true
                        setGimbalSettingsServoInfo(gimbalDirectionTiltLoader, channel)
                    } else if (functionFact.value === _rcFunctionMountPan) {
                        _panEnabled = true
                        setGimbalSettingsServoInfo(gimbalDirectionPanLoader, channel)
                    } else if (functionFact.value === _rcFunctionMountRoll) {
                        _rollEnabled = true
                        setGimbalSettingsServoInfo(gimbalDirectionRollLoader, channel)
                    }
                }
            }

            function setRCFunction(channel, rcFunction) {
                // First clear any previous settings for this function
                for (var index=_firstGimbalOutChannel; index<=_lastGimbalOutChannel; index++) {
                    var functionFact = controller.getParameterFact(-1, "r.SERVO" + index + "_FUNCTION")
                    if (functionFact.value !== _rcFunctionDisabled && functionFact.value === rcFunction) {
                        functionFact.value = _rcFunctionDisabled
                    }
                }

                // Now set the function into the new channel
                if (channel !== 0) {
                    var functionFact = controller.getParameterFact(-1, "r.SERVO" + channel + "_FUNCTION")
                    functionFact.value = rcFunction
                }
            }

            // Whenever any SERVO#_FUNCTION parameters changes we need to go looking for gimbal output channels again
            Connections { target: _rc5Function; onValueChanged: calcGimbalOutValues() }
            Connections { target: _rc6Function; onValueChanged: calcGimbalOutValues() }
            Connections { target: _rc7Function; onValueChanged: calcGimbalOutValues() }
            Connections { target: _rc8Function; onValueChanged: calcGimbalOutValues() }
            Connections { target: _rc9Function; onValueChanged: calcGimbalOutValues() }
            Connections { target: _rc10Function; onValueChanged: calcGimbalOutValues() }
            Connections { target: _rc11Function; onValueChanged: calcGimbalOutValues() }
            Connections { target: _rc12Function; onValueChanged: calcGimbalOutValues() }
            Connections { target: _rc13Function; onValueChanged: calcGimbalOutValues() }
            Connections { target: _rc14Function; onValueChanged: calcGimbalOutValues() }

            // Whenever an MNT_RC_IN_* setting is changed make sure to turn on RC targeting
            Connections {
                target:         _mountRCInPan
                onValueChanged: if(_mountDefaultMode) _mountDefaultMode.value = _mountDefaultModeRCTargetting
            }

            Connections {
                target:         _mountRCInRoll
                onValueChanged: if(_mountDefaultMode) _mountDefaultMode.value = _mountDefaultModeRCTargetting
            }

            Connections {
                target:         _mountRCInTilt
                onValueChanged: if(_mountDefaultMode) _mountDefaultMode.value = _mountDefaultModeRCTargetting
            }

            ListModel {
                id: gimbalOutModel
                ListElement { text: qsTr("Disabled");   value: 0 }
                ListElement { text: qsTr("Channel 5");  value: 5 }
                ListElement { text: qsTr("Channel 6");  value: 6 }
                ListElement { text: qsTr("Channel 7");  value: 7 }
                ListElement { text: qsTr("Channel 8");  value: 8 }
                ListElement { text: qsTr("Channel 9");  value: 9 }
                ListElement { text: qsTr("Channel 10"); value: 10 }
                ListElement { text: qsTr("Channel 11"); value: 11 }
                ListElement { text: qsTr("Channel 12"); value: 12 }
                ListElement { text: qsTr("Channel 13"); value: 13 }
                ListElement { text: qsTr("Channel 14"); value: 14 }
            }

            QGCCheckBox {
                id:     _allVisible
                text:   "Show all settings (advanced)"
            }

            QGCLabel {
                visible:     !_oldFW
                text:        "Camera mount tilt speed:"
                font.family: ScreenTools.demiboldFontFamily
            }

            QGCSlider {
                visible:    !_oldFW
                id:         slide
                width:      gimbalDirectionTiltLoader.width
                stepSize:   _fact.increment ? _fact.increment : 1

                property var  _fact:            controller.getParameterFact(-1, "MNT_JSTICK_SPD")
                property bool _loadComplete:    false

                // Override style to make handles smaller than default
                style: SliderStyle {
                    handle: Rectangle {
                        anchors.centerIn:   parent
                        color:              qgcPal.button
                        border.color:       qgcPal.buttonText
                        border.width:       1
                        implicitWidth:      _radius * 2
                        implicitHeight:     _radius * 2
                        radius:             _radius

                        property real _radius: Math.round(ScreenTools.defaultFontPixelHeight * 0.35)
                    }
                }

                onValueChanged: {
                    if (_loadComplete) {
                        _fact.value = value
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onWheel: {
                        // do nothing
                        wheel.accepted = true;
                    }
                    onPressed: {
                        // propogate/accept
                        mouse.accepted = false;
                    }
                    onReleased: {
                        // propogate/accept
                        mouse.accepted = false;
                    }
                }
            }

            // Gimbal axis setup
            Component {
                id: gimbalDirectionSettings

                // The following properties must be set in the Loader
                //      property string directionTitle
                //      property bool directionEnabled
                //      property int gimbalOutIndex
                //      property Fact mountRcInFact
                //      property Fact mountStabFact
                //      property Fact mountAngMinFact
                //      property Fact mountAngMaxFact
                //      property Fact servoPWMMinFact
                //      property Fact servoPWMMaxFact
                //      property Fact servoReverseFact
                //      property bool servoReverseIsBool
                //      property int rcFunction

                // Each section is a row
                Row {

                    // Stack section title and section backdrop/content
                    Column {
                        spacing: _margins/2

                        // Section Title
                        QGCLabel {
                            id:          directionLabel
                            text:        qsTr("Gimbal ") + directionTitle
                            font.family: ScreenTools.demiboldFontFamily
                        }

                        // Section Backdrop
                        Rectangle {
                            id:     rectangle
                            height: innerColumn.height + _margins*2
                            width:  innerColumn.width + _margins*2
                            color:  palette.windowShade

                            // Section Content - 3 Rows
                            Column {
                                id:                 innerColumn
                                spacing:            _margins
                                anchors.margins:    _margins
                                anchors.top:        rectangle.top
                                anchors.left:       rectangle.left

                                // Input/output channel setup and CheckBoxes
                                Row {
                                    spacing: _margins

                                    // Input/output channel setup
                                    Column {
                                        spacing: _margins

                                        Row {
                                            spacing: _margins

                                            QGCLabel {
                                                id:               gimbalOutLabel
                                                anchors.baseline: outputChan.baseline
                                                text:             qsTr("Output channel:")
                                            }

                                            QGCComboBox {
                                                id:             outputChan
                                                width:          servoPWMMinField.width
                                                model:          gimbalOutModel
                                                textRole:       "text"
                                                currentIndex:   gimbalOutIndex

                                                onActivated: setRCFunction(gimbalOutModel.get(index).value, rcFunction)
                                            }
                                        }

                                        Component.onCompleted: {
                                            mountRcInFact.value = hardCodedChanned
                                        }
                                    } // Column - Input/output channel setup

                                    // CheckBoxes
                                    Column {
                                        spacing: _margins
                                        enabled: directionEnabled

                                        FactCheckBox {
                                            text:           qsTr("Servo reverse")
                                            checkedValue:   _servoReverseIsBool ? 1 : -1
                                            uncheckedValue: _servoReverseIsBool ? 0 : 1
                                            fact:           servoReverseFact

                                            property bool _servoReverseIsBool: servoReverseIsBool
                                        }

                                        FactCheckBox {
                                            anchors.margins: _margins
                                            text:            qsTr("Stabilize")
                                            fact:            mountStabFact
                                            checkedValue:    1
                                            uncheckedValue:  0
                                            visible:         _allVisible.checked
                                        }
                                    } // Column - CheckBoxes
                                } // Row input/output setup and CheckBoxes

                                // Servo PWM Limits
                                Row {
                                    id:      servoLimitRow
                                    spacing: _margins
                                    enabled: directionEnabled

                                    property var _labelBaseline: servoPWMMinField.baseline

                                    QGCLabel {
                                        text:             qsTr("Servo PWM limits:")
                                        anchors.baseline: servoLimitRow._labelBaseline
                                        width:            angleLimitLabel.width
                                    }

                                    QGCLabel {
                                        text:             qsTr("min")
                                        anchors.baseline: servoLimitRow._labelBaseline
                                    }

                                    FactTextField {
                                        id:   servoPWMMinField
                                        fact: servoPWMMinFact
                                    }

                                    QGCLabel {
                                        text:             qsTr("max")
                                        anchors.baseline: servoLimitRow._labelBaseline
                                    }

                                    FactTextField {
                                        fact: servoPWMMaxFact
                                    }
                                } // Row - Servo PWM limits

                                // Gimbal angle limits
                                Row {
                                    id:      angleLimitRow
                                    spacing: _margins
                                    enabled: directionEnabled
                                    visible: _allVisible.checked

                                    property var _labelBaseline: mountAngMinField.baseline

                                    QGCLabel {
                                        id:                 angleLimitLabel
                                        text:               qsTr("Gimbal angle limits:")
                                        anchors.baseline:   angleLimitRow._labelBaseline
                                    }

                                    QGCLabel {
                                        text:               qsTr("min")
                                        anchors.baseline:   angleLimitRow._labelBaseline
                                    }

                                    FactTextField {
                                        id:   mountAngMinField
                                        fact: mountAngMinFact
                                    }

                                    QGCLabel {
                                        text:              qsTr("max")
                                        anchors.baseline:  angleLimitRow._labelBaseline
                                    }

                                    FactTextField {
                                        fact: mountAngMaxFact
                                    }
                                } // Row - Gimbal angle limits
                            } // Column - Section content
                        }// Rectangle - Section backdrop
                    } // Column - Stack section title and section backdrop/content
                } // Row - section
            } // Component - gimbalDirectionSettings

            Component {
                id: gimbalSettings

                Item {
                    width:  rectangle.x + rectangle.width
                    height: rectangle.y + rectangle.height

                    property Fact _mountDefaultMode:    controller.getParameterFact(-1, "MNT_DEFLT_MODE")
                    property Fact _mountType:           controller.getParameterFact(-1, "MNT_TYPE")

                    QGCLabel {
                        id:             settingsLabel
                        text:           qsTr("Gimbal Settings")
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        id:                 rectangle
                        anchors.topMargin:  _margins / 2
                        anchors.top:        settingsLabel.bottom
                        width:              gimbalModeCombo.x + gimbalModeCombo.width + _margins
                        height:             gimbalModeCombo.y + gimbalModeCombo.height + _margins
                        color:              palette.windowShade

                        QGCLabel {
                            id:                 gimbalTypeLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.baseline:   gimbalTypeCombo.baseline
                            text:               qsTr("Type:")
                        }

                        FactComboBox {
                            id:                 gimbalTypeCombo
                            anchors.topMargin:  _margins
                            anchors.top:        parent.top
                            anchors.left:       gimbalModeCombo.left
                            width:              gimbalModeCombo.width
                            fact:               _mountType
                            indexModel:         false
                        }

                        QGCLabel {
                            id:                     rebootLabel
                            anchors.topMargin:      _margins / 2
                            anchors.leftMargin:     _margins
                            anchors.rightMargin:    _margins
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            anchors.top:            gimbalTypeCombo.bottom
                            wrapMode:               Text.WordWrap
                            text:                   qsTr("Gimbal Type changes takes affect next reboot of autopilot")
                        }

                        QGCLabel {
                            id:                 gimbalModeLabel
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.baseline:   gimbalModeCombo.baseline
                            text:               qsTr("Default Mode:")
                        }

                        FactComboBox {
                            id:                 gimbalModeCombo
                            anchors.margins:    _margins
                            anchors.top:        rebootLabel.bottom
                            anchors.left:       gimbalModeLabel.right
                            width:              ScreenTools.defaultFontPixelWidth * 15
                            fact:               _mountDefaultMode
                            indexModel:         false
                        }
                    } // Rectangle
                } // Item
            } // Component - gimbalSettings

            Loader {
                id:                 gimbalDirectionTiltLoader
                sourceComponent:    gimbalDirectionSettings

                property int    hardCodedChanned:   8 // ArduSub/joystick.cpp cam_tilt
                property string directionTitle:     qsTr("Tilt")
                property bool   directionEnabled:   _tiltEnabled
                property int    gimbalOutIndex:     0
                property Fact   mountRcInFact:      _mountRCInTilt
                property Fact   mountStabFact:      _mountStabTilt
                property Fact   mountAngMinFact:    _mountAngMinTilt
                property Fact   mountAngMaxFact:    _mountAngMaxTilt
                property Fact   servoPWMMinFact:    Fact { }
                property Fact   servoPWMMaxFact:    Fact { }
                property Fact   servoReverseFact:   Fact { }
                property bool   servoReverseIsBool: _servoReverseIsBool
                property int    rcFunction:         _rcFunctionMountTilt
            }

            Loader {
                id:                 gimbalDirectionRollLoader
                sourceComponent:    gimbalDirectionSettings
                visible:            _allVisible.checked

                property int    hardCodedChanned:   0 // ArduSub/joystick.cpp cam_roll does not exist
                property string directionTitle:     qsTr("Roll")
                property bool   directionEnabled:   _rollEnabled
                property int    gimbalOutIndex:     0
                property Fact   mountRcInFact:      _mountRCInRoll
                property Fact   mountStabFact:      _mountStabRoll
                property Fact   mountAngMinFact:    _mountAngMinRoll
                property Fact   mountAngMaxFact:    _mountAngMaxRoll
                property Fact   servoPWMMinFact:    Fact { }
                property Fact   servoPWMMaxFact:    Fact { }
                property Fact   servoReverseFact:   Fact { }
                property bool   servoReverseIsBool: _servoReverseIsBool
                property int    rcFunction:         _rcFunctionMountRoll
            }

            Loader {
                id:                 gimbalDirectionPanLoader
                sourceComponent:    gimbalDirectionSettings
                visible:            _allVisible.checked

                property int    hardCodedChanned:   7 // ArduSub/joystick.cpp cam_pan
                property string directionTitle:     qsTr("Pan")
                property bool   directionEnabled:   _panEnabled
                property int    gimbalOutIndex:     0
                property Fact   mountRcInFact:      _mountRCInPan
                property Fact   mountStabFact:      _mountStabPan
                property Fact   mountAngMinFact:    _mountAngMinPan
                property Fact   mountAngMaxFact:    _mountAngMaxPan
                property Fact   servoPWMMinFact:    Fact { }
                property Fact   servoPWMMaxFact:    Fact { }
                property Fact   servoReverseFact:   Fact { }
                property bool   servoReverseIsBool: _servoReverseIsBool
                property int    rcFunction:         _rcFunctionMountPan
            }

            Loader {
                id:         gimbalSettingsLoader
                visible:    _allVisible.checked
            }

        } // Column
    } // Component
} // SetupPage
