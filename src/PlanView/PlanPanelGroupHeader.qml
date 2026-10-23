import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Group heading followed by a rule across the rest of the width, with optional trailing summary text
RowLayout {
    property alias text:    captionLabel.text
    property alias summary: summaryLabel.text

    Layout.fillWidth:   true
    spacing:            ScreenTools.defaultFontPixelHeight * 0.5

    QGCPalette { id: qgcPal }

    QGCCaptionLabel {
        id:     captionLabel
        color:  qgcPal.textPrimary
    }

    Rectangle {
        Layout.fillWidth:   true
        implicitHeight:     1
        color:              qgcPal.borderSubtle
    }

    QGCLabel {
        id:             summaryLabel
        visible:        text !== ""
        color:          qgcPal.textSecondary
        font.pointSize: ScreenTools.smallFontPointSize
    }
}
