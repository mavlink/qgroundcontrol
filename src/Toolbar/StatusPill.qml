import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Small colored pill label. Hidden when text is empty.
Rectangle {
    id: control

    property string text: ""
    property real fontPointSize: ScreenTools.smallFontPointSize

    width: statusLabel.implicitWidth + ScreenTools.defaultFontPixelWidth
    height: statusLabel.implicitHeight
    radius: height / 2
    visible: control.text.length > 0

    QGCLabel {
        id: statusLabel

        anchors.centerIn: parent
        text: control.text
        color: QGroundControl.globalPalette.indicatorText
        font.pointSize: control.fontPointSize
        font.bold: true
    }
}
