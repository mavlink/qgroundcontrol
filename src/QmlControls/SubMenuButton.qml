import QtQuick
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

// Important Note: SubMenuButtons must manage their checked state manually in order to support
// view switch prevention. This means they can't be checkable or autoExclusive.

Button {
    id:             control
    text:           "Button"
    focusPolicy:    Qt.ClickFocus
    hoverEnabled:   !ScreenTools.isMobile
    implicitHeight: ScreenTools.defaultFontPixelHeight * 2.5

    property bool   setupComplete:  true                                    ///< true: setup complete indicator shows as completed
    property var    imageColor:     undefined
    property string imageResource:  "/qmlimages/subMenuButtonImage.png"     ///< Button image
    property bool   largeSize:      false
    property bool   showHighlight:  control.pressed | control.checked

    property size   sourceSize:     Qt.size(ScreenTools.defaultFontPixelHeight * 2, ScreenTools.defaultFontPixelHeight * 2)

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

    background: Rectangle {
        id:     innerRect
        color:  qgcPal.windowShade

        implicitWidth: titleBar.x + titleBar.contentWidth + ScreenTools.defaultFontPixelWidth

        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.buttonHighlight
            opacity:        showHighlight ? 1 : control.enabled && control.hovered ? .2 : 0
        }

        QGCColoredImage {
            id:                     image
            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            width:                  ScreenTools.defaultFontPixelHeight * 2
            height:                 ScreenTools.defaultFontPixelHeight * 2
            fillMode:               Image.PreserveAspectFit
            mipmap:                 true
            color:                  imageColor ? imageColor : (control.setupComplete ? titleBar.color : "red")
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
