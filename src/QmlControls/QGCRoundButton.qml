import QtQuick
import QtQuick.Controls

import QGroundControl.ScreenTools
import QGroundControl.Palette

Item {
    id: _root

    signal          clicked()
    property alias  buttonImage:    button.source
    property real   radius:         ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 1.75 : ScreenTools.defaultFontPixelHeight * 1.25
    property bool   rotateImage:    false
    property bool   lightBorders:   true

    width:  radius * 2
    height: radius * 2

    property bool checked: false
    property ButtonGroup buttonGroup: null

    QGCPalette { id: qgcPal }

    onButtonGroupChanged: {
        if (buttonGroup) {
            buttonGroup.addButton(_root)
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
        border.color:   lightBorders ? qgcPal.mapWidgetBorderLight : qgcPal.mapWidgetBorderDark
        color:          checked ? qgcPal.buttonHighlight : qgcPal.button

        QGCColoredImage {
            id:                 button
            anchors.fill:       parent
            sourceSize.height:  parent.height
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            color:              checked ? qgcPal.buttonHighlightText : qgcPal.buttonText

            RotationAnimation on rotation {
                id:             imageRotation
                loops:          Animation.Infinite
                from:           0
                to:             360
                duration:       500
                running:        false
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
