// QtQuick.Control 1.x Menu

import QtQuick          2.11
import QtQuick.Controls 2.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Menu {

    id: _root

    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    QGCPalette { id: qgcPalDisable; colorGroupEnabled: false }

    delegate: QGCMenuItem {
    }

    background: Rectangle {

        implicitWidth: {
            var result = 0;
            for (var i = 0; i < count; ++i) {
                result = Math.max(itemAt(i).implicitWidth, result);
            }
            return result;
        }

        color: qgcPal.window
        radius: ScreenTools.defaultFontPixelWidth * 1
    }
}
