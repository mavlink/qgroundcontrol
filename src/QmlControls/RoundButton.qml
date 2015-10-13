import QtQuick                  2.4
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools   1.0

Item {
    id: _root

    signal          clicked()
    property alias  buttonImage:        button.source
    property real   radius:             (ScreenTools.defaultFontPixelHeight * 3) / 2

    width:  radius * 2
    height: radius * 2

    Rectangle {
        anchors.fill:   parent
        radius:         width / 2
        border.width:   2
        border.color:   "white"
        color:          "transparent"
        Image {
            id:             button
            anchors.fill:   parent
            fillMode:       Image.PreserveAspectFit
            mipmap:         true
            smooth:         true
            MouseArea {
                anchors.fill:   parent
                onClicked:      _root.clicked()
            }
        }
    }
}
