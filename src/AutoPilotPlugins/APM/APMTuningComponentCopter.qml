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
    property Fact _rateRollP:   controller.getParameterFact(-1, "RATE_RLL_P")
    property Fact _rateRollI:   controller.getParameterFact(-1, "RATE_RLL_I")
    property Fact _ratePitchP:  controller.getParameterFact(-1, "RATE_PIT_P")
    property Fact _ratePitchI:  controller.getParameterFact(-1, "RATE_PIT_I")

    property real _margins: ScreenTools.defaultFontPixelHeight

    ExclusiveGroup { id: fenceActionRadioGroup }
    ExclusiveGroup { id: landLoiterRadioGroup }
    ExclusiveGroup { id: returnAltRadioGroup }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Flickable {
            clip:               true
            anchors.fill:       parent
            boundsBehavior:     Flickable.StopAtBounds
            contentHeight:      basicTuning.y + basicTuning.height
            flickableDirection: Flickable.VerticalFlick

            QGCLabel {
                id:         basicLabel
                text:       "Basic Tuning"
                font.weight: Font.DemiBold
            }

            Rectangle {
                id:                 basicTuning
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
                            text:       "Throttle Hover"
                            font.weight: Font.DemiBold
                        }

                        QGCLabel {
                            text: "How much throttle is needed to maintain a steady hover"
                        }

                        Slider {
                            id:                 throttleHover
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            minimumValue:       200
                            maximumValue:       800
                            stepSize:           1.0
                            value:              _throttleMid.value
                        }
                    }

                    Column {
                        anchors.left:   parent.left
                        anchors.right:  parent.right

                        QGCLabel {
                            text:       "Roll/Pitch Sensitivity"
                            font.weight: Font.DemiBold
                        }

                        QGCLabel {
                            text: "Slide to the right if the copter is sluggish or slide to the left if the copter is twitchy"
                        }

                        Slider {
                            id:                 rollPitch
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            minimumValue:       0.08
                            maximumValue:       0.4
                            value:              _rateRollP.value

                            onValueChanged: {
                                _rateRollI.value = value
                                _ratePitchP.value = value
                                _ratePitchI.value = value
                            }
                        }
                    }

                    Column {
                        anchors.left:   parent.left
                        anchors.right:  parent.right

                        QGCLabel {
                            text:       "RC Roll/Pitch Feel"
                            font.weight: Font.DemiBold
                        }

                        QGCLabel {
                            text: "Slide to the left for soft control, slide to the right for crisp control"
                        }

                        Slider {
                            id:                 rcFeel
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            minimumValue:       0
                            maximumValue:       100
                            stepSize:           1.0
                            value:              _rcFeel.value
                        }
                    }
                }
            } // Rectangle - Basic tuning
        } // Flickable
    } // QGCViewPanel
} // QGCView
