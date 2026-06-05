import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Shared cursor-position info popup used by LogViewerChart and LogViewerAltChart.
///
/// Set required properties and place chart-specific rows as children — they
/// appear below the shared time label via the default property alias.
///
/// Caller is responsible for setting x/y and visible.
Rectangle {
    id: control

    // -------------------------------------------------------------------------
    // Required properties
    // -------------------------------------------------------------------------
    required property real cursorXValue        ///< seconds from log start
    required property var logParser           ///< for logParser.startTime
    required property bool xAxisShowLocalTime  ///< elapsed vs. local clock
    required property real zoomMinX
    required property real zoomMaxX
    required property real plotAreaWidth       ///< drives sub-second decimal count

    // -------------------------------------------------------------------------
    // Chart-specific content — injected as children of this item
    // -------------------------------------------------------------------------
    default property alias extraContent: _extraColumn.data

    // Exposed so callers can align indented rows to the color block
    readonly property real colorBlockWidth: ScreenTools.defaultFontPixelHeight * 0.8

    // -------------------------------------------------------------------------
    // Styling
    // -------------------------------------------------------------------------
    readonly property real _margin: ScreenTools.defaultFontPixelWidth / 2

    color: qgcPal.windowShade
    border.color: qgcPal.windowShadeDark
    radius: ScreenTools.defaultFontPixelWidth * 0.3
    implicitWidth: _mainColumn.implicitWidth  + _margin * 2
    implicitHeight: _mainColumn.implicitHeight + _margin * 2

    QGCPalette { id: qgcPal }

    // -------------------------------------------------------------------------
    // Content
    // -------------------------------------------------------------------------
    ColumnLayout {
        id: _mainColumn
        anchors.fill: parent
        anchors.margins: control._margin
        spacing: ScreenTools.defaultFontPixelHeight * 0.2

        // ---- Shared time label ----
        QGCLabel {
            font.bold: true
            text: {
                const secsPerPixel = control.plotAreaWidth > 0
                        ? (control.zoomMaxX - control.zoomMinX) / control.plotAreaWidth : 1.0
                const decimals = secsPerPixel < 0.1 ? 2 : secsPerPixel < 1.0 ? 1 : 0

                const wholeSecs = Math.floor(control.cursorXValue)
                const frac      = control.cursorXValue - wholeSecs
                const hh = Math.floor(wholeSecs / 3600)
                const mm = Math.floor((wholeSecs % 3600) / 60)
                const ss = wholeSecs % 60
                const fracStr = decimals > 0 ? frac.toFixed(decimals).slice(1) : ""

                let elapsed = ""
                if (hh > 0) {
                    elapsed = hh + ":" + String(mm).padStart(2, "0") + ":" + String(ss).padStart(2, "0") + fracStr
                } else if (mm > 0) {
                    elapsed = mm + ":" + String(ss).padStart(2, "0") + fracStr
                } else {
                    elapsed = decimals > 0 ? (ss + frac).toFixed(decimals) + "s" : ss + "s"
                }

                if (control.xAxisShowLocalTime) {
                    const st = control.logParser.startTime
                    if (st && !isNaN(st.getTime()) && st.getTime() > 0) {
                        const use12h = Qt.locale().timeFormat(Locale.ShortFormat).indexOf("a") >= 0
                                    || Qt.locale().timeFormat(Locale.ShortFormat).indexOf("A") >= 0
                        const local = Qt.formatTime(new Date(st.getTime() + control.cursorXValue * 1000),
                                                     use12h ? "h:mm:ss AP" : "HH:mm:ss")
                        return local + qsTr(" (local)  /  ") + elapsed + qsTr(" (elapsed)")
                    }
                }
                return elapsed + qsTr(" (elapsed)")
            }
        }

        // ---- Chart-specific rows (provided by each chart as children) ----
        ColumnLayout {
            id: _extraColumn
            spacing: ScreenTools.defaultFontPixelHeight * 0.2
        }
    }
}
