import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Vertical signal bar with SNR-proportional height and threshold-based coloring.
/// Used for satellite signal strength displays.
Rectangle {
    id: root

    property real   snr: 0
    property bool   used: false
    property real   maxSnr: 50.0
    property real   barHeight: ScreenTools.defaultFontPixelHeight * 2


    width:  implicitWidth
    height: barHeight
    color:  "transparent"

    implicitWidth: ScreenTools.defaultFontPixelWidth * 1.2

    QGCPalette { id: _pal }

    Rectangle {
        anchors.bottom:           parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width:  parent.width - 1
        height: Math.max(2, parent.height * (root.snr / root.maxSnr))
        radius: 1
        color: {
            if (!root.used) return _pal.colorGrey
            if (root.snr >= GPSSNRThresholds.strong) return _pal.colorGreen
            if (root.snr >= GPSSNRThresholds.weak)   return _pal.colorOrange
            return _pal.colorRed
        }
    }
}
