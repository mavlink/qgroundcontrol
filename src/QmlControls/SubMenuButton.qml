import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtGraphicalEffects       1.0

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Button {
    property bool   setupComplete:  true                                    ///< true: setup complete indicator shows as completed
    property bool   setupIndicator: true                                    ///< true: show setup complete indicator
    property string imageResource:  "/qmlimages/subMenuButtonImage.png"     ///< Button image

    text: "Button"  ///< Pass in your own button text

    checkable:      true
    implicitHeight: ScreenTools.isTinyScreen ? ScreenTools.defaultFontPixelHeight * 3.5 : ScreenTools.defaultFontPixelHeight * 2.5

    style: ButtonStyle {
        id: buttonStyle

        QGCPalette {
            id:                 qgcPal
            colorGroupEnabled:  control.enabled
        }

        property bool showHighlight: control.pressed | control.checked

        background: Rectangle {
            id:     innerRect
            color:  showHighlight ? qgcPal.buttonHighlight : qgcPal.windowShade

            implicitWidth: titleBar.x + titleBar.contentWidth + ScreenTools.defaultFontPixelWidth

            QGCColoredImage {
                id:                     image
                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
                anchors.left:           parent.left
                anchors.verticalCenter: parent.verticalCenter
                width:                  ScreenTools.defaultFontPixelHeight * 2
                height:                 ScreenTools.defaultFontPixelHeight * 2
                fillMode:               Image.PreserveAspectFit
                smooth:                 true
                color:                  control.setupComplete ? qgcPal.button : "red"
                source:                 control.imageResource
            }

            QGCLabel {
                id:                     titleBar
                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
                anchors.left:           image.right
                anchors.verticalCenter: parent.verticalCenter
                verticalAlignment:      TextEdit.AlignVCenter
                color:                  showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
                text:                   control.text
            }
        }

        label: Item {}
    }
}
