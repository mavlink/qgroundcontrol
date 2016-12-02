import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Button {
    property bool primary:      false                               ///< primary button for a group of buttons
    property real pointSize:    ScreenTools.defaultFontPointSize    ///< Point size for button text

    property var    _qgcPal:            QGCPalette { colorGroupEnabled: enabled }
    property bool   _showHighlight:     (pressed | hovered | checked) && !__forceHoverOff
    property bool   _showBorder:        _qgcPal.globalTheme === QGCPalette.Light

    // This fixes the issue with button hover where if a Button is near the edge oa QQuickWidget you can
    // move the mouse fast enough such that the MouseArea does not trigger an onExited. This is turn
    // cause the hover property to not be cleared correctly.

    property bool __forceHoverOff: false

    property int __lastGlobalMouseX:    0
    property int __lastGlobalMouseY:    0
    property int __padding:             Math.round(ScreenTools.defaultFontPixelHeight * 0.5)

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
                top:    __padding
                left:   __padding
                right:  control.menu !== null ? Math.round(ScreenTools.defaultFontPixelHeight) : __padding
                bottom: __padding
            }

            /*! This defines the background of the button. */
            background: Item {
                property bool down: control.pressed || (control.checkable && control.checked)
                implicitWidth:      Math.round(ScreenTools.defaultFontPixelWidth * 4.5)
                implicitHeight:     ScreenTools.isMobile ? Math.max(25, Math.round(ScreenTools.defaultFontPixelHeight * 2)) : Math.max(25, Math.round(ScreenTools.defaultFontPixelHeight * 1.2))

                Rectangle {
                    anchors.fill:   parent
                    border.width:   _showBorder ? 1: 0
                    border.color:   _qgcPal.buttonText
                    color:          _showHighlight ?
                                        control._qgcPal.buttonHighlight :
                                        (primary ? control._qgcPal.primaryButton : control._qgcPal.button)
                }

                Image {
                    id:                     imageItem
                    visible:                control.menu !== null
                    source:                 "/qmlimages/arrow-down.png"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right:          parent.right
                    anchors.rightMargin:    __padding
                    opacity:                control.enabled ? 0.6 : 0.5
                }
            }

            /*! This defines the label of the button.  */
            label: Item {
                implicitWidth:          control.menu === null ? row.implicitWidth : row.implicitWidth + ScreenTools.defaultFontPixelWidth
                implicitHeight:         row.implicitHeight
                baselineOffset:         row.y + text.y + text.baselineOffset

                Row {
                    id:                 row
                    anchors.centerIn:   parent
                    spacing:            ScreenTools.defaultFontPixelWidth * 0.25

                    Image {
                        source: control.iconSource
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        id:             text
                        antialiasing:   true
                        text:           control.text
                        font.pointSize: pointSize
                        font.family:    ScreenTools.normalFontFamily
                        anchors.verticalCenter: parent.verticalCenter
                        color: _showHighlight ?
                            control._qgcPal.buttonHighlightText :
                            (primary ? control._qgcPal.primaryButtonText : control._qgcPal.buttonText)
                    }
                }
            }
        }
}
