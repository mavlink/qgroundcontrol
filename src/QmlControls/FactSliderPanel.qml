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

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
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

    property real _margins:         ScreenTools.defaultFontPixelHeight
    property bool _loadComplete:    false

    Component.onCompleted: _loadComplete = true

    FactPanelController {
        id: controller
    }

    QGCPalette { id: palette; colorGroupEnabled: enabled }

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

                Column {
                    id:                 sliderColumn
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        sliderRect.top
                    spacing:            _margins

                    QGCLabel {
                        text:           title
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Slider {
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        minimumValue:       min
                        maximumValue:       max
                        stepSize:           step
                        tickmarksEnabled:   true
                        value:              _fact.value

                        property Fact _fact: controller.getParameterFact(-1, param)

                        onValueChanged: {
                            if (_loadComplete) {
                                _fact.value = value
                            }
                        }
                    }

                    QGCLabel {
                        text:           description
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                    }
                }
            }
        }
    }
}
