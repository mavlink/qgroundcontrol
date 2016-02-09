import QtQuick                  2.5
import QtQuick.Controls         1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Rectangle
{
    id: __mapButton

    property var    __qgcPal: QGCPalette { colorGroupEnabled: enabled }
    property bool   __showHighlight: (__pressed | __hovered | checked) && !__forceHoverOff

    property bool   __forceHoverOff:    false
    property int    __lastGlobalMouseX: 0
    property int    __lastGlobalMouseY: 0
    property bool   __pressed:          false
    property bool   __hovered:          false

    property bool   checked:    false
    property bool   complete:   false
    property alias  text:       nameLabel.text
    property alias  size:       sizeLabel.text

    signal clicked()

    color:          __showHighlight ? __qgcPal.buttonHighlight : __qgcPal.button
    anchors.margins: ScreenTools.defaultFontPixelWidth
    Row {
        anchors.centerIn: parent
        QGCLabel {
            id:     nameLabel
            width:  __mapButton.width * 0.4
            color:  __showHighlight ? __qgcPal.buttonHighlightText : __qgcPal.buttonText
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCLabel {
            id:     sizeLabel
            width:  __mapButton.width * 0.4
            horizontalAlignment: Text.AlignRight
            anchors.verticalCenter: parent.verticalCenter
            color:  __showHighlight ? __qgcPal.buttonHighlightText : __qgcPal.buttonText
        }
        Item {
            width:  ScreenTools.defaultFontPixelWidth * 2
            height: 1
        }
        Rectangle {
            width:   sizeLabel.height * 0.5
            height:  sizeLabel.height * 0.5
            radius:  width / 2
            color:   complete ? "#31f55b" : "#fc5656"
            opacity: sizeLabel.text.length > 0 ? 1 : 0
            anchors.verticalCenter: parent.verticalCenter
        }
        Item {
            width:  ScreenTools.defaultFontPixelWidth * 2
            height: 1
        }
        QGCColoredImage {
            width:      sizeLabel.height * 0.8
            height:     sizeLabel.height * 0.8
            source:     "/res/buttonRight.svg"
            mipmap:     true
            fillMode:   Image.PreserveAspectFit
            color:      __showHighlight ? __qgcPal.buttonHighlightText : __qgcPal.buttonText
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onMouseXChanged: {
            __lastGlobalMouseX = ScreenTools.mouseX()
            __lastGlobalMouseY = ScreenTools.mouseY()
        }
        onMouseYChanged: {
            __lastGlobalMouseX = ScreenTools.mouseX()
            __lastGlobalMouseY = ScreenTools.mouseY()
        }
        onEntered:  { __hovered = true;  __forceHoverOff = false; hoverTimer.start() }
        onExited:   { __hovered = false; __forceHoverOff = false; hoverTimer.stop()  }
        onPressed:  { __pressed = true;  }
        onReleased: { __pressed = false; }
        onClicked: {
            __mapButton.clicked()
        }
    }

    Timer {
        id:         hoverTimer
        interval:   250
        repeat:     true
        onTriggered: {
            if (__lastGlobalMouseX !== ScreenTools.mouseX() || __lastGlobalMouseY !== ScreenTools.mouseY()) {
                __forceHoverOff = true
            } else {
                __forceHoverOff = false
            }
        }
    }
}
