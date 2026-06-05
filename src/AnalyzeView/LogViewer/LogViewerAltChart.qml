import QtQuick
import QtQuick.Layouts
import QtGraphs

import QGroundControl
import QGroundControl.Controls

/// Altitude mini-chart for the Log Viewer Map tab.
/// Displays vehicle_global_position.alt (or equivalent) as a line chart
/// with zoom, position marker, and value popup. The marker timestamp is
/// exposed via the markerChanged / markerCleared signals so the parent
/// can place a dot on the map.
Item {
    id: control

    required property var logParser
    required property string altFieldName   ///< e.g. "vehicle_global_position.alt"

    property bool xAxisShowLocalTime: false

    signal markerChanged(double timestampSeconds)
    signal markerCleared()
    signal zoomApplied(real minX, real maxX)

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    property real _markerAltValue: 0
    property real _altMin: 0
    property real _altMax: 0
    property bool _hasAltRange: false
    property var _altSeries: null

    // -------------------------------------------------------------------------
    // Public API (delegated to _base)
    // -------------------------------------------------------------------------
    function setSharedZoom(minX, maxX) { _base.setSharedZoom(minX, maxX) }
    function setSharedCursor(t)        { _base.setSharedCursor(t) }

    // -------------------------------------------------------------------------
    // Series management
    // -------------------------------------------------------------------------
    Component {
        id: _altSeriesComp
        LineSeries {
            color: QGroundControl.globalPalette.colorGreen
            width: 2
        }
    }

    Component.onCompleted: {
        _altSeries = _altSeriesComp.createObject(_base.graphsView, {
            axisX: _base.xAxis,
            axisY: _base.yAxis
        })
        _base.graphsView.addSeries(_altSeries)
    }

    function _refreshSeries() {
        const fieldName = control.altFieldName
        if (!logParser.parseComplete || fieldName.length === 0 || !_altSeries) {
            if (_altSeries) _altSeries.clear()
            return
        }

        const pixelWidth = Math.max(1, Math.floor(_base.graphsView.plotArea.width))
        const points = logParser.fieldSamplesFiltered(fieldName, _base.zoomMinX, _base.zoomMaxX, pixelWidth)

        _altSeries.clear()
        if (!points || points.length === 0) return

        let minY = Number.MAX_VALUE
        let maxY = -Number.MAX_VALUE
        for (let i = 0; i < points.length; i++) {
            _altSeries.append(points[i].x, points[i].y)
            if (points[i].y < minY) minY = points[i].y
            if (points[i].y > maxY) maxY = points[i].y
        }

        // Track full-dataset min/max (not just visible window)
        const fr = logParser.fieldMinMax(fieldName)
        if (fr && fr.min !== undefined && fr.min <= fr.max) {
            _altMin = fr.min
            _altMax = fr.max
            _hasAltRange = true
        } else {
            _hasAltRange = false
        }

        const pad = Math.max(1, (maxY - minY) * 0.05)
        _base.yAxis.min = minY - pad
        _base.yAxis.max = maxY + pad

        if (_base.markerVisible) {
            _markerAltValue = logParser.fieldValueAt(control.altFieldName, _base.markerXValue)
        }
    }

    // -------------------------------------------------------------------------
    // Cursor helpers
    // -------------------------------------------------------------------------
    function _initCursor() {
        if (!logParser.parseComplete || _base.xAxis.max <= _base.xAxis.min) return
        const mid = (_base.xAxis.min + _base.xAxis.max) / 2
        _base.setCursor(mid)
        control.markerChanged(mid)
    }

    // -------------------------------------------------------------------------
    // Connections
    // -------------------------------------------------------------------------
    Connections {
        target: logParser

        function onParseCompleteChanged() {
            if (!logParser.parseComplete) {
                if (_altSeries) _altSeries.clear()
                _hasAltRange = false
                control.markerCleared()
                return
            }
            const hasRange = logParser.minTimestamp >= 0 && logParser.maxTimestamp > logParser.minTimestamp
            _base.initRange(hasRange ? logParser.minTimestamp : 0,
                            hasRange ? logParser.maxTimestamp : 1)
            Qt.callLater(control._initCursor)
        }
    }

    Connections {
        target: _base
        function onZoomRangeSet(minX, maxX) { Qt.callLater(_refreshSeries) }
        function onCursorPositionSet(t)     { _markerAltValue = logParser.fieldValueAt(altFieldName, t) }
        function onCursorMoved(t)           { control.markerChanged(t) }
        function onZoomApplied(minX, maxX)  { control.zoomApplied(minX, maxX) }
    }

    onVisibleChanged: {
        if (visible && logParser.parseComplete && !_base.markerVisible) {
            Qt.callLater(_initCursor)
        }
    }

    // -------------------------------------------------------------------------
    // Base chart
    // -------------------------------------------------------------------------
    LogViewerBaseChart {
        id: _base
        anchors.fill: parent
        logParser: control.logParser
        xAxisShowLocalTime: control.xAxisShowLocalTime
        yAxisTitle: qsTr("Alt (m)")
        popupYOffset: ScreenTools.defaultFontPixelHeight * 0.4

        // ---- Chart-specific popup rows ----
        QGCLabel {
            text: altFieldName
            font.bold: true
            elide: Text.ElideMiddle
            width: _base.popupWidth - (ScreenTools.defaultFontPixelWidth * 2)
        }

        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth * 0.3

            QGCLabel { text: qsTr("Current") }
            QGCLabel { text: isNaN(_markerAltValue) ? "—" : _markerAltValue.toFixed(1) + " m"; font.bold: true }

            Item { visible: _hasAltRange; width: ScreenTools.defaultFontPixelWidth * 0.5 }

            QGCLabel { visible: _hasAltRange; text: qsTr("Min") }
            QGCLabel { visible: _hasAltRange; text: _altMin.toFixed(1) + " m"; font.bold: true }

            Item { visible: _hasAltRange; width: ScreenTools.defaultFontPixelWidth * 0.5 }

            QGCLabel { visible: _hasAltRange; text: qsTr("Max") }
            QGCLabel { visible: _hasAltRange; text: _altMax.toFixed(1) + " m"; font.bold: true }
        }
    }
}

