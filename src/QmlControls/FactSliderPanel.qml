/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.5
import QtQuick.Controls     1.4

import QGroundControl.FactSystem    1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Column {
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

    property var qgcViewPanel

    property real _margins:         ScreenTools.defaultFontPixelHeight
    property bool _loadComplete:    false

    FactPanelController {
        id:         controller
        factPanel:  qgcViewPanel
    }

    QGCPalette { id: palette; colorGroupEnabled: enabled }

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

    QGCLabel {
        id:             panelLabel
        text:           panelTitle
        font.family:    ScreenTools.demiboldFontFamily
    }

    Column {
        id:                 sliderOuterColumn
        anchors.left:       parent.left
        anchors.right:      parent.right
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
                        font.family:    ScreenTools.demiboldFontFamily
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
} // QGCView
