import QtQuick
import QtQuick.Controls
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

// Important Note: SubMenuButtons must manage their checked state manually in order to support
// view switch prevention. This means they can't be checkable or autoExclusive.

Button {
    id:             control
    property bool   setupComplete:  true                                    ///< true: setup complete indicator shows as completed
    property bool   setupIndicator: true                                    ///< true: show setup complete indicator
    property var    imageColor:     undefined
    property string imageResource:  "/qmlimages/subMenuButtonImage.png"     ///< Button image
    property size   sourceSize:     Qt.size(ScreenTools.defaultFontPixelHeight * 2, ScreenTools.defaultFontPixelHeight * 2)

    text:               "Button"  ///< Pass in your own button text
    focusPolicy:    Qt.ClickFocus

    implicitHeight: ScreenTools.isTinyScreen ? ScreenTools.defaultFontPixelHeight * 3.5 : ScreenTools.defaultFontPixelHeight * 2.5
    //implicitWidth:  __panel.implicitWidth

    property ButtonGroup buttonGroup:    null
    onButtonGroupChanged: {
        if (buttonGroup) {
            buttonGroup.addButton(control)
        }
    }

    onCheckedChanged: checkable = false

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
            mipmap:                 true
            color:                  imageColor ? imageColor : (control.setupComplete ? qgcPal.button : "red")
            source:                 control.imageResource
            sourceSize:             control.sourceSize
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

    contentItem: Item {}
}
