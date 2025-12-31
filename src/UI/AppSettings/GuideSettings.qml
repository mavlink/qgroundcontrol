import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    color:          qgcPal.toolbarBackground
    anchors.fill:   parent

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCFlickable {
        anchors.margins:    0
        anchors.fill:       parent
        contentWidth:       parent.width
        contentHeight:      grid.height
        clip:               true

        GridLayout {
            id:         grid
            width:      parent.width
            columns:    2
            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
            rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.5

            QGCLabel { text: qsTr("Application"); color: qgcPal.text }
            QGCLabel { text: qsTr("IG GCS FLY"); color: qgcPal.text }

            QGCLabel { text: qsTr("Version"); color: qgcPal.text }
            QGCLabel { text: qsTr("v2.0.2 Release"); color: qgcPal.text }

            QGCLabel { text: qsTr("Organization"); color: qgcPal.text }
            QGCLabel { text: qsTr("IG Drones"); color: qgcPal.text }
        }
    }
}
