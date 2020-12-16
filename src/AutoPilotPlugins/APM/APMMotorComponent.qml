/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             motorPage
    pageComponent:  pageComponent

    readonly property int _barHeight:           10
    readonly property int _barWidth:            5
    readonly property int _sliderHeight:        10
    readonly property int _motorTimeoutSecs:    3

    FactPanelController {
        id:             controller
    }

    Component {
        id: pageComponent

        Column {
            spacing: ScreenTools.defaultFontPixelHeight

            QGCLabel {
                text:       qsTr("Warning: Unable to determine motor count")
                color:      qgcPal.warningText
                visible:    controller.vehicle.motorCount == -1
            }

            Row {
                id:         motorSliders
                enabled:    safetySwitch.checked
                spacing:    ScreenTools.defaultFontPixelWidth * 4

                Repeater {
                    id:         sliderRepeater
                    model:      controller.vehicle.motorCount == -1 ? 8 : controller.vehicle.motorCount

                    Column {
                        property alias motorSlider: slider

                        QGCLabel {
                            anchors.horizontalCenter:   parent.horizontalCenter
                            text:                       vehicleComponent.motorIndexToLetter(index)
                        }

                        QGCSlider {
                            id:                         slider
                            height:                     ScreenTools.defaultFontPixelHeight * _sliderHeight
                            orientation:                Qt.Vertical
                            minimumValue:               0
                            maximumValue:               100
                            stepSize:                   1
                            value:                      0
                            updateValueWhileDragging:   false

                            onValueChanged: {
                                controller.vehicle.motorTest(index + 1, value, value == 0 ? 0 : _motorTimeoutSecs, true)
                                if (value != 0) {
                                    motorTimer.restart()
                                }
                            }

                            Timer {
                                id:             motorTimer
                                interval:       _motorTimeoutSecs * 1000
                                repeat:         false
                                running:        false

                                onTriggered: {
                                    allSlider.value = 0
                                    slider.value = 0
                                }
                            }
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
                        minimumValue:               0
                        maximumValue:               100
                        stepSize:                   1
                        value:                      0
                        updateValueWhileDragging:   false

                        onValueChanged: {
                            for (var sliderIndex=0; sliderIndex<sliderRepeater.count; sliderIndex++) {
                                sliderRepeater.itemAt(sliderIndex).motorSlider.value = allSlider.value
                            }
                        }
                    }
                } // Column
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
                    text:   safetySwitch.checked ? qsTr("Careful: Motor sliders are enabled") : qsTr("Propellers are removed - Enable motor sliders")
                }
            } // Row
        } // Column
    } // Component
} // SetupPahe
