import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    id: root

    property color  statusColor:    qgcPal.colorGrey
    property string statusText:     ""
    property string buttonText:     ""
    property bool   buttonVisible:  true
    property bool   buttonEnabled:  true

    signal buttonClicked

    Layout.fillWidth: true
    spacing: ScreenTools.defaultFontPixelWidth

    QGCPalette { id: qgcPal }

    Rectangle {
        width:  ScreenTools.defaultFontPixelHeight * 0.75
        height: width
        radius: width / 2
        color:  root.statusColor
        Layout.alignment: Qt.AlignVCenter
    }

    QGCLabel {
        Layout.fillWidth: true
        text:     root.statusText
        wrapMode: Text.WordWrap
    }

    QGCButton {
        text:    root.buttonText
        visible: root.buttonVisible
        enabled: root.buttonEnabled
        onClicked: root.buttonClicked()
    }
}
