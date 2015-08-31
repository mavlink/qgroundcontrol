import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools 1.0

Rectangle {
    property int missionItemIndex  ///< Index to show in label

    width:          ScreenTools.defaultFontPixelHeight * 1.5
    height:         width
    radius:         width / 2
    border.width:   2
    border.color:   "white"
    color:          "orange"

    QGCLabel {
        anchors.fill:           parent
        horizontalAlignment:    Text.AlignHCenter
        verticalAlignment:      Text.AlignVCenter
        color:                  "white"
        text:                   missionItemIndex
    }
}
