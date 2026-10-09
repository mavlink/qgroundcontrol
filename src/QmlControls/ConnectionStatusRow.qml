import QtQuick
import QtQuick.Layouts

import QGroundControl.Controls

RowLayout {
    id: root

    property color  statusColor
    property string statusText
    property string buttonText
    property bool   buttonEnabled: true
    property string buttonObjectName: ""

    signal clicked()

    spacing: ScreenTools.defaultFontPixelWidth

    FixStatusDot {
        Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 0.75
        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.75
        Layout.alignment:       Qt.AlignVCenter
        statusColor:            root.statusColor
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
        text:             root.statusText
        textFormat:       Text.PlainText
        wrapMode:         Text.WordWrap
    }

    QGCButton {
        objectName: root.buttonObjectName
        text:      root.buttonText
        enabled:   root.buttonEnabled
        visible:   root.buttonText !== ""
        onClicked: root.clicked()
    }
}
