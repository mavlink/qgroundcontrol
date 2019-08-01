// QtQuick.Control 1.x Menu

import QtQuick          2.11
import QtQuick.Controls 2.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Menu {

    id: _root

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    background: Rectangle {

        implicitWidth: {
                var result = 0;
                var padding = 0;
                for (var i = 0; i < count; ++i) {
                    var item = itemAt(i);
                    result = Math.max(item.contentItem.implicitWidth, result);
                    padding = Math.max(item.padding, padding);
                }
                return result + padding * 2;
            }

        implicitHeight: 40
        color: qgcPal.window
        border.color: qgcPal.windowShadeDark
        radius: 2
    }
}
