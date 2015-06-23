import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Button {
    // primary: true - this is the primary button for this group of buttons
    property bool primary: false

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    property bool __showHighlight: (pressed | hovered | checked) && !__forceHoverOff

    // This fixes the issue with button hover where if a Button is near the edge oa QQuickWidget you can
    // move the mouse fast enough such that the MouseArea does not trigger an onExited. This is turn
    // cause the hover property to not be cleared correctly.

    property bool __forceHoverOff: false

    property int __lastGlobalMouseX: 0
    property int __lastGlobalMouseY: 0

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
        onEntered: { __forceHoverOff; false; hoverTimer.start() }
        onExited: { __forceHoverOff; false; hoverTimer.stop() }
    }

    Timer {
        id:         hoverTimer
        interval:   250
        repeat:     true

        onTriggered: {
            if (__lastGlobalMouseX != ScreenTools.mouseX() || __lastGlobalMouseY != ScreenTools.mouseY()) {
                __forceHoverOff = true
            } else {
                __forceHoverOff = false
            }
        }
    }

    style: ButtonStyle {
            /*! The padding between the background and the label components. */
            padding {
                top: 4
                left: 4
                right:  control.menu !== null ? Math.round(TextSingleton.implicitHeight * 0.5) : 4
                bottom: 4
            }

            /*! This defines the background of the button. */
            background: Item {
                property bool down: control.pressed || (control.checkable && control.checked)
                implicitWidth: Math.round(TextSingleton.implicitHeight * 4.5)
                implicitHeight: Math.max(25, Math.round(TextSingleton.implicitHeight * 1.2))

                Rectangle {
                    anchors.fill: parent
                    color: __showHighlight ?
                        control.__qgcPal.buttonHighlight :
                        (primary ? control.__qgcPal.primaryButton : control.__qgcPal.button)
                }

                Image {
                    id: imageItem
                    visible: control.menu !== null
                    source: "/qmlimages/arrow-down.png"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: padding.right
                    opacity: control.enabled ? 0.6 : 0.5
                }
            }

            /*! This defines the label of the button.  */
            label: Item {
                implicitWidth: row.implicitWidth
                implicitHeight: row.implicitHeight
                baselineOffset: row.y + text.y + text.baselineOffset

                Row {
                    id: row
                    anchors.centerIn: parent
                    spacing: 2

                    Image {
                        source: control.iconSource
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        id:             text
                        antialiasing:   true
                        text:           control.text
                        font.pixelSize: ScreenTools.defaultFontPixelSize

                        anchors.verticalCenter: parent.verticalCenter

                        color: __showHighlight ?
                            control.__qgcPal.buttonHighlightText :
                            (primary ? control.__qgcPal.primaryButtonText : control.__qgcPal.buttonText)
                    }
                }
            }
        }
}
