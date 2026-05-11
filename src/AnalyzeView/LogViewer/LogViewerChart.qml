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
    required property var logParser
    required property var logViewerController

    spacing: ScreenTools.defaultFontPixelHeight * 0.5

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    property bool   _positionMarkerVisible: false
    property real   _markerPixelX: 0
    property real   _markerXValue: 0
    property var    _markerRows: []
    property var    _markerEventRows: []
    property string _markerModeName: ""

    property real   _fullMinX: 0
    property real   _fullMaxX: 1
    property real   _zoomMinX: 0
    property real   _zoomMaxX: 1

    property var    _seriesByField: ({})
    property var    _fieldYRange: ({})
    property var    _fieldFullRange: ({})   // full-dataset min/max, set when series is first created
    property var    _eventSeriesByType: ({})

    // Shared position / zoom signals
    signal cursorMoved(real timestampSeconds)
    signal zoomApplied(real minX, real maxX)


    readonly property real _legendRowHeight: ScreenTools.defaultFontPixelHeight * 1.1
    readonly property real _legendColorBlockSize: ScreenTools.defaultFontPixelHeight * 0.8
    readonly property real _legendItemSpacing: ScreenTools.defaultFontPixelWidth * 0.2

    QGCPalette { id: qgcPal }

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
        return logViewerController.modeColor(String(modeName))
    }

    function modeLegendEntries() {
        return logViewerController.modeLegendEntries(logParser.modeSegments)
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
    // Zoom
    // -------------------------------------------------------------------------
    function _applyZoomInternal(minX, maxX) {
        if (maxX <= minX) return
        _zoomMinX = minX
        _zoomMaxX = maxX
        _binXAxis.min = _zoomMinX
        _binXAxis.max = _zoomMaxX
        if (_positionMarkerVisible && (_markerXValue < _zoomMinX || _markerXValue > _zoomMaxX)) {
            _markerXValue = (_zoomMinX + _zoomMaxX) / 2
        }
        _refreshCursorPixelPos()
        Qt.callLater(_syncSeriesWithSelection)
    }

    // User-driven zoom — also emits zoomApplied so other charts can sync
    function applyZoomRange(minX, maxX) {
        _applyZoomInternal(minX, maxX)
        zoomApplied(minX, maxX)
    }

    // External zoom sync — apply without re-emitting
    function setSharedZoom(minX, maxX) {
        _applyZoomInternal(minX, maxX)
    }

    // External cursor sync — apply without re-emitting
    function setSharedCursor(t) {
        if (_binXAxis.max <= _binXAxis.min) return
        _markerXValue = t
        _markerPixelX = _axisXToPixel(t)
        _positionMarkerVisible = true
        _queryCursorValues()
    }

    function resetZoom() {
        applyZoomRange(_fullMinX, _fullMaxX)
    }

    // -------------------------------------------------------------------------
    // Coordinate conversions
    // -------------------------------------------------------------------------
    function _pixelToAxisX(pixelX) {
        const plotX = _binChart.plotArea.x
        const plotW = _binChart.plotArea.width
        if (plotW <= 0 || _binXAxis.max <= _binXAxis.min) return _binXAxis.min
        const clampedPixel = Math.max(plotX, Math.min(plotX + plotW, pixelX))
        const ratio = (clampedPixel - plotX) / plotW
        return _binXAxis.min + (ratio * (_binXAxis.max - _binXAxis.min))
    }

    function _axisXToPixel(axisX) {
        const plotX = _binChart.plotArea.x
        const plotW = _binChart.plotArea.width
        if (plotW <= 0 || _binXAxis.max <= _binXAxis.min) return plotX
        const ratio = (axisX - _binXAxis.min) / (_binXAxis.max - _binXAxis.min)
        return plotX + Math.max(0, Math.min(plotW, ratio * plotW))
    }

    // -------------------------------------------------------------------------
    // Marker / cursor
    // -------------------------------------------------------------------------
    function _queryCursorValues() {
        _markerModeName = logParser.modeAt(_markerXValue)

        const selectedFields = logViewerController.selectedFields
        const rows = []
        for (let i = 0; i < selectedFields.length; i++) {
            const field = selectedFields[i]
            const value = logParser.fieldValueAt(field, _markerXValue)
            if (isNaN(value)) continue
            const fr = _fieldFullRange[field]
            rows.push({
                name:  field,
                color: fieldColor(field),
                value: value,
                min:   fr ? fr.min : NaN,
                max:   fr ? fr.max : NaN
            })
        }
        _markerRows = rows

        const threshold = Math.max(0.05, (_binXAxis.max - _binXAxis.min) / 200.0)
        const nearbyEvents = logParser.eventsNear(_markerXValue, threshold)
        const events = []
        for (let i = 0; i < nearbyEvents.length; i++) {
            events.push({ color: eventColor(nearbyEvents[i].type), text: nearbyEvents[i].description })
        }
        _markerEventRows = events
    }

    function _updateCursorInfo(pixelX, pixelY, w, h) {
        if (w <= 0 || h <= 0 || _binXAxis.max <= _binXAxis.min) return
        _positionMarkerVisible = true
        _markerPixelX = Math.max(_binChart.plotArea.x, Math.min(_binChart.plotArea.x + _binChart.plotArea.width, pixelX))
        _markerXValue = _pixelToAxisX(_markerPixelX)
        _queryCursorValues()
        cursorMoved(_markerXValue)
    }

    function _refreshCursorPixelPos() {
        if (_positionMarkerVisible) {
            _markerPixelX = _axisXToPixel(_markerXValue)
        }
    }

    // Public: called by parent after log load
    function centerCursor() {
        if (_binXAxis.max <= _binXAxis.min) return
        _markerXValue = (_binXAxis.min + _binXAxis.max) / 2
        _markerPixelX = _axisXToPixel(_markerXValue)
        _positionMarkerVisible = true
        _queryCursorValues()
    }

    function _refreshCursorAtCurrentPosition() {
        if (_binXAxis.max <= _binXAxis.min) return
        _positionMarkerVisible = true
        _markerPixelX = _axisXToPixel(_markerXValue)
        _queryCursorValues()
    }

    // Public: called by parent on log clear
    function clearMarker() {
        _positionMarkerVisible = false
        _markerRows = []
        _markerEventRows = []
    }

    // -------------------------------------------------------------------------
    // Chart / series management
    // -------------------------------------------------------------------------

    // Public: full reset — called by parent on log load or clear
    function refreshBinChart() {
        while (_binChart.seriesList.length > 0) {
            _binChart.removeSeries(_binChart.seriesList[0])
        }
        _seriesByField = {}
        _fieldYRange = {}
        _fieldFullRange = {}
        _eventSeriesByType = {}

        if (logParser.minTimestamp >= 0.0 && logParser.maxTimestamp > logParser.minTimestamp) {
            _fullMinX = logParser.minTimestamp
            _fullMaxX = logParser.maxTimestamp
        } else {
            _fullMinX = 0
            _fullMaxX = 1
        }
        _zoomMinX = _fullMinX
        _zoomMaxX = _fullMaxX
        _binXAxis.min = _zoomMinX
        _binXAxis.max = _zoomMaxX
        _binYAxis.min = 0
        _binYAxis.max = 1
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
                _binChart.removeSeries(_seriesByField[tracked[i]])
                delete _seriesByField[tracked[i]]
                delete _fieldYRange[tracked[i]]
            }
        }

        // Rebuild series data for the current zoom window.
        // Reuse existing series objects (clear + repopulate) to avoid Qt Graphs
        // lifecycle issues that occur when a series is removed and immediately recreated.
        for (let i = 0; i < newSelection.length; i++) {
            const fieldName = String(newSelection[i])

            const pixelWidth = Math.max(1, Math.floor(_binChart.plotArea.width))
            const points = logParser.fieldSamplesFiltered(fieldName, _zoomMinX, _zoomMaxX, pixelWidth)

            let series
            if (_seriesByField[fieldName]) {
                series = _seriesByField[fieldName]
                series.clear()
            } else {
                series = _lineSeriesComponent.createObject(_binChart, {
                    color: fieldColor(fieldName),
                    width: 2,
                    axisX: _binXAxis,
                    axisY: _binYAxis
                })
                _binChart.addSeries(series)
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
        _binYAxis.min = globalMinY
        _binYAxis.max = globalMaxY

        const existingTypes = Object.keys(_eventSeriesByType)
        for (let i = 0; i < existingTypes.length; i++) {
            _binChart.removeSeries(_eventSeriesByType[existingTypes[i]])
        }
        _eventSeriesByType = {}
        const eventList = logParser.events
        for (let e = 0; e < eventList.length; e++) {
            const ev = eventList[e]
            if (ev.time < _binXAxis.min || ev.time > _binXAxis.max) continue
            if (!_eventSeriesByType[ev.type]) {
                const eventSeries = _scatterSeriesComponent.createObject(_binChart, {
                    color: eventColor(ev.type),
                    axisX: _binXAxis,
                    axisY: _binYAxis
                })
                _binChart.addSeries(eventSeries)
                _eventSeriesByType[ev.type] = eventSeries
            }
            _eventSeriesByType[ev.type].append(ev.time, _binYAxis.max)
        }

        if (_positionMarkerVisible) {
            Qt.callLater(_refreshCursorAtCurrentPosition)
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

    // -------------------------------------------------------------------------
    // Timeline bars (offset to align with chart plot area)
    // -------------------------------------------------------------------------
    ColumnLayout {
        id: _timelineContainer
        Layout.preferredWidth: _binChart.plotArea.x + _binChart.plotArea.width
        spacing: ScreenTools.defaultFontPixelHeight * 0.1

        property real _barHeight: ScreenTools.defaultFontPixelHeight * 0.6

        function _segmentX(start) {
            const w = _binChart.plotArea.width
            if (_binXAxis.max <= _binXAxis.min) return 0
            return Math.max(0, Math.min(w, ((Math.max(start, _binXAxis.min) - _binXAxis.min) / (_binXAxis.max - _binXAxis.min)) * w))
        }

        function _segmentWidth(start, end) {
            const w = _binChart.plotArea.width
            if (_binXAxis.max <= _binXAxis.min) return 0
            return ((Math.min(end, _binXAxis.max) - Math.max(start, _binXAxis.min)) / (_binXAxis.max - _binXAxis.min)) * w
        }

        function _eventX(time, itemWidth) {
            const w = _binChart.plotArea.width
            if (_binXAxis.max <= _binXAxis.min) return 0
            return Math.max(0, Math.min(w - itemWidth, ((time - _binXAxis.min) / (_binXAxis.max - _binXAxis.min)) * w))
        }

        // Bar 1: Flight modes
        RowLayout {
            Layout.fillWidth: true
            visible: logParser.modeSegments.length > 0

            QGCLabel {
                text: qsTr("Modes")
                font.bold: true
                Layout.preferredWidth: _binChart.plotArea.x  // Align with chart plot area
                Layout.maximumWidth: Layout.preferredWidth
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: _timelineContainer._barHeight

                Repeater {
                    model: logParser.modeSegments

                    Rectangle {
                        visible: _binXAxis.max > _binXAxis.min && modelData.end >= _binXAxis.min && modelData.start <= _binXAxis.max
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
                Layout.preferredWidth: _binChart.plotArea.x  // Align with chart plot area
                Layout.maximumWidth: Layout.preferredWidth
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: _timelineContainer._barHeight

                Repeater {
                    model: logParser.dropouts

                    Rectangle {
                        visible: _binXAxis.max > _binXAxis.min && modelData.end >= _binXAxis.min && modelData.start <= _binXAxis.max
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
                Layout.preferredWidth: _binChart.plotArea.x  // Align with chart plot area
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
                        visible: _binXAxis.max > _binXAxis.min
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
    Item {
        id: _chartContainer
        Layout.fillWidth: true
        Layout.fillHeight: true

        GraphsView {
            id: _binChart
            anchors.fill: parent
            marginTop: 0
            marginRight: 0
            marginBottom: 0
            marginLeft: 0

            theme: GraphsTheme {
                colorScheme: qgcPal.globalTheme === QGCPalette.Light ? GraphsTheme.ColorScheme.Light : GraphsTheme.ColorScheme.Dark
                backgroundColor: qgcPal.windowShadeDark
                backgroundVisible: true
                plotAreaBackgroundColor: qgcPal.windowShadeDark
                grid.mainColor: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.25)
                grid.subColor: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)
                grid.mainWidth: 1
                labelBackgroundVisible: false
                labelTextColor: qgcPal.text
                axisXLabelFont.family: ScreenTools.fixedFontFamily
                axisXLabelFont.pointSize: ScreenTools.smallFontPointSize
                axisYLabelFont.family: ScreenTools.fixedFontFamily
                axisYLabelFont.pointSize: ScreenTools.smallFontPointSize
            }

            axisX: ValueAxis {
                id: _binXAxis
                titleText: qsTr("Time (s)")
                min: 0
                max: 1
            }

            axisY: ValueAxis {
                id: _binYAxis
                titleText: qsTr("Value")
                min: 0
                max: 1
            }
        }

        Rectangle {
            id: _zoomSelectionRect
            visible: false
            color: Qt.rgba(1, 1, 1, 0.2)
            border.color: qgcPal.buttonHighlight
            border.width: 1
            z: 1000
        }

        MouseArea {
            id: _chartZoomArea
            anchors.fill: parent
            enabled: _binXAxis.max > _binXAxis.min
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            z: 1001

            property real _dragStartX: 0

            onPressed: (mouse) => {
                if (mouse.button === Qt.RightButton) {
                    resetZoom()
                    return
                }
                _dragStartX = mouse.x
                _zoomSelectionRect.x = mouse.x
                _zoomSelectionRect.y = _binChart.plotArea.y
                _zoomSelectionRect.width = 0
                _zoomSelectionRect.height = _binChart.plotArea.height
                _zoomSelectionRect.visible = true
            }

            onPositionChanged: (mouse) => {
                if (pressed && _zoomSelectionRect.visible) {
                    const left  = Math.min(_dragStartX, mouse.x)
                    const right = Math.max(_dragStartX, mouse.x)
                    _zoomSelectionRect.x = left
                    _zoomSelectionRect.width = Math.max(0, right - left)
                }
            }

            onReleased: (mouse) => {
                if (!_zoomSelectionRect.visible) return
                const dragWidth = _zoomSelectionRect.width
                _zoomSelectionRect.visible = false
                if (dragWidth < ScreenTools.defaultFontPixelWidth * 0.5) {
                    _updateCursorInfo(mouse.x, mouse.y, width, height)
                    return
                }
                const leftX  = _pixelToAxisX(_zoomSelectionRect.x)
                const rightX = _pixelToAxisX(_zoomSelectionRect.x + _zoomSelectionRect.width)
                applyZoomRange(Math.min(leftX, rightX), Math.max(leftX, rightX))
            }
        }

        // Position marker line
        Rectangle {
            visible: _positionMarkerVisible
            x: _markerPixelX
            y: _binChart.plotArea.y
            width: 1
            height: _binChart.plotArea.height
            color: qgcPal.text
            z: 1002
        }

        // Value popup
        Rectangle {
            id: _valuePopup
            x: _popupX()
            y: _binChart.plotArea.y
            z: 1003
            implicitWidth: _valueColumnLayout.implicitWidth + (margin * 2)
            implicitHeight: _valueColumnLayout.implicitHeight + (margin * 2)
            color: qgcPal.windowShade
            border.color: qgcPal.windowShadeDark
            radius: ScreenTools.defaultFontPixelWidth * 0.3
            visible: _positionMarkerVisible

            property real margin: ScreenTools.defaultFontPixelWidth / 2
            property real colorBlockWidth: ScreenTools.defaultFontPixelHeight * 0.8

            function _popupX() {
                const plotMidX = _binChart.plotArea.x + _binChart.plotArea.width / 2
                if (_markerPixelX < plotMidX) {
                    // Cursor in left half — place popup on the right
                    const rightX = _binChart.plotArea.x + _binChart.plotArea.width - width
                    return Math.max(0, Math.min(rightX, _chartContainer.width - width))
                } else {
                    // Cursor in right half — place popup on the left
                    return Math.max(0, _binChart.plotArea.x)
                }
            }

            ColumnLayout {
                id: _valueColumnLayout
                anchors.fill: parent
                anchors.margins: _valuePopup.margin
                spacing: ScreenTools.defaultFontPixelHeight * 0.2

                QGCLabel {
                    text: qsTr("t=%1 s").arg(_markerXValue.toFixed(3))
                    font.bold: true
                }

                RowLayout {
                    visible: _markerModeName.length > 0
                    spacing: ScreenTools.defaultFontPixelWidth * 0.2

                    Rectangle {
                        Layout.preferredWidth: _valuePopup.colorBlockWidth
                        Layout.preferredHeight: _valuePopup.colorBlockWidth
                        color: modeColor(_markerModeName)
                    }

                    QGCLabel { text: qsTr("Mode:") }
                    QGCLabel { text: _markerModeName; font.bold: true }
                }

                Repeater {
                    model: _markerRows

                    ColumnLayout {
                        spacing: ScreenTools.defaultFontPixelHeight * 0.15

                        // Line 1: color block + field name
                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth * 0.4

                            Rectangle {
                                Layout.preferredWidth:  _valuePopup.colorBlockWidth
                                Layout.preferredHeight: _valuePopup.colorBlockWidth
                                color: modelData.color
                            }

                            QGCLabel {
                                width: _valuePopup.width - (ScreenTools.defaultFontPixelWidth * 4)
                                elide: Text.ElideMiddle
                                text:  modelData.name
                                font.bold: true
                            }
                        }

                        // Line 2: Current / Min / Max
                        RowLayout {
                            Layout.leftMargin: _valuePopup.colorBlockWidth + ScreenTools.defaultFontPixelWidth * 0.4
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
                            Layout.preferredWidth: _valuePopup.colorBlockWidth
                            Layout.preferredHeight: _valuePopup.colorBlockWidth
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
        visible: logViewerController.selectedFields.length > 0
        spacing: ScreenTools.defaultFontPixelWidth

        QGCLabel { text: qsTr("Fields:"); font.bold: true }

        Repeater {
            model: logViewerController.selectedFields

            RowLayout {
                spacing: _legendItemSpacing

                Rectangle {
                    Layout.preferredWidth: _legendColorBlockSize
                    Layout.preferredHeight: _legendColorBlockSize
                    color: fieldColor(modelData)
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
            text: qsTr("Drag on chart to zoom X-axis. Right click chart to reset zoom.")
        }

        QGCButton {
            text: qsTr("Reset Zoom")
            enabled: _zoomMinX !== _fullMinX || _zoomMaxX !== _fullMaxX
            onClicked: resetZoom()
        }
    }
}
