import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools 1.0

/// Mission item summary display control
Rectangle {
    property var missionItem        ///< Mission Item object
    property int missionItemIndex   ///< Index for this item

    width:          ScreenTools.defaultFontPixelWidth * 15
    height:         ScreenTools.defaultFontPixelWidth * 3
    border.width:   2
    border.color:   "white"
    color:          "white"
    opacity:        0.75
    radius:         ScreenTools.defaultFontPixelWidth
    
    QGCLabel {
        anchors.margins:        parent.radius / 2
        anchors.left:           parent.left
        anchors.top:            parent.top
        color:                  "black"
        horizontalAlignment:    Text.AlignTop
        text:                   missionItem.type
    }
    
    MissionItemIndexLabel {
        anchors.top:        parent.top
        anchors.right:      parent.right
        missionItemIndex:   parent.missionItemIndex + 1
    }
}
