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

import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools

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

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

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
                color:              qgcPal.windowShade

                Column {
                    id:                 sliderColumn
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        sliderRect.top

                    QGCLabel {
                        text:           title
                        font.bold:      true
                    }
                    Item {
                        width: 1
                        height: _margins
                    }

                    Slider {
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        from:       min
                        to:       max
                        stepSize:           step
                        //tickmarksEnabled:   true  // FIXME_QT6
                        value:              _fact.value
                        id:                 slider
                        property int handleWidth: 0

                        property Fact _fact: controller.getParameterFact(-1, param)

                        onValueChanged: {
                            if (_loadComplete && enabled) {
                                _fact.value = value
                            }
                        }

                        // FIXME-QT6 - Controls 2 doesn't do styling this way
                        /*
                        style: SliderStyle {
                            tickmarks: Repeater {
                                id: repeater
                                model: control.stepSize > 0 ? 1 + (control.to - control.from) / control.stepSize : 0
                                property int unused: get()
                                function get() {
                                    slider.handleWidth = styleData.handleWidth
                                    return 0
                                }

                                Rectangle {
                                    color: Qt.hsla(qgcPal.text.hslHue, qgcPal.text.hslSaturation, qgcPal.text.hslLightness, 0.5)
                                    width: 2
                                    height: 4
                                    y: repeater.height
                                    x: styleData.handleWidth / 2 + index * ((repeater.width - styleData.handleWidth) / (repeater.count-1))
                                }
                            }
                        }
                        */
                    }

                    Item { // spacing
                        width: 1
                        height: 4
                    }

                    RowLayout {
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        QGCLabel {
                            id: leftValueLabel
                            text: slider.from
                            horizontalAlignment: Text.AlignLeft
                        }
                        Item {
                            QGCLabel {
                                visible: slider.value != slider.from && slider.value != slider.to
                                text: Math.round(slider._fact.value*100000)/100000
                                x: getX()
                                function getX() {
                                    var span = slider.to - slider.from
                                    var x = slider.handleWidth / 2 + (slider.value-slider.from)/span * (slider.width-slider.handleWidth) - width / 2
                                    // avoid overlapping text
                                    var minX = leftValueLabel.x + leftValueLabel.width + _margins/2
                                    if (x < minX) x = minX
                                    var maxX = rightValueLabel.x - _margins/2 - width
                                    if (x > maxX) x = maxX
                                    return x
                                }
                            }
                        }
                        QGCLabel {
                            id: rightValueLabel
                            text: slider.to
                            Layout.alignment: Qt.AlignRight
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
