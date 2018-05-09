import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Button {
    property var imageSource: undefined
    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }
    property bool __showHighlight: (pressed | hovered | checked) && !__forceHoverOff

    // This fixes the issue with button hover where if a Button is near the edge oa QQuickWidget you can
    // move the mouse fast enough such that the MouseArea does not trigger an onExited. This is turn
    // cause the hover property to not be cleared correctly.

    property bool   __forceHoverOff: false
    property int    __lastGlobalMouseX: 0
    property int    __lastGlobalMouseY: 0

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
        onExited:  { __forceHoverOff; false; hoverTimer.stop()  }
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
        /*! This defines the background of the button. */
        background: Item {
            property bool __checked: (control.checkable && control.checked)

            Rectangle {
                id: backgroundRectangle
                anchors.fill: parent
                color: __showHighlight ? __qgcPal.buttonHighlight : (__checked ? __qgcPal.buttonHighlight : __qgcPal.window);
            }

            QGCColoredImage {
                id: image
                anchors.fill: parent
                opacity: control.enabled ? 0.6 : 0.5
                source: imageSource
                color: __showHighlight ? __qgcPal.buttonHighlightText : (__checked ? __qgcPal.primaryButtonText : __qgcPal.buttonText)
            }
        }
    }
}
