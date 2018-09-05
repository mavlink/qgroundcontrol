import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Button {
    activeFocusOnPress: true

    property bool   primary:        false                               ///< primary button for a group of buttons
    property real   pointSize:      ScreenTools.defaultFontPointSize    ///< Point size for button text
    property bool   showBorder:     _qgcPal.globalTheme === QGCPalette.Light
    property bool   iconLeft:       false
    property real   backRadius:     0
    property real   heightFactor:   0.5

    property var    _qgcPal:            QGCPalette { colorGroupEnabled: enabled }
    property bool   _showHighlight:     (pressed | hovered | checked) && !__forceHoverOff

    // This fixes the issue with button hover where if a Button is near the edge oa QQuickWidget you can
    // move the mouse fast enough such that the MouseArea does not trigger an onExited. This is turn
    // cause the hover property to not be cleared correctly.

    property bool __forceHoverOff: false

    property int __lastGlobalMouseX:    0
    property int __lastGlobalMouseY:    0
    property int _horizontalPadding:    ScreenTools.defaultFontPixelWidth
    property int _verticalPadding:      Math.round(ScreenTools.defaultFontPixelHeight * heightFactor)

    Connections {
        target: __behavior
        onMouseXChanged: {
            __lastGlobalMouseX = ScreenTools.mouseX()
            __lastGlobalMouseY = ScreenTools.mouseY()
        }
        onMouseYChanged: {
            __lastGlobalMouseX = ScreenTools.mouseX()
            __lastGlobalMouseY = ScreenTools.mouseY()
        }
        onEntered: { __forceHoverOff = false; hoverTimer.start() }
        onExited: { __forceHoverOff = false; hoverTimer.stop() }
    }

    Timer {
        id:         hoverTimer
        interval:   250
        repeat:     true
        onTriggered: {
            __forceHoverOff = (__lastGlobalMouseX !== ScreenTools.mouseX() || __lastGlobalMouseY !== ScreenTools.mouseY());
        }
    }

    style: ButtonStyle {
            /*! The padding between the background and the label components. */
            padding {
                top:    _verticalPadding
                bottom: _verticalPadding
                left:   _horizontalPadding
                right:  _horizontalPadding
            }

            /*! This defines the background of the button. */
            background: Rectangle {
                id:             backRect
                implicitWidth:  ScreenTools.implicitButtonWidth
                implicitHeight: ScreenTools.implicitButtonHeight
                radius:         backRadius
                border.width:   showBorder ? 1 : 0
                border.color:   _qgcPal.buttonText
                color:          _showHighlight ?
                                    control._qgcPal.buttonHighlight :
                                    (primary ? control._qgcPal.primaryButton : control._qgcPal.button)
            }

            /*! This defines the label of the button.  */
            label: Item {
                implicitWidth:          text.implicitWidth + icon.width
                implicitHeight:         text.implicitHeight
                baselineOffset:         text.y + text.baselineOffset

                QGCColoredImage {
                    id:                     icon
                    source:                 control.iconSource
                    height:                 source === "" ? 0 : text.height
                    width:                  height
                    color:                  text.color
                    fillMode:               Image.PreserveAspectFit
                    sourceSize.height:      height
                    anchors.left:           control.iconLeft ? parent.left : undefined
                    anchors.leftMargin:     control.iconLeft ? ScreenTools.defaultFontPixelWidth : undefined
                    anchors.right:          !control.iconLeft ? parent.right : undefined
                    anchors.rightMargin:    !control.iconLeft ? ScreenTools.defaultFontPixelWidth : undefined
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    id:                     text
                    anchors.centerIn:       parent
                    antialiasing:           true
                    text:                   control.text
                    font.pointSize:         pointSize
                    font.family:            ScreenTools.normalFontFamily
                    color:                  _showHighlight ?
                                                control._qgcPal.buttonHighlightText :
                                                (primary ? control._qgcPal.primaryButtonText : control._qgcPal.buttonText)
                }
            }
        }
}
