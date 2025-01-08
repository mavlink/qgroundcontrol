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
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.ScreenTools

SetupPage {
    id:             motorPage
    pageComponent:  pageComponent

    readonly property int _barHeight:           10
    readonly property int _barWidth:            5
    readonly property int _sliderWidth:         15
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
                Column {

                    QGCLabel {
                        id:                         labelThrottle
                        anchors.horizontalCenter:   parent.horizontalCenter
                        text:                       qsTr("Throttle") + " " + sliderThrottle.value + " " + qsTr("%")
                    }

                    Row {
                        id:         motorSlider
                        enabled:    safetySwitch.checked
                        spacing:    ScreenTools.defaultFontPixelWidth * 4
                        
                        QGCButton {
                            anchors.verticalCenter:     parent.verticalCenter
                            id:         minusMinusButton
                            text:       qsTr("-")
                            onClicked: sliderThrottle.value-=1
                        }

                        QGCSlider {
                            id:                         sliderThrottle
                            width:                      ScreenTools.defaultFontPixelHeight * _sliderWidth
                            orientation:                Qt.Horizontal
                            anchors.verticalCenter:     parent.verticalCenter
                            from:                       0
                            to:                         100
                            stepSize:                   1
                            value:                      0
                            live:                       false
                            displayValue:               true
                        }

                        QGCButton {
                            anchors.verticalCenter:     parent.verticalCenter
                            id:         plusButton
                            text:       qsTr("+")
                            onClicked: sliderThrottle.value+=1
                        }
                    } // Row
                } // Column
            } // Row

            QGCLabel {
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                text:           qsTr("Make sure you remove all props.")
            }

            Row {
                id:         motorButtons
                enabled:    safetySwitch.checked
                spacing:    ScreenTools.defaultFontPixelWidth * 4

                Repeater {
                    id:         buttonRepeater
                    model:      controller.vehicle.motorCount === -1 ? 8 : controller.vehicle.motorCount

                    QGCButton {
                        id:         button
                        anchors.verticalCenter:     parent.verticalCenter
                        text:       index + 1
                        onClicked:  {
                            controller.vehicle.motorTest(index + 1, sliderThrottle.value, sliderThrottle.value === 0 ? 0 : _motorTimeoutSecs, true)
                        }
                    }
                } // Repeater

                QGCButton {
                    id:         allButton
                    text:       qsTr("All")
                    onClicked:  {
                        for (var motorIndex=0; motorIndex<buttonRepeater.count; motorIndex++) {
                            controller.vehicle.motorTest(motorIndex + 1, sliderThrottle.value, sliderThrottle.value === 0 ? 0 : _motorTimeoutSecs, true)
                        }
                    }
                }

                QGCButton {
                    id:         allStopButton
                    text:       qsTr("Stop")
                    onClicked:  {
                        for (var motorIndex=0; motorIndex<buttonRepeater.count; motorIndex++) {
                            controller.vehicle.motorTest(motorIndex + 1, 0, 0, true)
                        }
                    }
                }
            } // Row

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                Switch {
                    id: safetySwitch
                    onClicked: {
                        if (!checked) {
                            sliderThrottle.value = 0
                        }
                    }
                }

                QGCLabel {
                    anchors.verticalCenter:     parent.verticalCenter
                    color:  qgcPal.warningText
                    text:   safetySwitch.checked ? qsTr("Careful : Motors are enabled") : qsTr("Propellers are removed - Enable slider and motors")
                }
            } // Row
        } // Column
    } // Component
} // SetupPage
