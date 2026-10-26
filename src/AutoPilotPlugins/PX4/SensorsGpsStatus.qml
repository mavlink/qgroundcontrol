import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// What the GPS offers a calibration which is matched to the GPS: a coloured dot, the fix, and its accuracy and satellites.
/// gps is the result of SensorsRows.gpsReadiness().
ColumnLayout {
    id: control

    required property var gps

    spacing: 0

    QGCPalette { id: qgcPal }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        Rectangle {
            implicitWidth:  ScreenTools.defaultFontPixelHeight * 0.6
            implicitHeight: implicitWidth
            radius:         implicitWidth / 2
            color:          control.gps.level === "none" ? qgcPal.colorRed :
                                control.gps.level === "poor" ? qgcPal.colorOrange : qgcPal.colorGreen
        }

        QGCLabel {
            text:       control.gps.summary
            font.bold:  true
            color:      control.gps.level === "none" ? qgcPal.errorText :
                            control.gps.level === "poor" ? qgcPal.warningText : qgcPal.successText
        }
    }

    QGCLabel {
        text:           control.gps.detail
        color:          qgcPal.textSecondary
        font.pointSize: ScreenTools.smallFontPointSize
    }
}
