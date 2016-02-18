import QtQuick                  2.4
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Item {
    id: _root

    signal          clicked()
    property alias  buttonImage:        button.source
    property real   radius:             ScreenTools.defaultFontPixelHeight * 1.5
    property bool   rotateImage:        false

    width:  radius * 2
    height: radius * 2

    property bool checked: false
    property ExclusiveGroup exclusiveGroup: null

    QGCPalette { id: qgcPal }

    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }

    onRotateImageChanged: {
        if (rotateImage) {
            imageRotation.running = true
        } else {
            imageRotation.running = false
            button.rotation = 0
        }
    }

    Rectangle {
        anchors.fill:   parent
        radius:         width / 2
        border.width:   ScreenTools.defaultFontPixelHeight * 0.0625
        border.color:   "white"
        color:          checked ? qgcPal.mapButtonHighlight : qgcPal.mapButton

        Image {
            id:             button
            anchors.fill:   parent
            fillMode:       Image.PreserveAspectFit
            mipmap:         true
            smooth:         true
            RotationAnimation on rotation {
                id: imageRotation
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 500
                running: false
            }

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
