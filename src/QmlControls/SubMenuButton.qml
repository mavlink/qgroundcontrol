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
    property real   activeStripWidth: Math.max(2, ScreenTools.defaultFontPixelWidth * 0.28)

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
        color:  showHighlight ? Qt.rgba(0.18, 0.20, 0.22, 0.96)
                              : (control.enabled && control.hovered ? Qt.rgba(1, 1, 1, 0.055) : "transparent")
        radius: Math.round(ScreenTools.defaultFontPixelWidth * 0.65)

        implicitWidth: titleBar.x + titleBar.contentWidth + ScreenTools.defaultFontPixelWidth

        Rectangle {
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            width:                  control.activeStripWidth
            height:                 parent.height * 0.58
            radius:                 width / 2
            color:                  qgcPal.primaryButton
            visible:                showHighlight
        }

        QGCColoredImage {
            id:                     image
            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            width:                  ScreenTools.defaultFontPixelHeight * 1.65
            height:                 ScreenTools.defaultFontPixelHeight * 1.65
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
