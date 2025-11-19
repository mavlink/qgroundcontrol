import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls


// Important Note: SubMenuButtons must manage their checked state manually in order to support
// view switch prevention. This means they can't be checkable or autoExclusive.

Button {
    id:             control
    text:           "Button"
    focusPolicy:    Qt.ClickFocus
    hoverEnabled:   !ScreenTools.isMobile
    implicitHeight: ScreenTools.defaultFontPixelHeight * 2.8

    property bool   setupComplete:  true                                    ///< true: setup complete indicator shows as completed
    property var    imageColor:     undefined
    property string imageResource:  "/qmlimages/subMenuButtonImage.png"     ///< Button image
    property bool   largeSize:      false
    property bool   showHighlight:  control.pressed || control.checked

    property size   sourceSize:     Qt.size(ScreenTools.defaultFontPixelHeight * 2, ScreenTools.defaultFontPixelHeight * 2)

    property real   _sidePadding:   ScreenTools.defaultFontPixelWidth * 1.2

    // Customization hooks for border styling (optional)
    property color  borderColor:    undefined
    property real   borderWidth:    -1

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

    property color _cyan: "#00F0FF"
    property color _panelBg: Qt.rgba(0.02, 0.10, 0.16, 0.9)
    property color _white: "#FFFFFF"

    background: Rectangle {
        id:         backgroundFrame
        implicitWidth: contentRow.implicitWidth + (_sidePadding * 2)
        implicitHeight: contentRow.implicitHeight + ScreenTools.defaultFontPixelHeight
        radius:     ScreenTools.defaultFontPixelHeight * 0.55
        color:      _panelBg
        border.color: borderColor ? borderColor : (showHighlight || control.hovered ? _cyan : _cyan)
        border.width: borderWidth > 0 ? borderWidth : 1
        layer.enabled: true
        layer.smooth:  true

        Rectangle {
            anchors.fill:   parent
            anchors.margins: 0
            color:          _cyan
            opacity:        control.pressed ? 0.22 : (control.checked ? 0.16 : (control.hovered ? 0.12 : 0.08))
            radius:         parent.radius
            visible:        true
        }

        RowLayout {
            id:                 contentRow
            anchors.fill:       parent
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.6
            spacing:            ScreenTools.defaultFontPixelWidth

            QGCColoredImage {
                id:                     image
                Layout.alignment:       Qt.AlignVCenter
                width:                  ScreenTools.defaultFontPixelHeight * 1.8
                height:                 width
                fillMode:               Image.PreserveAspectFit
                mipmap:                 true
                color:                  imageColor ? imageColor : (control.setupComplete ? _white : qgcPal.colorRed)
                source:                 control.imageResource
                sourceSize:             control.sourceSize
            }

            QGCLabel {
                id:                     titleBar
                Layout.alignment:       Qt.AlignVCenter
                verticalAlignment:      Text.AlignVCenter
                color:                  qgcPal.buttonText
                font.pointSize:         ScreenTools.mediumFontPointSize
                text:                   control.text
            }
        }
    }

    contentItem: Item {}
}
