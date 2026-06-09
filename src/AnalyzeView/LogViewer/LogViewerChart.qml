import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs

import QGroundControl
import QGroundControl.Controls

/// Self-contained chart view for the Log Viewer.
/// Sizing and visibility are controlled entirely by the parent.
/// The parent calls refreshBinChart(), centerCursor(), and clearMarker() at
/// appropriate lifecycle points (log load, log clear, field-clear).
ColumnLayout {
    id: control

    required property var logParser
    required property var logViewerController

    property bool xAxisShowLocalTime: false

    spacing: ScreenTools.defaultFontPixelHeight * 0.5

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    property var _markerRows: []
    property var _markerEventRows: []
    property string _markerModeName: ""

    property var _seriesByField: ({})
    property var _fieldYRange: ({})
    property var _fieldFullRange: ({})   // full-dataset min/max, set when series is first created
    property var _eventSeriesByType: ({})

    // Shared position / zoom signals
    signal cursorMoved(real timestampSeconds)
    signal zoomApplied(real minX, real maxX)


    readonly property real _legendRowHeight: ScreenTools.defaultFontPixelHeight * 1.1
    readonly property real _legendColorBlockSize: ScreenTools.defaultFontPixelHeight * 0.8
    readonly property real _legendItemSpacing: ScreenTools.defaultFontPixelWidth * 0.2

    readonly property var _fieldChartColors: [
        "#1E88E5", // blue
        "#E53935", // red
        "#43A047", // green
        "#FB8C00", // orange
        "#8E24AA", // purple
        "#00ACC1", // cyan
        "#FDD835", // yellow
        "#D81B60", // pink
        "#6D4C41", // brown
        "#546E7A", // blue grey
        "#3949AB", // indigo
        "#00897B", // teal
    ]

    // -------------------------------------------------------------------------
    // Helper components
    // -------------------------------------------------------------------------
    Component {
        id: _lineSeriesComponent
        LineSeries { }
    }

    Component {
        id: _scatterSeriesComponent
        ScatterSeries { }
    }

    // -------------------------------------------------------------------------
    // Color helpers (public — parent may call fieldColor() for the field list)
    // -------------------------------------------------------------------------
    function eventColor(eventType) {
        return logViewerController.eventColor(eventType)
    }

    function modeColor(modeName) {
        return logParser.modeColor(String(modeName))
    }

    function modeLegendEntries() {
        return logParser.modeNames
    }

    function fieldColor(fieldName) {
        const idx = logViewerController.selectedFields.indexOf(fieldName)
        if (idx >= 0) {
            return _fieldChartColors[idx % _fieldChartColors.length]
        }
        return logViewerController.fieldColor(fieldName)
    }

    function eventTypeLabel(eventType) {
        if (eventType === "mode")    return qsTr("Mode")
        if (eventType === "event")   return qsTr("Event")
        if (eventType === "error")   return qsTr("Error")
        if (eventType === "warning") return qsTr("Warning")
        return eventType
    }

    // -------------------------------------------------------------------------
    // Zoom / cursor (delegated to _base)
    // -------------------------------------------------------------------------
    function applyZoomRange(minX, maxX) { _base.applyZoomRange(minX, maxX) }
    function setSharedZoom(minX, maxX)  { _base.setSharedZoom(minX, maxX) }
    function setSharedCursor(t)         { _base.setSharedCursor(t) }
    function resetZoom()                { _base.resetZoom() }

    // -------------------------------------------------------------------------
    // Marker / cursor
    // -------------------------------------------------------------------------
    function _queryValuesAtTime(xValue) {
        const modeName = logParser.modeAt(xValue)
        const selectedFields = logViewerController.selectedFields
        const rows = []
        for (let i = 0; i < selectedFields.length; i++) {
            const field = selectedFields[i]
            const value = logParser.fieldValueAt(field, xValue)
            if (isNaN(value)) continue
            const fr = _fieldFullRange[field]
            rows.push({
                name: field,
                color: fieldColor(field),
                value: value,
                min: fr ? fr.min : NaN,
                max: fr ? fr.max : NaN
            })
        }
        const threshold = Math.max(0.05, (_base.xAxis.max - _base.xAxis.min) / 200.0)
        const nearbyEvents = logParser.eventsNear(xValue, threshold)
        const events = []
        for (let i = 0; i < nearbyEvents.length; i++) {
            events.push({ color: eventColor(nearbyEvents[i].type), text: nearbyEvents[i].description })
        }
        return { modeName: modeName, rows: rows, events: events }
    }

    function _queryCursorValues() {
        const result = _queryValuesAtTime(_base.markerXValue)
        _markerModeName = result.modeName
        _markerRows = result.rows
        _markerEventRows = result.events
    }

    // Public: called by parent after log load
    function centerCursor() {
        if (_base.xAxis.max <= _base.xAxis.min) return
        _base.setCursor((_base.xAxis.min + _base.xAxis.max) / 2)
    }

    // Public: called by parent on log clear
    function clearMarker() {
        _base.clearCursor()
        _markerModeName = ""
        _markerRows = []
        _markerEventRows = []
    }

    // -------------------------------------------------------------------------
    // Chart / series management
    // -------------------------------------------------------------------------

    // Public: full reset — called by parent on log load or clear
    function refreshBinChart() {
        while (_base.graphsView.seriesList.length > 0) {
            _base.graphsView.removeSeries(_base.graphsView.seriesList[0])
        }
        _seriesByField = {}
        _fieldYRange = {}
        _fieldFullRange = {}
        _eventSeriesByType = {}

        const hasRange = logParser.minTimestamp >= 0.0 && logParser.maxTimestamp > logParser.minTimestamp
        _base.initRange(hasRange ? logParser.minTimestamp : 0,
                        hasRange ? logParser.maxTimestamp : 1)
        _base.yAxis.min = 0
        _base.yAxis.max = 1
    }

    function _syncSeriesWithSelection() {
        const newSelection = logViewerController.selectedFields

        // Remove series no longer in selection
        const desired = {}
        for (let i = 0; i < newSelection.length; i++) {
            desired[String(newSelection[i])] = true
        }
        const tracked = Object.keys(_seriesByField)
        for (let i = 0; i < tracked.length; i++) {
            if (!desired[tracked[i]]) {
                _base.graphsView.removeSeries(_seriesByField[tracked[i]])
                delete _seriesByField[tracked[i]]
                delete _fieldYRange[tracked[i]]
            }
        }

        // Rebuild series data for the current zoom window.
        // Reuse existing series objects (clear + repopulate) to avoid Qt Graphs
        // lifecycle issues that occur when a series is removed and immediately recreated.
        for (let i = 0; i < newSelection.length; i++) {
            const fieldName = String(newSelection[i])

            const pixelWidth = Math.max(1, Math.floor(_base.graphsView.plotArea.width))
            const points = logParser.fieldSamplesFiltered(fieldName, _base.zoomMinX, _base.zoomMaxX, pixelWidth)

            let series
            if (_seriesByField[fieldName]) {
                series = _seriesByField[fieldName]
                series.clear()
            } else {
                series = _lineSeriesComponent.createObject(_base.graphsView, {
                    color: fieldColor(fieldName),
                    width: 2,
                    axisX: _base.xAxis,
                    axisY: _base.yAxis
                })
                _base.graphsView.addSeries(series)
                _seriesByField[fieldName] = series

                // Compute full-dataset min/max once when the series is first created
                const fr = logParser.fieldMinMax(fieldName)
                _fieldFullRange[fieldName] = (fr && fr.min !== undefined && fr.min <= fr.max) ? { min: fr.min, max: fr.max } : null
            }

            if (!points || points.length === 0) {
                _fieldYRange[fieldName] = { min: 0, max: 1 }
                continue
            }

            let minY = Number.MAX_VALUE
            let maxY = -Number.MAX_VALUE
            for (let j = 0; j < points.length; j++) {
                series.append(points[j].x, points[j].y)
                if (points[j].y < minY) minY = points[j].y
                if (points[j].y > maxY) maxY = points[j].y
            }
            _fieldYRange[fieldName] = { min: minY, max: maxY }
        }

        for (let i = 0; i < newSelection.length; i++) {
            const fn = String(newSelection[i])
            const s = _seriesByField[fn]
            if (s) s.color = fieldColor(fn)
        }

        const allTracked = Object.keys(_seriesByField)
        let globalMinY = 0
        let globalMaxY = 1
        if (allTracked.length > 0) {
            globalMinY = Number.MAX_VALUE
            globalMaxY = -Number.MAX_VALUE
            for (let i = 0; i < allTracked.length; i++) {
                const r = _fieldYRange[allTracked[i]]
                if (r) {
                    if (r.min < globalMinY) globalMinY = r.min
                    if (r.max > globalMaxY) globalMaxY = r.max
                }
            }
            if (globalMinY === globalMaxY) globalMaxY = globalMinY + 1
        }
        _base.yAxis.min = globalMinY
        _base.yAxis.max = globalMaxY

        const existingTypes = Object.keys(_eventSeriesByType)
        for (let i = 0; i < existingTypes.length; i++) {
            _base.graphsView.removeSeries(_eventSeriesByType[existingTypes[i]])
        }
        _eventSeriesByType = {}
        const eventList = logParser.events
        for (let e = 0; e < eventList.length; e++) {
            const ev = eventList[e]
            if (ev.time < _base.xAxis.min || ev.time > _base.xAxis.max) continue
            if (!_eventSeriesByType[ev.type]) {
                const eventSeries = _scatterSeriesComponent.createObject(_base.graphsView, {
                    color: eventColor(ev.type),
                    axisX: _base.xAxis,
                    axisY: _base.yAxis
                })
                _base.graphsView.addSeries(eventSeries)
                _eventSeriesByType[ev.type] = eventSeries
            }
            _eventSeriesByType[ev.type].append(ev.time, _base.yAxis.max)
        }

        if (_base.markerVisible) {
            Qt.callLater(_queryCursorValues)
        }
    }

    // -------------------------------------------------------------------------
    // Internal connections
    // -------------------------------------------------------------------------
    Connections {
        target: logViewerController
        function onSelectedFieldsChanged() {
            Qt.callLater(_syncSeriesWithSelection)
        }
    }

    Connections {
        target: _base
        function onZoomRangeSet(minX, maxX) { Qt.callLater(_syncSeriesWithSelection) }
        function onCursorPositionSet(t)     { _queryCursorValues() }
        function onCursorMoved(t)           { cursorMoved(t) }
        function onZoomApplied(minX, maxX)  { zoomApplied(minX, maxX) }
    }

    // -------------------------------------------------------------------------
    // Timeline bars (offset to align with chart plot area)
    // -------------------------------------------------------------------------
    ColumnLayout {
        id: _timelineContainer
        Layout.preferredWidth: _base.graphsView.plotArea.x + _base.graphsView.plotArea.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.1

        property real _barHeight: ScreenTools.defaultFontPixelHeight * 0.6

        function _segmentX(start) {
            const w = _base.graphsView.plotArea.width
            if (_base.xAxis.max <= _base.xAxis.min) return 0
            return Math.max(0, Math.min(w, ((Math.max(start, _base.xAxis.min) - _base.xAxis.min) / (_base.xAxis.max - _base.xAxis.min)) * w))
        }

        function _segmentWidth(start, end) {
            const w = _base.graphsView.plotArea.width
            if (_base.xAxis.max <= _base.xAxis.min) return 0
            return ((Math.min(end, _base.xAxis.max) - Math.max(start, _base.xAxis.min)) / (_base.xAxis.max - _base.xAxis.min)) * w
        }

        function _eventX(time, itemWidth) {
            const w = _base.graphsView.plotArea.width
            if (_base.xAxis.max <= _base.xAxis.min) return 0
            return Math.max(0, Math.min(w - itemWidth, ((time - _base.xAxis.min) / (_base.xAxis.max - _base.xAxis.min)) * w))
        }

        // Bar 1: Flight modes
        RowLayout {
            Layout.fillWidth: true
            visible: logParser.modeSegments.length > 0

            QGCLabel {
                text: qsTr("Modes")
                font.bold: true
                Layout.preferredWidth: _base.graphsView.plotArea.x  // Align with chart plot area
                Layout.maximumWidth: Layout.preferredWidth
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: _timelineContainer._barHeight

                Repeater {
                    model: logParser.modeSegments

                    Rectangle {
                        visible: _base.xAxis.max > _base.xAxis.min && modelData.end >= _base.xAxis.min && modelData.start <= _base.xAxis.max
                        height: parent.height
                        color: modeColor(modelData.mode)
                        x: _timelineContainer._segmentX(modelData.start)
                        width: Math.max(1, _timelineContainer._segmentWidth(modelData.start, modelData.end))
                    }
                }
            }
        }

        // Bar 2: Dropouts
        RowLayout {
            Layout.fillWidth: true
            visible: logParser.dropouts.length > 0

            QGCLabel {
                text: qsTr("Dropouts")
                font.bold: true
                Layout.preferredWidth: _base.graphsView.plotArea.x  // Align with chart plot area
                Layout.maximumWidth: Layout.preferredWidth
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: _timelineContainer._barHeight

                Repeater {
                    model: logParser.dropouts

                    Rectangle {
                        visible: _base.xAxis.max > _base.xAxis.min && modelData.end >= _base.xAxis.min && modelData.start <= _base.xAxis.max
                        height: parent.height
                        color: Qt.alpha(eventColor("error"), 0.533)
                        x: _timelineContainer._segmentX(modelData.start)
                        width: Math.max(2, _timelineContainer._segmentWidth(modelData.start, modelData.end))
                    }
                }
            }
        }

        // Bar 3: Events
        RowLayout {
            Layout.fillWidth: true
            visible: logParser.events.length > 0

            QGCLabel {
                text: qsTr("Events")
                font.bold: true
                Layout.preferredWidth: _base.graphsView.plotArea.x  // Align with chart plot area
                Layout.maximumWidth: Layout.preferredWidth
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: _timelineContainer._barHeight

                Repeater {
                    model: logParser.events

                    Rectangle {
                        width: 2
                        height: parent.height
                        visible: _base.xAxis.max > _base.xAxis.min
                        color: eventColor(modelData.type)
                        x: _timelineContainer._eventX(modelData.time, width)
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Chart area
    // -------------------------------------------------------------------------
    LogViewerBaseChart {
        id: _base
        Layout.fillWidth: true
        Layout.fillHeight: true
        logParser: control.logParser
        xAxisShowLocalTime: control.xAxisShowLocalTime
        yAxisTitle: qsTr("Value")

        // ---- Chart-specific popup rows ----
        RowLayout {
            visible: _markerModeName.length > 0
            spacing: ScreenTools.defaultFontPixelWidth * 0.2

            Rectangle {
                Layout.preferredWidth: _base.colorBlockWidth
                Layout.preferredHeight: _base.colorBlockWidth
                color: modeColor(_markerModeName)
            }

            QGCLabel { text: qsTr("Mode:") }
            QGCLabel { text: _markerModeName; font.bold: true }
        }

        Repeater {
            model: _markerRows

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight * 0.15

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth * 0.4

                    Rectangle {
                        Layout.preferredWidth: _base.colorBlockWidth
                        Layout.preferredHeight: _base.colorBlockWidth
                        color: modelData.color
                    }

                    QGCLabel {
                        width: _base.popupWidth - (ScreenTools.defaultFontPixelWidth * 4)
                        elide: Text.ElideMiddle
                        text: modelData.name
                        font.bold: true
                    }
                }

                RowLayout {
                    Layout.leftMargin: _base.colorBlockWidth + ScreenTools.defaultFontPixelWidth * 0.4
                    spacing: ScreenTools.defaultFontPixelWidth * 0.3

                    QGCLabel { text: qsTr("Current") }
                    QGCLabel { text: Number(modelData.value).toFixed(3); font.bold: true }

                    Item { width: ScreenTools.defaultFontPixelWidth * 0.5 }

                    QGCLabel { text: qsTr("Min") }
                    QGCLabel { text: isNaN(modelData.min) ? "—" : Number(modelData.min).toFixed(3); font.bold: true }

                    Item { width: ScreenTools.defaultFontPixelWidth * 0.5 }

                    QGCLabel { text: qsTr("Max") }
                    QGCLabel { text: isNaN(modelData.max) ? "—" : Number(modelData.max).toFixed(3); font.bold: true }
                }
            }
        }

        Repeater {
            model: _markerEventRows

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth * 0.2

                Rectangle {
                    Layout.preferredWidth: _base.colorBlockWidth
                    Layout.preferredHeight: _base.colorBlockWidth
                    color: modelData.color
                }

                QGCLabel {
                    Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 20
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    text: modelData.text
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Legends
    // -------------------------------------------------------------------------
    Row {
        Layout.fillWidth: true
        Layout.preferredHeight: _legendRowHeight
        visible: modeLegendEntries().length > 0
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel { text: qsTr("Modes:"); font.bold: true }

        Repeater {
            model: modeLegendEntries()

            RowLayout {
                spacing: _legendItemSpacing

                Rectangle {
                    Layout.preferredWidth: _legendColorBlockSize
                    Layout.preferredHeight: _legendColorBlockSize
                    color: modeColor(modelData)
                }

                QGCLabel { text: modelData }
            }
        }
    }

    Row {
        Layout.fillWidth: true
        Layout.preferredHeight: _legendRowHeight
        visible: logParser.events.length > 0
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel { text: qsTr("Events:"); font.bold: true }

        Repeater {
            model: ["mode", "event", "error", "warning"]

            RowLayout {
                spacing: _legendItemSpacing
                visible: {
                    for (let i = 0; i < logParser.events.length; i++) {
                        if (logParser.events[i].type === modelData) return true
                    }
                    return false
                }

                Rectangle {
                    Layout.preferredWidth: _legendColorBlockSize
                    Layout.preferredHeight: _legendColorBlockSize
                    color: eventColor(modelData)
                }

                QGCLabel { text: eventTypeLabel(modelData) }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Zoom controls
    // -------------------------------------------------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel {
            Layout.fillWidth: true
            text: qsTr("Click to place cursor. Shift+drag to move cursor. Drag to zoom X-axis. Double-click to reset zoom.")
        }

        QGCButton {
            text: qsTr("Reset Zoom")
            enabled: _base.zoomMinX !== _base.fullMinX || _base.zoomMaxX !== _base.fullMaxX
            onClicked: _base.resetZoom()
        }
    }
}
