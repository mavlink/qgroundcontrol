// QtQuick.Control 1.x Menu

import QtQuick          2.11
import QtQuick.Controls 2.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

MenuItem {
    id: _root

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    implicitWidth: textLabel.contentWidth * 1.2
    implicitHeight: textLabel.contentHeight * 2

    background: Rectangle {
        anchors.fill: _root
        color: _root.highlighted ? qgcPal.buttonHighlight : qgcPal.window
    }

    contentItem: Text {
        id: textLabel
        text: _root.text
        color: qgcPal.text
        font.pointSize: ScreenTools.defaultFontPixelHeight
        verticalAlignment: Text.AlignVCenter
    }
}
