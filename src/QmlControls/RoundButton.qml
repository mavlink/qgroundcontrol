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

    property bool checked: false
    property ExclusiveGroup exclusiveGroup: null

    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }

    Rectangle {
        anchors.fill:   parent
        radius:         width / 2
        border.width:   2
        border.color:   "white"
        opacity:        checked ? 0.95 : 0.65
        color:          checked ? "orange" : "black"
        Image {
            id:             button
            anchors.fill:   parent
            fillMode:       Image.PreserveAspectFit
            mipmap:         true
            smooth:         true
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    checked = !checked
                    _root.clicked()
                }
            }
        }
    }
}
