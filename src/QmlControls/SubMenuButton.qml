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

    checkable:  true
    height:     ScreenTools.defaultFontPixelHeight * 5

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

            readonly property real titleHeight: ScreenTools.defaultFontPixelHeight * 1.5

            QGCLabel {
                id:                     titleBar
                width:                  parent.width
                height:                 parent.titleHeight
                verticalAlignment:      TextEdit.AlignVCenter
                horizontalAlignment:    TextEdit.AlignHCenter
                color:                  showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
                text:                   control.text

                Rectangle {
                    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth / 3
                    anchors.right:          parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width:                  ScreenTools.defaultFontPixelWidth
                    height:                 width
                    radius:                 width / 2
                    color:                  control.setupComplete ? "#00d932" : "red"
                    visible:                control.setupIndicator
                }
            }

            Rectangle {
                anchors.top:    titleBar.bottom
                anchors.bottom: parent.bottom
                width:          parent.width
                color:          qgcPal.windowShadeDark

                QGCColoredImage {
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * .75
                    anchors.fill:       parent
                    fillMode:           Image.PreserveAspectFit
                    smooth:             true
                    color:              showHighlight ? qgcPal.buttonHighlight : qgcPal.button
                    source:             control.imageResource
                }
            }
        }

        label: Item {}
    }
}
