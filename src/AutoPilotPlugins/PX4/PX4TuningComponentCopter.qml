/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtCharts         2.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             tuningPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Column {
            width: availableWidth

            Component.onCompleted: {
                showAdvanced = !ScreenTools.isMobile
            }

            FactPanelController {
                id:         controller
                factPanel:  tuningPage.viewPanel
            }

            // Standard tuning page
            FactSliderPanel {
                width:          availableWidth
                qgcViewPanel:   tuningPage.viewPanel
                visible:        !advanced

                sliderModel: ListModel {
                    ListElement {
                        title:          qsTr("Hover Throttle")
                        description:    qsTr("Adjust throttle so hover is at mid-throttle. Slide to the left if hover is lower than throttle center. Slide to the right if hover is higher than throttle center.")
                        param:          "MPC_THR_HOVER"
                        min:            20
                        max:            80
                        step:           1
                    }

                    ListElement {
                        title:          qsTr("Manual minimum throttle")
                        description:    qsTr("Slide to the left to start the motors with less idle power. Slide to the right if descending in manual flight becomes unstable.")
                        param:          "MPC_MANTHR_MIN"
                        min:            0
                        max:            15
                        step:           1
                    }
                    /*
  These seem to have disappeared from PX4 firmware!
                    ListElement {
                        title:          qsTr("Roll sensitivity")
                        description:    qsTr("Slide to the left to make roll control faster and more accurate. Slide to the right if roll oscillates or is too twitchy.")
                        param:          "MC_ROLL_TC"
                        min:            0.15
                        max:            0.25
                        step:           0.01
                    }

                    ListElement {
                        title:          qsTr("Pitch sensitivity")
                        description:    qsTr("Slide to the left to make pitch control faster and more accurate. Slide to the right if pitch oscillates or is too twitchy.")
                        param:          "MC_PITCH_TC"
                        min:            0.15
                        max:            0.25
                        step:           0.01
                    }
*/
                }
            }

            Loader {
                anchors.left:       parent.left
                anchors.right:      parent.right
                sourceComponent:    advanced ? advancePageComponent : undefined
            }

            Component {
                id: advancePageComponent

                // Advanced page
                RowLayout {
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    layoutDirection:    Qt.RightToLeft

                    property real   _chartHeight:       ScreenTools.defaultFontPixelHeight * 20
                    property real   _margins:           ScreenTools.defaultFontPixelHeight / 2
                    property string _currentTuneType:   _tuneList[0]
                    property real   _roll:              _activeVehicle.roll.value
                    property real   _rollSetpoint:      _activeVehicle.setpoint.roll.value
                    property real   _rollRate:          _activeVehicle.rollRate.value
                    property real   _rollRateSetpoint:  _activeVehicle.setpoint.rollRate.value
                    property real   _pitch:             _activeVehicle.pitch.value
                    property real   _pitchSetpoint:     _activeVehicle.setpoint.pitch.value
                    property real   _pitchRate:         _activeVehicle.pitchRate.value
                    property real   _pitchRateSetpoint: _activeVehicle.setpoint.pitchRate.value
                    property real   _yaw:               _activeVehicle.heading.value
                    property real   _yawSetpoint:       _activeVehicle.setpoint.yaw.value
                    property real   _yawRate:           _activeVehicle.yawRate.value
                    property real   _yawRateSetpoint:   _activeVehicle.setpoint.yawRate.value
                    property var    _valueXAxis:        valueXAxis
                    property var    _valueRateXAxis:    valueRateXAxis
                    property var    _valueYAxis:        valueYAxis
                    property var    _valueRateYAxis:    valueRateYAxis
                    property int    _msecs:             0
                    property var    _savedTuningParamValues:    [ ]

                    // The following are set when getValues is called
                    property real   _value
                    property real   _valueSetpoint
                    property real   _valueRate
                    property real   _valueRateSetpoint

                    readonly property int _tickSeparation:      5
                    readonly property int _maxTickSections:     10
                    readonly property int _tuneListRollIndex:   0
                    readonly property int _tuneListPitchIndex:  1
                    readonly property int _tuneListYawIndex:    2
                    readonly property var _tuneList:            [ qsTr("Roll"), qsTr("Pitch"), qsTr("Yaw") ]
                    readonly property var _params:              [
                        [ controller.getParameterFact(-1, "MC_ROLL_P"),
                         controller.getParameterFact(-1, "MC_ROLLRATE_P"),
                         controller.getParameterFact(-1, "MC_ROLLRATE_I"),
                         controller.getParameterFact(-1, "MC_ROLLRATE_D"),
                         controller.getParameterFact(-1, "MC_ROLLRATE_FF") ],
                        [ controller.getParameterFact(-1, "MC_PITCH_P"),
                         controller.getParameterFact(-1, "MC_PITCHRATE_P"),
                         controller.getParameterFact(-1, "MC_PITCHRATE_I"),
                         controller.getParameterFact(-1, "MC_PITCHRATE_D"),
                         controller.getParameterFact(-1, "MC_PITCHRATE_FF") ],
                        [ controller.getParameterFact(-1, "MC_YAW_P"),
                         controller.getParameterFact(-1, "MC_YAWRATE_P"),
                         controller.getParameterFact(-1, "MC_YAWRATE_I"),
                         controller.getParameterFact(-1, "MC_YAWRATE_D"),
                         controller.getParameterFact(-1, "MC_YAW_FF"),
                         controller.getParameterFact(-1, "MC_YAWRATE_FF") ] ]

                    function adjustYAxisMin(yAxis, newValue) {
                        var newMin = Math.min(yAxis.min, newValue)
                        if (newMin % 5 != 0) {
                            newMin -= 5
                            newMin = Math.floor(newMin / _tickSeparation) * _tickSeparation
                        }
                        yAxis.min = newMin
                    }

                    function adjustYAxisMax(yAxis, newValue) {
                        var newMax = Math.max(yAxis.max, newValue)
                        if (newMax % 5 != 0) {
                            newMax += 5
                            newMax = Math.floor(newMax / _tickSeparation) * _tickSeparation
                        }
                        yAxis.max = newMax
                    }

                    function getValues() {
                        if (_currentTuneType === _tuneList[_tuneListRollIndex]) {
                            _value = _roll
                            _valueSetpoint = _rollSetpoint
                            _valueRate = _rollRate
                            _valueRateSetpoint = _rollRateSetpoint
                        } else if (_currentTuneType === _tuneList[_tuneListPitchIndex]) {
                            _value = _pitch
                            _valueSetpoint = _pitchSetpoint
                            _valueRate = _pitchRate
                            _valueRateSetpoint = _pitchRateSetpoint
                        } else if (_currentTuneType === _tuneList[_tuneListYawIndex]) {
                            _value = _yaw
                            _valueSetpoint = _yawSetpoint
                            _valueRate = _yawRate
                            _valueRateSetpoint = _yawRateSetpoint
                        }
                    }

                    function resetGraphs() {
                        valueSeries.removePoints(0, valueSeries.count)
                        valueSetpointSeries.removePoints(0, valueSetpointSeries.count)
                        valueRateSeries.removePoints(0, valueRateSeries.count)
                        valueRateSetpointSeries.removePoints(0, valueRateSetpointSeries.count)
                        _valueXAxis.min = 0
                        _valueXAxis.max = 0
                        _valueRateXAxis.min = 0
                        _valueRateXAxis.max = 0
                        _valueYAxis.min = 0
                        _valueYAxis.max = 10
                        _valueRateYAxis.min = 0
                        _valueRateYAxis.max = 10
                        _msecs = 0
                    }

                    function currentTuneTypeIndex() {
                        if (_currentTuneType === _tuneList[_tuneListRollIndex]) {
                            return _tuneListRollIndex
                        } else if (_currentTuneType === _tuneList[_tuneListPitchIndex]) {
                            return _tuneListPitchIndex
                        } else if (_currentTuneType === _tuneList[_tuneListYawIndex]) {
                            return _tuneListYawIndex
                        }
                    }

                    // Save the current set of tuning values so we can reset to them
                    function saveTuningParamValues() {
                        var tuneTypeIndex = currentTuneTypeIndex()

                        _savedTuningParamValues = [ ]
                        var currentTuneParams = _params[tuneTypeIndex]
                        for (var i=0; i<currentTuneParams.length; i++) {
                            _savedTuningParamValues.push(currentTuneParams[i].valueString)
                        }
                        savedRepeater.model = _savedTuningParamValues
                    }

                    function resetToSavedTuningParamValues() {
                        var tuneTypeIndex = currentTuneTypeIndex()

                        for (var i=0; i<_savedTuningParamValues.length; i++) {
                            _params[tuneTypeIndex][i].value = _savedTuningParamValues[i]
                        }
                    }

                    Component.onCompleted: {
                        saveTuningParamValues()
                    }

                    on_CurrentTuneTypeChanged: {
                        saveTuningParamValues()
                        resetGraphs()
                    }

                    ExclusiveGroup {
                        id: tuneTypeRadios
                    }

                    ValueAxis {
                        id:             valueXAxis
                        min:            0
                        max:            0
                        labelFormat:    "%d"
                        titleText:      "sec"
                    }

                    ValueAxis {
                        id:             valueRateXAxis
                        min:            0
                        max:            0
                        labelFormat:    "%d"
                        titleText:      "sec"
                    }

                    ValueAxis {
                        id:         valueYAxis
                        min:        0
                        max:        10
                        titleText:  "deg"
                        tickCount:  Math.min(((max - min) / _tickSeparation), _maxTickSections) + 1
                    }

                    ValueAxis {
                        id:         valueRateYAxis
                        min:        0
                        max:        10
                        titleText:  "deg/s"
                        tickCount:  Math.min(((max - min) / _tickSeparation), _maxTickSections) + 1
                    }

                    Timer {
                        id:         dataTimer
                        interval:   10
                        running:    false
                        repeat:     true

                        onTriggered: {
                            var seconds = _msecs / 1000
                            _valueXAxis.max = seconds
                            _valueRateXAxis.max = seconds

                            getValues()

                            valueSeries.append(seconds, _value)
                            adjustYAxisMin(_valueYAxis, _value)
                            adjustYAxisMax(_valueYAxis, _value)

                            valueSetpointSeries.append(seconds, _valueSetpoint)
                            adjustYAxisMin(_valueYAxis, _valueSetpoint)
                            adjustYAxisMax(_valueYAxis, _valueSetpoint)

                            valueRateSeries.append(seconds, _valueRate)
                            adjustYAxisMin(_valueRateYAxis, _valueRate)
                            adjustYAxisMax(_valueRateYAxis, _valueRate)

                            valueRateSetpointSeries.append(seconds, _valueRateSetpoint)
                            adjustYAxisMin(_valueRateYAxis, _valueRateSetpoint)
                            adjustYAxisMax(_valueRateYAxis, _valueRateSetpoint)

                            _msecs += interval
                            /*
                              Testing with just start/stop for now. No time limit.
                            if (valueSeries.count > _maxPointCount) {
                                valueSeries.remove(0)
                                valueSetpointSeries.remove(0)
                                valueRateSeries.remove(0)
                                valueRateSetpointSeries.remove(0)
                                valueXAxis.min = valueSeries.at(0).x
                                valueRateXAxis.min = valueSeries.at(0).x
                            }
                            */
                        }

                        property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
                        property int _maxPointCount:    10000 / interval
                    }

                    Column {
                        spacing:            _margins
                        Layout.alignment:   Qt.AlignTop

                        QGCLabel { text: qsTr("Tuning Axis:") }

                        RowLayout {
                            spacing: _margins

                            Repeater {
                                model: _tuneList
                                QGCRadioButton {
                                    text:           modelData
                                    checked:        _currentTuneType === modelData
                                    exclusiveGroup: tuneTypeRadios

                                    onClicked: _currentTuneType = modelData
                                }
                            }
                        }

                        Item { width: 1; height: 1 }

                        QGCLabel { text: qsTr("Tuning Values:") }

                        GridLayout {
                            rows:           factList.length
                            flow:           GridLayout.TopToBottom
                            rowSpacing:     _margins
                            columnSpacing:  _margins

                            property var factList: _params[_tuneList.indexOf(_currentTuneType)]

                            Repeater {
                                model: parent.factList

                                QGCLabel { text: modelData.name }
                            }

                            Repeater {
                                model: parent.factList

                                QGCButton {
                                    text: "-"
                                    onClicked: {
                                        var value = modelData.value
                                        modelData.value -= value * adjustPercentModel.get(adjustPercentCombo.currentIndex).value
                                    }
                                }
                            }

                            Repeater {
                                model: parent.factList

                                FactTextField {
                                    Layout.fillWidth:   true
                                    fact:               modelData
                                    showUnits:          false
                                }
                            }

                            Repeater {
                                model: parent.factList

                                QGCButton {
                                    text: "+"
                                    onClicked: {
                                        var value = modelData.value
                                        modelData.value += value * adjustPercentModel.get(adjustPercentCombo.currentIndex).value
                                    }
                                }
                            }
                        }

                        RowLayout {
                            QGCLabel { text: qsTr("Increment/Decrement %") }

                            QGCComboBox {
                                id:     adjustPercentCombo
                                model:  ListModel {
                                    id: adjustPercentModel
                                    ListElement { text: "5"; value: 0.05 }
                                    ListElement { text: "10"; value: 0.10 }
                                    ListElement { text: "15"; value: 0.15 }
                                    ListElement { text: "20"; value: 0.20 }
                                }
                            }
                        }
                        Item { width: 1; height: 1 }

                        QGCLabel { text: qsTr("Saved Tuning Values:") }

                        GridLayout {
                            rows:           savedRepeater.model.length
                            flow:           GridLayout.TopToBottom
                            rowSpacing:     _margins
                            columnSpacing:  _margins

                            Repeater {
                                model: _params[_tuneList.indexOf(_currentTuneType)]

                                QGCLabel { text: modelData.name }
                            }

                            Repeater {
                                id: savedRepeater

                                QGCLabel { text: modelData }
                            }
                        }

                        RowLayout {
                            spacing: _margins

                            QGCButton {
                                text:       qsTr("Save Values")
                                onClicked:  saveTuningParamValues()
                            }

                            QGCButton {
                                text:       qsTr("Reset To Saved Values")
                                onClicked:  resetToSavedTuningParamValues()
                            }
                        }

                        Item { width: 1; height: 1 }

                        QGCLabel { text: qsTr("Chart:") }

                        RowLayout {
                            spacing: _margins

                            QGCButton {
                                text:       qsTr("Clear")
                                onClicked:  resetGraphs()
                            }

                            QGCButton {
                                text:       dataTimer.running ? qsTr("Stop") : qsTr("Start")
                                onClicked:  dataTimer.running = !dataTimer.running
                            }
                        }
                    }

                    Column {
                        Layout.fillWidth: true

                        ChartView {
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            height:             availableHeight / 2
                            title:              _currentTuneType
                            antialiasing:       true
                            legend.alignment:   Qt.AlignRight

                            LineSeries {
                                id:         valueSeries
                                name:       "Response"
                                axisY:      valueYAxis
                                axisX:      valueXAxis
                            }

                            LineSeries {
                                id:         valueSetpointSeries
                                name:       "Command"
                                axisY:      valueYAxis
                                axisX:      valueXAxis
                            }
                        }

                        ChartView {
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            height:             availableHeight / 2
                            title:              _currentTuneType + qsTr(" Rate")
                            antialiasing:       true
                            legend.alignment:   Qt.AlignRight

                            LineSeries {
                                id:         valueRateSeries
                                name:       "Response"
                                axisY:      valueRateYAxis
                                axisX:      valueRateXAxis
                            }

                            LineSeries {
                                id:         valueRateSetpointSeries
                                name:       "Command"
                                axisY:      valueRateYAxis
                                axisX:      valueRateXAxis
                            }
                        }
                    }
                } // RowLayout - Advanced Page
            } // Component - Advanced Page
        } // Column
    } // Component - pageComponent
} // SetupPage
