import QtQuick          2.11
import QtQuick.Controls 2.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

MenuItem {
    id: _root

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    implicitWidth: visible ? contentItem.implicitWidth : 0
    implicitHeight: visible ? contentItem.implicitHeight : 0

    arrow: Item {
        x: parent.width - width
        y: (parent.height - height)/2
        implicitWidth: ScreenTools.defaultFontPixelWidth * 4
        implicitHeight: ScreenTools.defaultFontPixelWidth * 6

        Canvas {
            anchors.fill: parent
            anchors.margins: ScreenTools.defaultFontPixelWidth
            visible: _root.subMenu
            onPaint: {
                var ctx = getContext("2d")
                ctx.fillStyle = textLabel.color
                ctx.globalAlpha = background.opacity
                ctx.moveTo(0, 0)
                ctx.lineTo(width, height / 2)
                ctx.lineTo(0, height)
                ctx.closePath()
                ctx.fill()
            }
        }
    }

    indicator: Item {
        x: 0
        y: (parent.height - height)/2
        implicitWidth: ScreenTools.defaultFontPixelWidth * 6
        implicitHeight: implicitWidth
        Rectangle {
            anchors.fill: parent
            anchors.margins: ScreenTools.defaultFontPixelWidth
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
        topPadding: ScreenTools.defaultFontPixelHeight * 0.5
        bottomPadding: topPadding
        text: _root.text
        font.pointSize: ScreenTools.defaultFontPointSize * 1.25
        opacity: enabled ? 1.0 : 0.3
        color: _root.highlighted ? qgcPal.buttonHighlightText : qgcPal.buttonText
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        color: _root.highlighted ? qgcPal.buttonHighlight : qgcPal.window
        opacity: _root.enabled ? 1 : 0.5
    }
}
