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
import QtCharts         2.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

RowLayout {
    layoutDirection: Qt.RightToLeft

    property var tuneList
    property var params

    property real   _chartHeight:       ScreenTools.defaultFontPixelHeight * 20
    property real   _margins:           ScreenTools.defaultFontPixelHeight / 2
    property string _currentTuneType:   tuneList[0]
    property real   _roll:              activeVehicle.roll.value
    property real   _rollSetpoint:      activeVehicle.setpoint.roll.value
    property real   _rollRate:          activeVehicle.rollRate.value
    property real   _rollRateSetpoint:  activeVehicle.setpoint.rollRate.value
    property real   _pitch:             activeVehicle.pitch.value
    property real   _pitchSetpoint:     activeVehicle.setpoint.pitch.value
    property real   _pitchRate:         activeVehicle.pitchRate.value
    property real   _pitchRateSetpoint: activeVehicle.setpoint.pitchRate.value
    property real   _yaw:               activeVehicle.heading.value
    property real   _yawSetpoint:       activeVehicle.setpoint.yaw.value
    property real   _yawRate:           activeVehicle.yawRate.value
    property real   _yawRateSetpoint:   activeVehicle.setpoint.yawRate.value
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
        if (_currentTuneType === tuneList[_tuneListRollIndex]) {
            _value = _roll
            _valueSetpoint = _rollSetpoint
            _valueRate = _rollRate
            _valueRateSetpoint = _rollRateSetpoint
        } else if (_currentTuneType === tuneList[_tuneListPitchIndex]) {
            _value = _pitch
            _valueSetpoint = _pitchSetpoint
            _valueRate = _pitchRate
            _valueRateSetpoint = _pitchRateSetpoint
        } else if (_currentTuneType === tuneList[_tuneListYawIndex]) {
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
        if (_currentTuneType === tuneList[_tuneListRollIndex]) {
            return _tuneListRollIndex
        } else if (_currentTuneType === tuneList[_tuneListPitchIndex]) {
            return _tuneListPitchIndex
        } else if (_currentTuneType === tuneList[_tuneListYawIndex]) {
            return _tuneListYawIndex
        }
    }

    // Save the current set of tuning values so we can reset to them
    function saveTuningParamValues() {
        var tuneTypeIndex = currentTuneTypeIndex()

        _savedTuningParamValues = [ ]
        var currentTuneParams = params[tuneTypeIndex]
        for (var i=0; i<currentTuneParams.length; i++) {
            _savedTuningParamValues.push(currentTuneParams[i].valueString)
        }
        savedRepeater.model = _savedTuningParamValues
    }

    function resetToSavedTuningParamValues() {
        var tuneTypeIndex = currentTuneTypeIndex()

        for (var i=0; i<_savedTuningParamValues.length; i++) {
            params[tuneTypeIndex][i].value = _savedTuningParamValues[i]
        }
    }

    Component.onCompleted: {
        activeVehicle.setPIDTuningTelemetryMode(true)
        saveTuningParamValues()
    }

    Component.onDestruction: activeVehicle.setPIDTuningTelemetryMode(false)

    on_CurrentTuneTypeChanged: {
        saveTuningParamValues()
        resetGraphs()
    }

    ValueAxis {
        id:             valueXAxis
        min:            0
        max:            0
        labelFormat:    "%d"
        titleText:      "msec"
        tickCount:      11
    }

    ValueAxis {
        id:             valueRateXAxis
        min:            0
        max:            0
        labelFormat:    "%d"
        titleText:      "msec"
        tickCount:      11
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
            _valueXAxis.max = _msecs
            _valueRateXAxis.max = _msecs

            getValues()

            valueSeries.append(_msecs, _value)
            adjustYAxisMin(_valueYAxis, _value)
            adjustYAxisMax(_valueYAxis, _value)

            valueSetpointSeries.append(_msecs, _valueSetpoint)
            adjustYAxisMin(_valueYAxis, _valueSetpoint)
            adjustYAxisMax(_valueYAxis, _valueSetpoint)

            valueRateSeries.append(_msecs, _valueRate)
            adjustYAxisMin(_valueRateYAxis, _valueRate)
            adjustYAxisMax(_valueRateYAxis, _valueRate)

            valueRateSetpointSeries.append(_msecs, _valueRateSetpoint)
            adjustYAxisMin(_valueRateYAxis, _valueRateSetpoint)
            adjustYAxisMax(_valueRateYAxis, _valueRateSetpoint)

            _msecs += interval
        }

        property int _maxPointCount:    10000 / interval
    }

    Column {
        spacing:            _margins
        Layout.alignment:   Qt.AlignTop

        Column {
            QGCLabel { text: qsTr("Tuning Axis:") }

            RowLayout {
                spacing: _margins

                Repeater {
                    model: tuneList
                    QGCRadioButton {
                        text:           modelData
                        checked:        _currentTuneType === modelData
                        onClicked: _currentTuneType = modelData
                    }
                }
            }
        }

        QGCLabel { text: qsTr("Tuning Values:") }

        GridLayout {
            rows:           factList.length
            flow:           GridLayout.TopToBottom
            rowSpacing:     _margins
            columnSpacing:  _margins

            property var factList: params[tuneList.indexOf(_currentTuneType)]

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
                        var newValue = value - (value * adjustPercentModel.get(adjustPercentCombo.currentIndex).value)
                        if (newValue >= modelData.min) {
                            modelData.value = newValue
                        }
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
                        var newValue = value + (value * adjustPercentModel.get(adjustPercentCombo.currentIndex).value)
                        if (newValue <= modelData.max) {
                            modelData.value = newValue
                        }
                    }
                }
            }
        }

        RowLayout {
            QGCLabel { text: qsTr("Increment/Decrement %") }

            QGCComboBox {
                id:         adjustPercentCombo
                textRole:   "text"
                model:  ListModel {
                    id: adjustPercentModel
                    ListElement { text: "5"; value: 0.05 }
                    ListElement { text: "10"; value: 0.10 }
                    ListElement { text: "15"; value: 0.15 }
                    ListElement { text: "20"; value: 0.20 }
                }
            }
        }

        Column {
            QGCLabel { text: qsTr("Clipboard Values:") }

            GridLayout {
                rows:           savedRepeater.model.length
                flow:           GridLayout.TopToBottom
                rowSpacing:     0
                columnSpacing:  _margins

                Repeater {
                    model: params[tuneList.indexOf(_currentTuneType)]

                    QGCLabel { text: modelData.name }
                }

                Repeater {
                    id: savedRepeater

                    QGCLabel { text: modelData }
                }
            }
        }

        RowLayout {
            spacing: _margins

            QGCButton {
                text:       qsTr("Save To Clipboard")
                onClicked:  saveTuningParamValues()
            }

            QGCButton {
                text:       qsTr("Restore From Clipboard")
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
                onClicked: {
                    dataTimer.running = !dataTimer.running
                    if (autoModeChange.checked) {
                        activeVehicle.flightMode = dataTimer.running ? "Stabilized" : activeVehicle.pauseFlightMode
                    }
                }
            }
        }

        QGCCheckBox {
            id:     autoModeChange
            text:   qsTr("Automatic Flight Mode Switching")
        }

        Column {
            visible: autoModeChange.checked
            QGCLabel {
                text:            qsTr("Switches to 'Stabilized' when you click Start.")
                font.pointSize:     ScreenTools.smallFontPointSize
            }

            QGCLabel {
                text:            qsTr("Switches to '%1' when you click Stop.").arg(activeVehicle.pauseFlightMode)
                font.pointSize:     ScreenTools.smallFontPointSize
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
} // RowLayout
