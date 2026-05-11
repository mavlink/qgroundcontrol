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
    id: root

    required property var    logParser
    required property string altFieldName   ///< e.g. "vehicle_global_position.alt"

    signal markerChanged(double timestampSeconds)
    signal markerCleared()
    signal zoomApplied(real minX, real maxX)

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    property real _fullMinX: 0
    property real _fullMaxX: 1
    property real _zoomMinX: 0
    property real _zoomMaxX: 1

    property bool _markerVisible: false
    property real _markerPixelX: 0
    property real _markerXValue: 0
    property real _markerAltValue: 0
    property real _altMin: 0
    property real _altMax: 0
    property bool _hasAltRange: false

    QGCPalette { id: qgcPal }

    // -------------------------------------------------------------------------
    // Coordinate conversions
    // -------------------------------------------------------------------------
    function _pixelToAxisX(pixelX) {
        const plotX = _altChart.plotArea.x
        const plotW = _altChart.plotArea.width
        if (plotW <= 0 || _xAxis.max <= _xAxis.min) return _xAxis.min
        const ratio = (Math.max(plotX, Math.min(plotX + plotW, pixelX)) - plotX) / plotW
        return _xAxis.min + ratio * (_xAxis.max - _xAxis.min)
    }

    function _axisXToPixel(axisX) {
        const plotX = _altChart.plotArea.x
        const plotW = _altChart.plotArea.width
        if (plotW <= 0 || _xAxis.max <= _xAxis.min) return plotX
        const ratio = (axisX - _xAxis.min) / (_xAxis.max - _xAxis.min)
        return plotX + Math.max(0, Math.min(plotW, ratio * plotW))
    }

    // -------------------------------------------------------------------------
    // Zoom
    // -------------------------------------------------------------------------
    function _applyZoomInternal(minX, maxX) {
        if (maxX <= minX) return
        _zoomMinX = minX
        _zoomMaxX = maxX
        _xAxis.min = minX
        _xAxis.max = maxX
        if (_markerVisible && (_markerXValue < minX || _markerXValue > maxX)) {
            _markerXValue = (minX + maxX) / 2
        }
        _markerPixelX = _axisXToPixel(_markerXValue)
        Qt.callLater(_refreshSeries)
    }

    // User-driven zoom — emits zoomApplied for cross-chart sync
    function _applyZoom(minX, maxX) {
        _applyZoomInternal(minX, maxX)
        zoomApplied(minX, maxX)
    }

    // External zoom sync — no re-emit
    function setSharedZoom(minX, maxX) {
        _applyZoomInternal(minX, maxX)
    }

    // External cursor sync — no re-emit
    function setSharedCursor(t) {
        _markerVisible = true
        _markerXValue  = t
        _markerPixelX  = _axisXToPixel(t)
        _markerAltValue = logParser.fieldValueAt(root.altFieldName, t)
    }

    function _resetZoom() {
        _applyZoom(_fullMinX, _fullMaxX)
    }

    // -------------------------------------------------------------------------
    // Series data
    // -------------------------------------------------------------------------
    function _refreshSeries() {
        const fieldName = root.altFieldName
        if (!logParser.parsed || fieldName.length === 0) {
            _altSeries.clear()
            return
        }

        const pixelWidth = Math.max(1, Math.floor(_altChart.plotArea.width))
        const points = logParser.fieldSamplesFiltered(fieldName, _zoomMinX, _zoomMaxX, pixelWidth)

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
        _yAxis.min = minY - pad
        _yAxis.max = maxY + pad

        if (_markerVisible) {
            _markerAltValue = logParser.fieldValueAt(root.altFieldName, _markerXValue)
        }
    }

    // -------------------------------------------------------------------------
    // Marker / cursor
    // -------------------------------------------------------------------------
    function _updateCursor(pixelX) {
        if (_xAxis.max <= _xAxis.min) return
        _markerVisible = true
        _markerPixelX = Math.max(_altChart.plotArea.x,
                                 Math.min(_altChart.plotArea.x + _altChart.plotArea.width, pixelX))
        _markerXValue = _pixelToAxisX(_markerPixelX)
        _markerAltValue = logParser.fieldValueAt(root.altFieldName, _markerXValue)
        root.markerChanged(_markerXValue)
    }

    // -------------------------------------------------------------------------
    // Connections
    // -------------------------------------------------------------------------
    Connections {
        target: logParser

        function onParsedChanged() {
            if (!logParser.parsed) {
                _altSeries.clear()
                _markerVisible  = false
                _hasAltRange    = false
                root.markerCleared()
                return
            }
            if (logParser.minTimestamp >= 0 && logParser.maxTimestamp > logParser.minTimestamp) {
                root._fullMinX = logParser.minTimestamp
                root._fullMaxX = logParser.maxTimestamp
            } else {
                root._fullMinX = 0
                root._fullMaxX = 1
            }
            root._applyZoom(root._fullMinX, root._fullMaxX)
            Qt.callLater(root._initCursor)
        }
    }

    // Called on first parse and when the component becomes visible after parse
    function _initCursor() {
        if (!logParser.parsed || _xAxis.max <= _xAxis.min) return
        _markerXValue   = (_xAxis.min + _xAxis.max) / 2
        _markerPixelX   = _axisXToPixel(_markerXValue)
        _markerAltValue = logParser.fieldValueAt(root.altFieldName, _markerXValue)
        _markerVisible  = true
        root.markerChanged(_markerXValue)
    }

    onVisibleChanged: {
        if (visible && logParser.parsed && !_markerVisible) {
            Qt.callLater(_initCursor)
        }
    }

    // -------------------------------------------------------------------------
    // Chart
    // -------------------------------------------------------------------------
    Item {
        id: _chartContainer
        anchors.fill: parent

        GraphsView {
            id: _altChart
            anchors.fill: parent
            marginTop: 0
            marginRight: 0
            marginBottom: 0
            marginLeft: 0

            theme: GraphsTheme {
                colorScheme:            qgcPal.globalTheme === QGCPalette.Light ? GraphsTheme.ColorScheme.Light : GraphsTheme.ColorScheme.Dark
                backgroundColor:        qgcPal.windowShadeDark
                backgroundVisible:      true
                plotAreaBackgroundColor: qgcPal.windowShadeDark
                grid.mainColor:         Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.25)
                grid.subColor:          Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)
                grid.mainWidth:         1
                labelBackgroundVisible: false
                labelTextColor:         qgcPal.text
                axisXLabelFont.family:       ScreenTools.fixedFontFamily
                axisXLabelFont.pointSize:    ScreenTools.smallFontPointSize
                axisYLabelFont.family:       ScreenTools.fixedFontFamily
                axisYLabelFont.pointSize:    ScreenTools.smallFontPointSize
            }

            axisX: ValueAxis {
                id: _xAxis
                titleText: qsTr("Time (s)")
                min: 0
                max: 1
            }

            axisY: ValueAxis {
                id: _yAxis
                titleText: qsTr("Alt (m)")
                min: 0
                max: 1
            }

            LineSeries {
                id: _altSeries
                color: QGroundControl.globalPalette.colorGreen
                width: 2
            }
        }

        // Zoom selection rect
        Rectangle {
            id: _zoomRect
            visible: false
            color: Qt.rgba(1, 1, 1, 0.2)
            border.color: qgcPal.buttonHighlight
            border.width: 1
            z: 10
        }

        // Mouse area — drag to zoom, right-click to reset, click/move to set cursor
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            z: 11

            property real _dragStartX: 0

            onPressed: (mouse) => {
                if (mouse.button === Qt.RightButton) {
                    root._resetZoom()
                    return
                }
                _dragStartX = mouse.x
                _zoomRect.x = mouse.x
                _zoomRect.y = _altChart.plotArea.y
                _zoomRect.width = 0
                _zoomRect.height = _altChart.plotArea.height
                _zoomRect.visible = true
            }

            onPositionChanged: (mouse) => {
                if (pressed && _zoomRect.visible) {
                    const left  = Math.min(_dragStartX, mouse.x)
                    const right = Math.max(_dragStartX, mouse.x)
                    _zoomRect.x = left
                    _zoomRect.width = Math.max(0, right - left)
                }
            }

            onReleased: (mouse) => {
                if (!_zoomRect.visible) return
                const dragW = _zoomRect.width
                _zoomRect.visible = false
                if (dragW < ScreenTools.defaultFontPixelWidth * 0.5) {
                    root._updateCursor(mouse.x)
                    return
                }
                const leftX  = root._pixelToAxisX(_zoomRect.x)
                const rightX = root._pixelToAxisX(_zoomRect.x + _zoomRect.width)
                root._applyZoom(Math.min(leftX, rightX), Math.max(leftX, rightX))
            }
        }

        // Position marker line
        Rectangle {
            visible: _markerVisible
            x:       _markerPixelX
            y:       _altChart.plotArea.y
            width:   1
            height:  _altChart.plotArea.height
            color:   qgcPal.text
            z:       12

            // Recalculate pixel position when the plot area is laid out or resized
            Connections {
                target: _altChart
                function onPlotAreaChanged() {
                    if (root._markerVisible) {
                        root._markerPixelX = root._axisXToPixel(root._markerXValue)
                    }
                }
            }
        }

        // Value popup
        Rectangle {
            id: _popup
            visible:  _markerVisible
            x:        _popupX()
            y:        _altChart.plotArea.y + ScreenTools.defaultFontPixelHeight * 0.4
            implicitWidth:  _popupCol.implicitWidth + (margin * 2)
            implicitHeight: _popupCol.implicitHeight + (margin * 2)
            color:    qgcPal.windowShade
            border.color: qgcPal.windowShadeDark
            radius:   ScreenTools.defaultFontPixelWidth * 0.3
            z:        13

            property real margin: ScreenTools.defaultFontPixelWidth / 2

            function _popupX() {
                const mid = _altChart.plotArea.x + _altChart.plotArea.width / 2
                if (_markerPixelX < mid) {
                    return Math.max(0, _altChart.plotArea.x + _altChart.plotArea.width - width)
                } else {
                    return Math.max(0, _altChart.plotArea.x)
                }
            }

            ColumnLayout {
                id: _popupCol
                anchors.fill: parent
                anchors.margins: _popup.margin
                spacing: ScreenTools.defaultFontPixelHeight * 0.2

                // Line 1: field name (bold)
                QGCLabel {
                    text: altFieldName
                    font.bold: true
                    elide: Text.ElideMiddle
                    width: parent.width
                }

                // Line 2: time
                QGCLabel {
                    text: qsTr("t = %1 s").arg(_markerXValue.toFixed(3))
                }

                // Line 3: Current / Min / Max
                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth * 0.3

                    QGCLabel { text: qsTr("Current") }
                    QGCLabel { text: isNaN(_markerAltValue) ? "\u2014" : _markerAltValue.toFixed(1) + " m"; font.bold: true }

                    Item { visible: _hasAltRange; width: ScreenTools.defaultFontPixelWidth * 0.5 }

                    QGCLabel { visible: _hasAltRange; text: qsTr("Min") }
                    QGCLabel { visible: _hasAltRange; text: _altMin.toFixed(1) + " m"; font.bold: true }

                    Item { visible: _hasAltRange; width: ScreenTools.defaultFontPixelWidth * 0.5 }

                    QGCLabel { visible: _hasAltRange; text: qsTr("Max") }
                    QGCLabel { visible: _hasAltRange; text: _altMax.toFixed(1) + " m"; font.bold: true }
                }
            }
        }
    }
}
