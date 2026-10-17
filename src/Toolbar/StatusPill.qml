import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Small colored pill label. Hidden when text is empty.
Rectangle {
    id: control

    width: statusLabel.implicitWidth + ScreenTools.defaultFontPixelWidth
    height: statusLabel.implicitHeight
    radius: height / 2
    visible: control.text.length > 0

    property string text: ""

    QGCLabel {
        id: statusLabel

        anchors.centerIn: parent
        text: control.text
        color: QGroundControl.globalPalette.indicatorText
        font.pointSize: ScreenTools.smallFontPointSize
        font.bold: true
    }
}
