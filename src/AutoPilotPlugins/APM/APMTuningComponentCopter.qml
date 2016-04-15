/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick              2.5
import QtQuick.Controls     1.4

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    id:                 _safetyView
    viewPanel:          panel
    anchors.fill:       parent

    FactPanelController { id: controller; factPanel: panel }

    QGCPalette { id: palette; colorGroupEnabled: enabled }

    property Fact _throttleMid: controller.getParameterFact(-1, "THR_MID")
    property Fact _rcFeel:      controller.getParameterFact(-1, "RC_FEEL_RP")
    property Fact _rateRollP:   controller.getParameterFact(-1, "r.ATC_RAT_RLL_P")
    property Fact _rateRollI:   controller.getParameterFact(-1, "r.ATC_RAT_RLL_I")
    property Fact _ratePitchP:  controller.getParameterFact(-1, "r.ATC_RAT_PIT_P")
    property Fact _ratePitchI:  controller.getParameterFact(-1, "r.ATC_RAT_PIT_I")
    property Fact _rateClimbP:  controller.getParameterFact(-1, "ACCEL_Z_P")
    property Fact _rateClimbI:  controller.getParameterFact(-1, "ACCEL_Z_I")

    property Fact _ch7Opt:  controller.getParameterFact(-1, "CH7_OPT")
    property Fact _ch8Opt:  controller.getParameterFact(-1, "CH8_OPT")
    property Fact _ch9Opt:  controller.getParameterFact(-1, "CH9_OPT")
    property Fact _ch10Opt: controller.getParameterFact(-1, "CH10_OPT")
    property Fact _ch11Opt: controller.getParameterFact(-1, "CH11_OPT")
    property Fact _ch12Opt: controller.getParameterFact(-1, "CH12_OPT")

    readonly property int   _firstOptionChannel:    7
    readonly property int   _lastOptionChannel:     12

    property Fact   _autoTuneAxes:                  controller.getParameterFact(-1, "AUTOTUNE_AXES")
    property int    _autoTuneSwitchChannelIndex:    0
    readonly property int _autoTuneOption:          17

    property real _margins: ScreenTools.defaultFontPixelHeight

    property bool _loadComplete: false

    ExclusiveGroup { id: fenceActionRadioGroup }
    ExclusiveGroup { id: landLoiterRadioGroup }
    ExclusiveGroup { id: returnAltRadioGroup }

    Component.onCompleted: {
        // Qml Sliders have a strange behavior in which they first set Slider::value to some internal
        // setting and then set Slider::value to the bound properties value. If you have an onValueChanged
        // handler which updates your property with the new value, this first value change will trash
        // your bound values. In order to work around this we don't set the values into the Sliders until
        // after Qml load is done. We also don't track value changes until Qml load completes.
        throttleHover.value = _throttleMid.value
        rollPitch.value = _rateRollP.value
        climb.value = _rateClimbP.value
        rcFeel.value = _rcFeel.value
        _loadComplete = true

        calcAutoTuneChannel()
    }

    /// The AutoTune switch is stored in one of the RC#_FUNCTION parameters. We need to loop through those
    /// to find them and setup the ui accordindly.
    function calcAutoTuneChannel() {
        _autoTuneSwitchChannelIndex = 0
        for (var channel=_firstOptionChannel; channel<=_lastOptionChannel; channel++) {
            var optionFact = controller.getParameterFact(-1, "CH" + channel + "_OPT")
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
            var optionFact = controller.getParameterFact(-1, "CH" + optionChannel + "_OPT")
            if (optionFact.value == _autoTuneOption) {
                optionFact.value = 0
            }
        }

        // Now set the function into the new channel
        if (channel != 0) {
            var optionFact = controller.getParameterFact(-1, "CH" + channel + "_OPT")
            optionFact.value = _autoTuneOption
        }
    }

    Connections { target: _ch7Opt; onValueChanged: calcAutoTuneChannel() }
    Connections { target: _ch8Opt; onValueChanged: calcAutoTuneChannel() }
    Connections { target: _ch9Opt; onValueChanged: calcAutoTuneChannel() }
    Connections { target: _ch10Opt; onValueChanged: calcAutoTuneChannel() }
    Connections { target: _ch11Opt; onValueChanged: calcAutoTuneChannel() }
    Connections { target: _ch12Opt; onValueChanged: calcAutoTuneChannel() }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      autoTuneRect.y + autoTuneRect.height
            flickableDirection: Flickable.VerticalFlick

            QGCLabel {
                id:         basicLabel
                text:       qsTr("Basic Tuning")
                font.weight: Font.DemiBold
            }

            Rectangle {
                id:                 basicTuningRect
                anchors.topMargin:  _margins / 2
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        basicLabel.bottom
                height:             basicTuningColumn.y + basicTuningColumn.height + _margins
                color:              palette.windowShade

                Column {
                    id:                 basicTuningColumn
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            _margins

                    Column {
                        anchors.left:   parent.left
                        anchors.right:  parent.right

                        QGCLabel {
                            text:       qsTr("Throttle Hover")
                            font.weight: Font.DemiBold
                        }

                        QGCLabel {
                            text: qsTr("How much throttle is needed to maintain a steady hover")
                        }

                        Slider {
                            id:                 throttleHover
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            minimumValue:       200
                            maximumValue:       800
                            stepSize:           10.0
                            tickmarksEnabled:   true

                            onValueChanged: {
                                if (_loadComplete) {
                                    _throttleMid.value = value
                                }
                            }
                        }
                    }

                    Column {
                        anchors.left:   parent.left
                        anchors.right:  parent.right

                        QGCLabel {
                            text:       qsTr("Roll/Pitch Sensitivity")
                            font.weight: Font.DemiBold
                        }

                        QGCLabel {
                            text: qsTr("Slide to the right if the copter is sluggish or slide to the left if the copter is twitchy")
                        }

                        Slider {
                            id:                 rollPitch
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            minimumValue:       0.08
                            maximumValue:       0.4
                            stepSize:           0.01
                            tickmarksEnabled:   true

                            onValueChanged: {
                                if (_loadComplete) {
                                    _rateRollP.value = value
                                    _rateRollI.value = value
                                    _ratePitchP.value = value
                                    _ratePitchI.value = value
                                }
                            }
                        }
                    }

                    Column {
                        anchors.left:   parent.left
                        anchors.right:  parent.right

                        QGCLabel {
                            text:       qsTr("Climb Sensitivity")
                            font.weight: Font.DemiBold
                        }

                        QGCLabel {
                            text: qsTr("Slide to the right to climb more aggressively or slide to the left to climb more gently")
                        }

                        Slider {
                            id:                 climb
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            minimumValue:       0.3
                            maximumValue:       1.0
                            stepSize:           0.02
                            tickmarksEnabled:   true
                            value:              _rateClimbP.value

                            onValueChanged: {
                                if (_loadComplete) {
                                    _rateClimbP.value = value
                                    _rateClimbI.value = value * 2
                                }
                            }
                        }
                    }

                    Column {
                        anchors.left:   parent.left
                        anchors.right:  parent.right

                        QGCLabel {
                            text:       qsTr("RC Roll/Pitch Feel")
                            font.weight: Font.DemiBold
                        }

                        QGCLabel {
                            text: qsTr("Slide to the left for soft control, slide to the right for crisp control")
                        }

                        Slider {
                            id:                 rcFeel
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            minimumValue:       0
                            maximumValue:       100
                            stepSize:           5.0
                            tickmarksEnabled:   true

                            onValueChanged: {
                                if (_loadComplete) {
                                    _rcFeel.value = value
                                }
                            }
                        }
                    }
                }
            } // Rectangle - Basic tuning

            QGCLabel {
                id:                 autoTuneLabel
                anchors.topMargin:  _margins
                anchors.top:        basicTuningRect.bottom
                text:               qsTr("AutoTune")
                font.weight:        Font.DemiBold
            }

            Rectangle {
                id:                 autoTuneRect
                anchors.topMargin:  _margins / 2
                anchors.left:       parent.left
                anchors.top:        autoTuneLabel.bottom
                width:              autoTuneColumn.x + autoTuneColumn.width + _margins
                height:             autoTuneColumn.y + autoTuneColumn.height + _margins
                color:              palette.windowShade

                Column {
                    id:                 autoTuneColumn
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    spacing:            _margins

                    Row {
                        spacing: _margins

                        QGCLabel { text: qsTr("Axes to AutoTune:") }
                        FactBitmask { fact: _autoTuneAxes }
                    }

                    Row {
                        spacing:    _margins

                        QGCLabel {
                            anchors.baseline:   autoTuneChannelCombo.baseline
                            text:               qsTr("Channel for AutoTune switch:")
                        }

                        QGCComboBox {
                            id:             autoTuneChannelCombo
                            width:          ScreenTools.defaultFontPixelWidth * 14
                            model:          [qsTr("None"), qsTr("Channel 7"), qsTr("Channel 8"), qsTr("Channel 9"), qsTr("Channel 10"), qsTr("Channel 11"), qsTr("Channel 12") ]
                            currentIndex:   _autoTuneSwitchChannelIndex

                            onActivated: {
                                var channel = index

                                if (channel > 0) {
                                    channel += 6
                                }
                                setChannelAutoTuneOption(channel)
                            }
                        }
                    }
                }
            } // Rectangle - AutoTune
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
