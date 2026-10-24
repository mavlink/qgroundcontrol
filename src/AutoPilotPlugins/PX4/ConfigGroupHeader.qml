import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Group heading in an outlined rounded label like a QGCGroupBox heading, followed by a rule across the rest of the width
RowLayout {
    property alias text: captionLabel.text

    Layout.fillWidth:   true
    spacing:            ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal }

    Rectangle {
        implicitWidth:      captionLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 3
        implicitHeight:     captionLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.4
        radius:             ScreenTools.defaultBorderRadius
        color:              "transparent"
        border.width:       1
        border.color:       qgcPal.borderSubtle

        QGCCaptionLabel {
            id:                 captionLabel
            anchors.centerIn:   parent
            color:              qgcPal.textPrimary
        }
    }

    Rectangle {
        Layout.fillWidth:   true
        implicitHeight:     1
        color:              qgcPal.borderSubtle
    }
}
