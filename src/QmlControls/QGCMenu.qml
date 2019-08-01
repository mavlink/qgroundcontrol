// QtQuick.Control 1.x Menu

import QtQuick          2.6
import QtQuick.Controls 2.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Menu {

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 40
        color: "#ffffff"
        border.color: "#21be2b"
        radius: 2
    }
}
