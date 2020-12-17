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
    property real   _rollRate:          globals.activeVehicle.rollRate.value
    property real   _rollRateSetpoint:  globals.activeVehicle.setpoint.rollRate.value
    property real   _pitchRate:         globals.activeVehicle.pitchRate.value
    property real   _pitchRateSetpoint: globals.activeVehicle.setpoint.pitchRate.value
    property real   _yawRate:           globals.activeVehicle.yawRate.value
    property real   _yawRateSetpoint:   globals.activeVehicle.setpoint.yawRate.value
    property var    _valueRateXAxis:    valueRateXAxis
    property var    _valueRateYAxis:    valueRateYAxis
    property int    _msecs:             0
    property double _last_t:            0
    property var    _savedTuningParamValues:    [ ]
    property bool   _showCharts: !ScreenTools.isMobile // TODO: test and enable on mobile

    // The following are set when getValues is called
    property real   _valueRate
    property real   _valueRateSetpoint

    readonly property int _tickSeparation:      5
    readonly property int _maxTickSections:     10
    readonly property int _tuneListRollIndex:   0
    readonly property int _tuneListPitchIndex:  1
    readonly property int _tuneListYawIndex:    2
    readonly property int _chartDisplaySec:     3 // number of seconds to display

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
            _valueRate = _rollRate
            _valueRateSetpoint = _rollRateSetpoint
        } else if (_currentTuneType === tuneList[_tuneListPitchIndex]) {
            _valueRate = _pitchRate
            _valueRateSetpoint = _pitchRateSetpoint
        } else if (_currentTuneType === tuneList[_tuneListYawIndex]) {
            _valueRate = _yawRate
            _valueRateSetpoint = _yawRateSetpoint
        }
    }

    function resetGraphs() {
        valueRateSeries.removePoints(0, valueRateSeries.count)
        valueRateSetpointSeries.removePoints(0, valueRateSetpointSeries.count)
        _valueRateXAxis.min = 0
        _valueRateXAxis.max = 0
        _valueRateYAxis.min = 0
        _valueRateYAxis.max = 10
        _msecs = 0
        _last_t = 0
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
        for (var i=0; i<params[tuneTypeIndex].count; i++) {
            var currentTuneParam = controller.getParameterFact(-1,
                params[tuneTypeIndex].get(i).param)
            _savedTuningParamValues.push(currentTuneParam.valueString)
        }
        savedRepeater.model = _savedTuningParamValues
    }

    function resetToSavedTuningParamValues() {
        var tuneTypeIndex = currentTuneTypeIndex()

        for (var i=0; i<params[tuneTypeIndex].count; i++) {
            var currentTuneParam = controller.getParameterFact(-1,
                params[tuneTypeIndex].get(i).param)
            currentTuneParam.value = _savedTuningParamValues[i]
        }
    }

    Component.onCompleted: {
        globals.activeVehicle.setPIDTuningTelemetryMode(true)
        saveTuningParamValues()
    }

    Component.onDestruction: globals.activeVehicle.setPIDTuningTelemetryMode(false)

    on_CurrentTuneTypeChanged: {
        saveTuningParamValues()
        resetGraphs()
    }

    ValueAxis {
        id:             valueRateXAxis
        min:            0
        max:            0
        labelFormat:    "%.2f"
        titleText:      "sec"
        tickCount:      11
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
        running:    true
        repeat:     true

        onTriggered: {
            _valueRateXAxis.max = _msecs / 1000
            _valueRateXAxis.min = _msecs / 1000 - _chartDisplaySec

            getValues()

            if (!isNaN(_valueRate)) {
                valueRateSeries.append(_msecs/1000, _valueRate)
                adjustYAxisMin(_valueRateYAxis, _valueRate)
                adjustYAxisMax(_valueRateYAxis, _valueRate)
            }

            if (!isNaN(_valueRateSetpoint)) {
                valueRateSetpointSeries.append(_msecs/1000, _valueRateSetpoint)
                adjustYAxisMin(_valueRateYAxis, _valueRateSetpoint)
                adjustYAxisMax(_valueRateYAxis, _valueRateSetpoint)
            }

            var t = new Date().getTime() // in ms
            if (_last_t > 0)
                _msecs += t-_last_t
            _last_t = t
        }

        property int _maxPointCount:    10000 / interval
    }

    Column {
        spacing:            _margins
        Layout.alignment:   Qt.AlignTop
        width:          parent.width * (_showCharts ? 0.4 : 1)

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


        // Instantiate all sliders (instead of switching the model), so that
        // values are not changed unexpectedly if they do not match with a tick
        // value
        FactSliderPanel {
            width:       parent.width
            visible:     _currentTuneType === tuneList[_tuneListRollIndex]
            sliderModel: params[_tuneListRollIndex]
        }
        FactSliderPanel {
            width:       parent.width
            visible:     _currentTuneType === tuneList[_tuneListPitchIndex]
            sliderModel: params[_tuneListPitchIndex]
        }
        FactSliderPanel {
            width:       parent.width
            visible:     _currentTuneType === tuneList[_tuneListYawIndex]
            sliderModel: params[_tuneListYawIndex]
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

                    QGCLabel { text: param }
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
    }

    Column {
        Layout.fillWidth: true
        Layout.alignment:   Qt.AlignTop
        visible:            _showCharts

        ChartView {
            id:                 ratesChart
            anchors.left:       parent.left
            anchors.right:      parent.right
            height:             availableHeight * 0.75
            title:              _currentTuneType + qsTr(" Rate")
            antialiasing:       true
            legend.alignment:   Qt.AlignBottom

            LineSeries {
                id:         valueRateSeries
                name:       "Response"
                axisY:      valueRateYAxis
                axisX:      valueRateXAxis
            }

            LineSeries {
                id:         valueRateSetpointSeries
                name:       "Setpoint"
                axisY:      valueRateYAxis
                axisX:      valueRateXAxis
            }

            // enable mouse dragging
            MouseArea {
                property var _startPoint: undefined
                property double _scaling: 0
                anchors.fill: parent
                onPressed: {
                    _startPoint = Qt.point(mouse.x, mouse.y)
                    var start = ratesChart.mapToValue(_startPoint)
                    var next = ratesChart.mapToValue(Qt.point(mouse.x+1, mouse.y+1))
                    _scaling = next.x - start.x
                }
                onPositionChanged: {
                    if(_startPoint != undefined) {
                        dataTimer.running = false
                        var cp = Qt.point(mouse.x, mouse.y)
                        var dx = (cp.x - _startPoint.x) * _scaling
                        _startPoint = cp
                        _valueRateXAxis.max -= dx
                        _valueRateXAxis.min -= dx
                    }
                }

                onReleased: {
                    _startPoint = undefined
                }
            }
        }

        Item { width: 1; height: 1 }

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
                    _last_t = 0
                    if (autoModeChange.checked) {
                        globals.activeVehicle.flightMode = dataTimer.running ? "Stabilized" : globals.activeVehicle.pauseFlightMode
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
                text:            qsTr("Switches to '%1' when you click Stop.").arg(globals.activeVehicle.pauseFlightMode)
                font.pointSize:     ScreenTools.smallFontPointSize
            }
        }
    }
} // RowLayout
