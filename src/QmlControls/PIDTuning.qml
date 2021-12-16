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
import QGroundControl.Vehicle       1.0

RowLayout {
    layoutDirection: Qt.RightToLeft

    property var    axis
    property string unit
    property string title
    property var    tuningMode
    property double chartDisplaySec:     8 // number of seconds to display
    property bool   showAutoModeChange:  false
    property bool   showAutoTuning:      false
    property alias  autotuningEnabled:   autotuningEnabled.checked

    property real   _margins:           ScreenTools.defaultFontPixelHeight / 2
    property int    _currentAxis:       0
    property var    _xAxis:             xAxis
    property var    _yAxis:             yAxis
    property int    _msecs:             0
    property double _last_t:            0
    property var    _savedTuningParamValues:    [ ]
    property bool   _showCharts: !ScreenTools.isMobile // TODO: test and enable on mobile

    readonly property int _tickSeparation:      5
    readonly property int _maxTickSections:     10

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

    function resetGraphs() {
        for (var i = 0; i < chart.count; ++i) {
            chart.series(i).removePoints(0, chart.series(i).count)
        }
        _xAxis.min = 0
        _xAxis.max = 0
        _yAxis.min = 0
        _yAxis.max = 0
        _msecs = 0
        _last_t = 0
    }

    // Save the current set of tuning values so we can reset to them
    function saveTuningParamValues() {
        _savedTuningParamValues = [ ]
        for (var i=0; i<axis[_currentAxis].params.count; i++) {
            var currentTuneParam = controller.getParameterFact(-1,
                axis[_currentAxis].params.get(i).param)
            _savedTuningParamValues.push(currentTuneParam.valueString)
        }
        savedRepeater.model = _savedTuningParamValues
    }

    function resetToSavedTuningParamValues() {
        for (var i=0; i<axis[_currentAxis].params.count; i++) {
            var currentTuneParam = controller.getParameterFact(-1,
                axis[_currentAxis].params.get(i).param)
            currentTuneParam.value = _savedTuningParamValues[i]
        }
    }

    function axisIndexChanged() {
        chart.removeAllSeries()
        axis[_currentAxis].plot.forEach(function(e) {
            chart.createSeries(ChartView.SeriesTypeLine, e.name, xAxis, yAxis);
        })
        var chartTitle = axis[_currentAxis].plotTitle
        if (chartTitle == null)
            chartTitle = axis[_currentAxis].name
        chart.title = chartTitle + " " + title
        saveTuningParamValues()
        resetGraphs()
    }

    Component.onCompleted: {
        axisIndexChanged()
        globals.activeVehicle.setPIDTuningTelemetryMode(tuningMode)
        saveTuningParamValues()
    }

    Component.onDestruction: globals.activeVehicle.setPIDTuningTelemetryMode(Vehicle.ModeDisabled)
    on_CurrentAxisChanged: axisIndexChanged()

    ValueAxis {
        id:             xAxis
        min:            0
        max:            0
        labelFormat:    "%.2f"
        titleText:      qsTr("sec")
        tickCount:      11
    }

    ValueAxis {
        id:         yAxis
        min:        0
        max:        10
        titleText:  unit
        tickCount:  Math.min(((max - min) / _tickSeparation), _maxTickSections) + 1
    }

    Timer {
        id:         dataTimer
        interval:   10
        running:    true
        repeat:     true

        onTriggered: {
            _xAxis.max = _msecs / 1000
            _xAxis.min = _msecs / 1000 - chartDisplaySec

            var firstPoint = _msecs == 0

            var len = axis[_currentAxis].plot.length
            for (var i = 0; i < len; ++i) {
                var value = axis[_currentAxis].plot[i].value
                if (!isNaN(value)) {
                    chart.series(i).append(_msecs/1000, value)
                    if (firstPoint) {
                        _yAxis.min = value
                        _yAxis.max = value
                    } else {
                        adjustYAxisMin(_yAxis, value)
                        adjustYAxisMax(_yAxis, value)
                    }
                    // limit history
                    var minSec = _msecs/1000 - 3*60
                    while (chart.series(i).count > 0 && chart.series(i).at(0).x < minSec) {
                        chart.series(i).remove(0)
                    }
                }
            }

            var t = new Date().getTime() // in ms
            if (_last_t > 0)
                _msecs += t-_last_t
            _last_t = t
        }

        property int _maxPointCount:    10000 / interval
    }

    QGCFlickable {
        contentWidth:           parent.width * (_showCharts ? 0.4 : 1)
        contentHeight:          rightColumn.height
        Layout.fillHeight:      true
        Layout.minimumWidth:    contentWidth
        Layout.maximumWidth:    contentWidth
        Layout.alignment:       Qt.AlignTop

        Column {
            spacing:            _margins
            Layout.alignment:   Qt.AlignTop

            width:          parent.width
            id:             rightColumn

            Row {
                id:        _autotuneSelectRow
                spacing:   _margins
                visible:   showAutoTuning

                Switch {
                    id:        autotuningEnabled
                    checked:   true
                }

                QGCLabel {
                    color:   qgcPal.text
                    text:    autotuningEnabled.checked ? qsTr("Autotune enabled") : qsTr("Autotune disabled")
                }
            }

            Column {
                width:     parent.width
                visible:   _autotuneSelectRow.visible && autotuningEnabled.checked

                AutotuneUI {
                    anchors {
                        top:         parent.top
                        topMargin:   _margins * 2
                    }

                    width:     parent.width
                }
            }

            Column {
                width:     parent.width
                visible:   !_autotuneSelectRow.visible || !autotuningEnabled.checked

                Column {
                    RowLayout {
                        spacing: _margins
                        visible: axis.length > 1

                        QGCLabel { text: qsTr("Select Tuning:") }

                        Repeater {
                            model: axis
                            QGCRadioButton {
                                text:           modelData.name
                                checked:        index == _currentAxis
                                onClicked: _currentAxis = index
                            }
                        }
                    }
                }

                // Instantiate all sliders (instead of switching the model), so that
                // values are not changed unexpectedly if they do not match with a tick
                // value
                Repeater {
                    model: axis
                    FactSliderPanel {
                        width:       parent.width
                        visible:     _currentAxis === index
                        sliderModel: axis[index].params
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
                            model: axis[_currentAxis].params

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
        }
    }

    ColumnLayout {
        visible:            _showCharts

        ChartView {
            id:                 chart
            antialiasing:       true
            legend.alignment:   Qt.AlignBottom
            Layout.fillHeight:  true
            Layout.fillWidth:   true

            // enable mouse dragging
            MouseArea {
                property var _startPoint: undefined
                property double _scaling: 0
                anchors.fill: parent
                onPressed: {
                    _startPoint = Qt.point(mouse.x, mouse.y)
                    var start = chart.mapToValue(_startPoint)
                    var next = chart.mapToValue(Qt.point(mouse.x+1, mouse.y+1))
                    _scaling = next.x - start.x
                }
                onWheel: {
                    if (wheel.angleDelta.y > 0)
                        chartDisplaySec /= 1.2
                    else
                        chartDisplaySec *= 1.2
                    _xAxis.min = _xAxis.max - chartDisplaySec
                }
                onPositionChanged: {
                    if(_startPoint != undefined) {
                        dataTimer.running = false
                        var cp = Qt.point(mouse.x, mouse.y)
                        var dx = (cp.x - _startPoint.x) * _scaling
                        _startPoint = cp
                        _xAxis.max -= dx
                        _xAxis.min -= dx
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
                    if (showAutoModeChange && autoModeChange.checked) {
                        globals.activeVehicle.flightMode = dataTimer.running ? "Stabilized" : globals.activeVehicle.pauseFlightMode
                    }
                }
            }
            Connections {
                target: globals.activeVehicle
                onArmedChanged: {
                    if (armed && !dataTimer.running) { // start plotting on arming if not already running
                        dataTimer.running = true
                        _last_t = 0
                    }
                }
            }
        }

        QGCCheckBox {
            visible: showAutoModeChange
            id:     autoModeChange
            text:   qsTr("Automatic Flight Mode Switching")
            onClicked: {
                if (checked)
                    dataTimer.running = false
            }
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
