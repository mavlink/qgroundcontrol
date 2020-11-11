/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Layouts  1.11

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0



Rectangle {
    color:          qgcPal.window
    anchors.fill:   parent

    Rectangle {
    color:          qgcPal.window
    width: 100
    height: 100
    anchors { horizontalCenter: parent.horizontalCenter; right: parent.right; rightMargin: 200; top: parent.top; topMargin: 300 }
            Image {
                width: 1316; height: 300
                source: "/qmlimages/intelpro_logo.png";

            }
    }

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCFlickable {
        anchors.margins:    _margins
        anchors.fill:       parent
        contentWidth:       grid.width
        contentHeight:      grid.height
        clip:               true

        GridLayout {
            id:         grid
            columns:    2

            QGCLabel { text: qsTr("GCS created by IntelPRO LLC") }
            QGCLabel {
                linkColor:          qgcPal.text
                text:               "<a href=\"http://intelpro.az\">https://intelpro.az</a>"
                onLinkActivated:    Qt.openUrlExternally(link)
            }
        }
    }
}
