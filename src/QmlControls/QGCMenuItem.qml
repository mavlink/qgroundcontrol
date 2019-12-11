import QtQuick          2.11
import QtQuick.Controls 2.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

MenuItem {
    id: _root

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    implicitWidth: visible ? contentItem.implicitWidth : 0
    implicitHeight: visible ? contentItem.implicitHeight * 1.5 : 0

    arrow: Canvas {
        x: parent.width - width
        implicitWidth: parent.implicitHeight
        implicitHeight: implicitWidth
        visible: _root.subMenu
        property real _arrwSize: implicitWidth * 0.2

        onPaint: {
            var ctx = getContext("2d")
            ctx.fillStyle = textLabel.color
            ctx.globalAlpha = background.opacity
            ctx.moveTo(_arrwSize, _arrwSize)
            ctx.lineTo(width/2, height / 2)
            ctx.lineTo(_arrwSize, height - _arrwSize)
            ctx.closePath()
            ctx.fill()
        }
    }

    indicator: Item {
        implicitWidth: implicitHeight
        implicitHeight: parent.implicitHeight
        Rectangle {
            width: parent.height * 0.7
            height: width
            anchors.centerIn: parent
            visible: _root.checkable
            border.color: qgcPal.windowShadeDark
            radius: ScreenTools.defaultFontPixelWidth * 0.2
            Rectangle {
                width: parent.height * 0.6
                height: width
                anchors.centerIn: parent
                visible: _root.checked
                color: qgcPal.buttonHighlight
                radius: ScreenTools.defaultFontPixelWidth * 0.2
            }
        }
    }

    contentItem: Text {
        id: textLabel
        leftPadding: _root.indicator.width
        rightPadding: _root.arrow.width
        text: _root.text
        font.pointSize: ScreenTools.defaultFontPointSize * 1.25
        opacity: enabled ? 1.0 : 0.3
        color: _root.highlighted ? qgcPal.buttonHighlightText : qgcPal.buttonText
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: _root.highlighted ? qgcPal.buttonHighlight : qgcPal.window
        opacity: _root.enabled ? 1 : 0.5
    }
}
