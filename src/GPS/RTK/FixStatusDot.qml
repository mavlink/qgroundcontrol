import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// Colored circle indicating GPS fix or general status.
/// Pick ONE of:
///   - `lockValue`   — GPS fix-type lock (uses GPSFormatter.fixTypeColor)
///   - `severity`    — "Info" | "Warning" | "Error" (e.g. event log entries)
///   - `statusColor` — set directly (other status semantics)
Item {
    id: root

    property int    lockValue: -1
    property string severity: ""
    property color  statusColor: _resolvedColor
    property real   dotSize: ScreenTools.defaultFontPixelHeight * 0.6

    Layout.preferredWidth:  dotSize
    Layout.preferredHeight: dotSize
    Layout.alignment:       Qt.AlignVCenter

    implicitWidth:  dotSize
    implicitHeight: dotSize

    QGCPalette { id: _pal }

    property color _resolvedColor: {
        if (severity !== "") {
            if (severity === "Error")   return _pal.colorRed
            if (severity === "Warning") return _pal.colorOrange
            return _pal.colorGreen
        }
        if (lockValue < 0) return _pal.colorGrey
        var c = GPSFormatter.fixTypeColor(lockValue)
        if (c === "green") return _pal.colorGreen
        if (c === "orange") return _pal.colorOrange
        if (c === "red") return _pal.colorRed
        return _pal.colorGrey
    }

    Rectangle {
        anchors.fill: parent
        radius:       width / 2
        color:        root.statusColor
    }
}
