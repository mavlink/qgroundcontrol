/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             motorPage
    pageComponent:  pageComponent

    readonly property int _barHeight:       10
    readonly property int _barWidth:        5
    readonly property int _sliderHeight:    10

    FactPanelController {
        id:             controller
        factPanel:      motorPage.viewPanel
    }

    Component {
        id: pageComponent

        Column {
            spacing: 10

            Row {
                id:         motorSliders
                enabled:    safetySwitch.checked
                spacing:    ScreenTools.defaultFontPixelWidth * 4

                Repeater {
                    id:         sliderRepeater
                    model:      controller.vehicle.motorCount == -1 ? 8 : controller.vehicle.motorCount

                    Column {
                        property alias motorSlider: slider

                        Timer {
                            interval:       250
                            running:        true
                            repeat:         true

                            property real _lastValue: 0

                            onTriggered: {
                                if (_lastValue != slider.value) {
                                    controller.vehicle.motorTest(index + 1, slider.value, 1)
                                }
                            }
                        }

                        QGCLabel {
                            anchors.horizontalCenter:   parent.horizontalCenter
                            text:                       index + 1
                        }

                        QGCSlider {
                            id:                         slider
                            height:                     ScreenTools.defaultFontPixelHeight * _sliderHeight
                            orientation:                Qt.Vertical
                            maximumValue:               100
                            value:                      0
                        }
                    } // Column
                } // Repeater

                Column {
                    QGCLabel {
                        anchors.horizontalCenter:   parent.horizontalCenter
                        text:                       qsTr("All")
                    }

                    QGCSlider {
                        id:                         allSlider
                        height:                     ScreenTools.defaultFontPixelHeight * _sliderHeight
                        orientation:                Qt.Vertical
                        maximumValue:               100
                        value:                      0

                        onValueChanged: {
                            for (var sliderIndex=0; sliderIndex<sliderRepeater.count; sliderIndex++) {
                                sliderRepeater.itemAt(sliderIndex).motorSlider.value = allSlider.value
                            }
                        }
                    }
                } // Column

                MultiRotorMotorDisplay {
                    anchors.top:    parent.top
                    anchors.bottom: parent.bottom
                    width:          height
                    motorCount:     controller.vehicle.motorCount
                    xConfig:        controller.vehicle.xConfigMotors
                    coaxial:        controller.vehicle.coaxialMotors
                }

            } // Row

            QGCLabel {
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                text:           qsTr("Moving the sliders will causes the motors to spin. Make sure you remove all props.")
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                Switch {
                    id: safetySwitch
                    onClicked: {
                        if (!checked) {
                            for (var sliderIndex=0; sliderIndex<sliderRepeater.count; sliderIndex++) {
                                sliderRepeater.itemAt(sliderIndex).motorSlider.value = 0
                            }
                            allSlider.value = 0
                        }
                    }
                }

                QGCLabel {
                    color:  qgcPal.warningText
                    text:   qsTr("Propellers are removed - Enable motor sliders")
                }
            } // Row
        } // Column
    } // Component
} // SetupPahe
