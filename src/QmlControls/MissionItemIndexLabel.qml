import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools 1.0

Rectangle {
    property alias  label:          _label.text
    property bool   isCurrentItem:  false

    signal clicked

    width:          ScreenTools.defaultFontPixelHeight * 1.5
    height:         width
    radius:         width / 2
    border.width:   2
    border.color:   "white"
    color:          isCurrentItem ? "green" : "orange"

    MouseArea {
        anchors.fill: parent

        onClicked: parent.clicked()
    }

    QGCLabel {
        id:                     _label
        anchors.fill:           parent
        horizontalAlignment:    Text.AlignHCenter
        verticalAlignment:      Text.AlignVCenter
        color:                  "white"
    }
}
