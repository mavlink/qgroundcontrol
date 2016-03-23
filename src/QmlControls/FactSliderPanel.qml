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
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    viewPanel: panel

    property string panelTitle: "Title" ///< Title for panel

    /// ListModel must contains elements which look like this:
    ///     ListElement {
    ///         title:          "Roll sensitivity"
    ///         description:    "Slide to the left to make roll control faster and more accurate. Slide to the right if roll oscillates or is too twitchy."
    ///         param:          "MC_ROLL_TC"
    ///         min:            0
    ///         max:            100
    ///         step:           1
    ///     }
    property ListModel sliderModel

    FactPanelController { id: controller; factPanel: panel }

    QGCPalette { id: palette; colorGroupEnabled: enabled }
    property real _margins: ScreenTools.defaultFontPixelHeight

    property bool _loadComplete: false

    Component.onCompleted: {
        // Qml Sliders have a strange behavior in which they first set Slider::value to some internal
        // setting and then set Slider::value to the bound properties value. If you have an onValueChanged
        // handler which updates your property with the new value, this first value change will trash
        // your bound values. In order to work around this we don't set the values into the Sliders until
        // after Qml load is done. We also don't track value changes until Qml load completes.
        for (var i=0; i<sliderModel.count; i++) {
            sliderRepeater.itemAt(i).sliderValue = controller.getParameterFact(-1, sliderModel.get(i).param).value
        }
        _loadComplete = true
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      sliderOuterColumn.y + sliderOuterColumn.height
            flickableDirection: Flickable.VerticalFlick

            QGCLabel {
                id:             panelLabel
                text:           panelTitle
                font.weight:    Font.DemiBold
            }


            Column {
                id:                 sliderOuterColumn
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        panelLabel.bottom
                spacing:            _margins

                Repeater {
                    id:     sliderRepeater
                    model:  sliderModel

                    Rectangle {
                        id:                 sliderRect
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        height:             sliderColumn.y + sliderColumn.height + _margins
                        color:              palette.windowShade

                        property alias sliderValue: slider.value

                        Column {
                            id:                 sliderColumn
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            anchors.top:        sliderRect.top

                            QGCLabel {
                                text:           title
                                font.weight:    Font.DemiBold
                            }

                            QGCLabel {
                                text:           description
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                            }

                            Slider {
                                id:                 slider
                                anchors.left:       parent.left
                                anchors.right:      parent.right
                                minimumValue:       min
                                maximumValue:       max
                                stepSize:           isNaN(fact.increment) ? step : fact.increment
                                tickmarksEnabled:   true

                                property Fact fact: controller.getParameterFact(-1, param)

                                onValueChanged: {
                                    if (_loadComplete) {
                                        fact.value = value
                                    }
                                }
                            } // Slider
                        } // Column
                    } // Rectangle
                } // Repeater
            } // Column
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView
