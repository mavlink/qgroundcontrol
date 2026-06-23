import QtQuick
import QtQuick.Layouts
import QtGraphs

import QGroundControl
import QGroundControl.Controls

/// Shared chart base for the Log Viewer.
///
/// Provides: GraphsView with QGC-themed GraphsTheme, a HH:MM:SS x-axis
/// labelDelegate, zoom/cursor interaction, position marker line, and
/// LogViewerCursorPopup.
///
/// Derived charts (LogViewerChart, LogViewerAltChart) embed this as an Item,
/// connect to zoomRangeSet / cursorPositionSet to refresh their series data,
/// and supply popup extra rows as declarative children (forwarded via the
/// default property alias to the popup's extraContent).
Item {
    id: control

    // -------------------------------------------------------------------------
    // Required
    // -------------------------------------------------------------------------
    required property var logParser

    // -------------------------------------------------------------------------
    // Optional
    // -------------------------------------------------------------------------
    property bool xAxisShowLocalTime: false
    property string yAxisTitle: ""
    property real popupYOffset: 0   ///< added to popup y (alt chart offsets slightly)

    // -------------------------------------------------------------------------
    // Read-only aliases to child items — derived charts access these
    // -------------------------------------------------------------------------
    readonly property alias graphsView: _chart
    readonly property alias xAxis: _xAxis
    readonly property alias yAxis: _yAxis
    readonly property alias colorBlockWidth: _popup.colorBlockWidth
    readonly property real popupWidth: _popup.implicitWidth

    // -------------------------------------------------------------------------
    // Default property — children declared inside LogViewerBaseChart { } go
    // into the popup's extraContent column (chart-specific popup rows).
    // -------------------------------------------------------------------------
    default property alias content: _popup.extraContent

    // -------------------------------------------------------------------------
    // Signals
    // -------------------------------------------------------------------------

    /// Emitted only on user-driven cursor interaction.
    /// Do NOT re-emit this when handling an external sync (setSharedCursor).
    signal cursorMoved(real t)

    /// Emitted only on user-driven zoom.
    /// Do NOT re-emit this when handling an external sync (setSharedZoom).
    signal zoomApplied(real minX, real maxX)

    /// Emitted whenever the zoom range changes (user-driven or external sync).
    /// Derived charts connect here to refresh their series data.
    signal zoomRangeSet(real minX, real maxX)

    /// Emitted whenever the cursor position changes (user-driven or external sync).
    /// Derived charts connect here to update popup values (e.g. _markerAltValue).
    signal cursorPositionSet(real t)

    // -------------------------------------------------------------------------
    // State — writable by base, readable by derived charts
    // -------------------------------------------------------------------------
    property bool markerVisible: false
    property real markerPixelX: 0
    property real markerXValue: 0
    property real fullMinX: 0
    property real fullMaxX: 1
    property real zoomMinX: 0
    property real zoomMaxX: 1

    // -------------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------------

    /// Set the full and zoom range (called on log load).
    /// Emits zoomRangeSet so derived charts can refresh their series.
    function initRange(minX, maxX) {
        fullMinX   = minX
        fullMaxX   = maxX
        zoomMinX   = minX
        zoomMaxX   = maxX
        _xAxis.min = minX
        _xAxis.max = maxX
        zoomRangeSet(minX, maxX)
    }

    /// User-driven zoom — also emits zoomApplied for cross-chart sync.
    function applyZoomRange(minX, maxX) {
        _applyZoomInternal(minX, maxX)
        zoomApplied(minX, maxX)
    }

    /// External zoom sync — does not re-emit zoomApplied.
    function setSharedZoom(minX, maxX) {
        _applyZoomInternal(minX, maxX)
    }

    /// Place the cursor at axis value t and emit cursorPositionSet.
    /// Does NOT emit cursorMoved — use for both external sync and initial placement.
    function setCursor(t) {
        if (_xAxis.max <= _xAxis.min) return
        markerXValue  = t
        markerPixelX  = _axisXToPixel(t)
        markerVisible = true
        cursorPositionSet(t)
    }

    /// External cursor sync — delegates to setCursor (no cursorMoved emitted).
    function setSharedCursor(t) {
        setCursor(t)
    }

    function resetZoom() {
        applyZoomRange(fullMinX, fullMaxX)
    }

    function clearCursor() {
        markerVisible = false
    }

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------
    function _pixelToAxisX(pixelX) {
        const plotX = _chart.plotArea.x
        const plotW = _chart.plotArea.width
        if (plotW <= 0 || _xAxis.max <= _xAxis.min) return _xAxis.min
        const ratio = (Math.max(plotX, Math.min(plotX + plotW, pixelX)) - plotX) / plotW
        return _xAxis.min + ratio * (_xAxis.max - _xAxis.min)
    }

    function _axisXToPixel(axisX) {
        const plotX = _chart.plotArea.x
        const plotW = _chart.plotArea.width
        if (plotW <= 0 || _xAxis.max <= _xAxis.min) return plotX
        const ratio = (axisX - _xAxis.min) / (_xAxis.max - _xAxis.min)
        return plotX + Math.max(0, Math.min(plotW, ratio * plotW))
    }

    function _applyZoomInternal(minX, maxX) {
        if (maxX <= minX) return
        zoomMinX   = minX
        zoomMaxX   = maxX
        _xAxis.min = minX
        _xAxis.max = maxX
        if (markerVisible && (markerXValue < minX || markerXValue > maxX)) {
            markerXValue = (minX + maxX) / 2
        }
        markerPixelX = _axisXToPixel(markerXValue)
        zoomRangeSet(minX, maxX)
    }

    function _updateCursorFromInteraction(pixelX) {
        if (_xAxis.max <= _xAxis.min) return
        const plotX   = _chart.plotArea.x
        const plotW   = _chart.plotArea.width
        markerVisible = true
        markerPixelX  = Math.max(plotX, Math.min(plotX + plotW, pixelX))
        markerXValue  = _pixelToAxisX(markerPixelX)
        cursorPositionSet(markerXValue)
        cursorMoved(markerXValue)
    }

    function _popupX() {
        const plotMidX = _chart.plotArea.x + _chart.plotArea.width / 2
        if (markerPixelX < plotMidX) {
            const rightX = _chart.plotArea.x + _chart.plotArea.width - _popup.width
            return Math.max(0, Math.min(rightX, control.width - _popup.width))
        } else {
            return _chart.plotArea.x + _popup._margin
        }
    }

    // -------------------------------------------------------------------------
    // Visual tree
    // -------------------------------------------------------------------------
    QGCPalette { id: qgcPal }

    GraphsView {
        id: _chart
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
            id: _xAxis
            titleText: xAxisShowLocalTime ? qsTr("Time (local)") : qsTr("Elapsed")
            labelFormat: "%.3f"
            min: 0
            max: 1

            labelDelegate: Component {
                Item {
                    property string text: ""  // raw seconds value, assigned by axis

                    Text {
                        anchors.centerIn: parent
                        color: _chart.theme.labelTextColor
                        font: _chart.theme.axisXLabelFont
                        horizontalAlignment: Text.AlignHCenter
                        text: {
                            const secs = parseFloat(parent.text)
                            if (isNaN(secs)) { return parent.text }
                            if (xAxisShowLocalTime) {
                                const st = logParser.startTime
                                if (st && !isNaN(st.getTime()) && st.getTime() > 0) {
                                    const use12h = Qt.locale().timeFormat(Locale.ShortFormat).indexOf("a") >= 0
                                                || Qt.locale().timeFormat(Locale.ShortFormat).indexOf("A") >= 0
                                    return Qt.formatTime(new Date(st.getTime() + secs * 1000),
                                                         use12h ? "h:mm:ss AP" : "HH:mm:ss")
                                }
                            }
                            const wholeSecs = Math.floor(secs)
                            const hh = Math.floor(wholeSecs / 3600)
                            const mm = Math.floor((wholeSecs % 3600) / 60)
                            const ss = wholeSecs % 60
                            if (hh > 0) {
                                return hh + ":" + String(mm).padStart(2, "0") + ":" + String(ss).padStart(2, "0")
                            } else if (mm > 0) {
                                return mm + ":" + String(ss).padStart(2, "0")
                            }
                            return ss + "s"
                        }
                    }
                }
            }
        }

        axisY: ValueAxis {
            id: _yAxis
            titleText: yAxisTitle
            min: 0
            max: 1
        }
    }

    // Recalculate marker pixel position when the chart is resized or laid out.
    Connections {
        target: _chart
        function onPlotAreaChanged() {
            if (control.markerVisible) {
                control.markerPixelX = control._axisXToPixel(control.markerXValue)
            }
        }
    }

    // Zoom selection rectangle
    Rectangle {
        id: _zoomRect
        visible: false
        color: Qt.rgba(1, 1, 1, 0.2)
        border.color: qgcPal.buttonHighlight
        border.width: 1
        z: 10
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        enabled: _xAxis.max > _xAxis.min
        z: 11

        property real _dragStartX: 0
        property bool _isDraggingCursor: false
        property bool _isDraggingZoom: false
        property bool _wasDoubleClick: false

        onPressed: (mouse) => {
            _isDraggingCursor = false
            _isDraggingZoom   = false
            _wasDoubleClick   = false
            if (mouse.modifiers & Qt.ShiftModifier) {
                // Shift pressed — start cursor drag
                _isDraggingCursor = true
                control._updateCursorFromInteraction(mouse.x)
            } else {
                // Plain press — start zoom selection
                _dragStartX       = mouse.x
                _zoomRect.x       = mouse.x
                _zoomRect.y       = _chart.plotArea.y
                _zoomRect.width   = 0
                _zoomRect.height  = _chart.plotArea.height
                _zoomRect.visible = false   // only show once drag is meaningful
            }
        }

        onPositionChanged: (mouse) => {
            if (!pressed) return
            if (_isDraggingCursor) {
                control._updateCursorFromInteraction(mouse.x)
            } else {
                // Plain drag — update zoom selection rect
                const left  = Math.min(_dragStartX, mouse.x)
                const right = Math.max(_dragStartX, mouse.x)
                _zoomRect.x     = left
                _zoomRect.width = Math.max(0, right - left)
                if (_zoomRect.width >= ScreenTools.defaultFontPixelWidth * 0.5) {
                    _isDraggingZoom   = true
                    _zoomRect.visible = true
                }
            }
        }

        onReleased: (mouse) => {
            if (_isDraggingCursor) {
                _isDraggingCursor = false
                return
            }
            _zoomRect.visible = false
            if (_isDraggingZoom) {
                _isDraggingZoom = false
                const leftX  = control._pixelToAxisX(_zoomRect.x)
                const rightX = control._pixelToAxisX(_zoomRect.x + _zoomRect.width)
                control.applyZoomRange(Math.min(leftX, rightX), Math.max(leftX, rightX))
            } else if (!_wasDoubleClick) {
                // Tap — place cursor (suppressed on the second click of a double-click)
                control._updateCursorFromInteraction(mouse.x)
            }
        }

        onDoubleClicked: (mouse) => {
            // onDoubleClicked fires before onReleased for the second click,
            // so this flag prevents onReleased from repositioning the cursor.
            _wasDoubleClick = true
            control.resetZoom()
            const centerT = (control.fullMinX + control.fullMaxX) / 2
            control.setCursor(centerT)
            control.cursorMoved(centerT)
        }
    }

    // Position marker line
    Rectangle {
        visible: markerVisible
        x: markerPixelX
        y: _chart.plotArea.y
        width: 1
        height: _chart.plotArea.height
        color: qgcPal.text
        z: 12
    }

    // Cursor popup — chart-specific rows are injected via default property alias
    LogViewerCursorPopup {
        id: _popup
        visible: markerVisible
        x: _popupX()
        y: _chart.plotArea.y + popupYOffset
        z: 13
        cursorXValue: markerXValue
        logParser: control.logParser
        xAxisShowLocalTime: control.xAxisShowLocalTime
        zoomMinX: control.zoomMinX
        zoomMaxX: control.zoomMaxX
        plotAreaWidth: _chart.plotArea.width
    }
}
