/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls



Rectangle {
    color:          qgcPal.toolbarBackground
    anchors.fill:   parent

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

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
            QGCLabel { text: qsTr("Application") }
            QGCLabel { text: qsTr("IG GCS FLY") }
            QGCLabel { text: qsTr("Version") }
            QGCLabel { text: QGroundControl.qgcVersion.replace(/^v[^ ]+/, "v2.0.3") }
            QGCLabel { text: qsTr("Organization") }
            QGCLabel { text: qsTr("IG Drones") }
            QGCLabel { text: qsTr("Website") }
            QGCLabel {
                linkColor:          qgcPal.text
                text:               "<a href=\"https://igdrones.com\">https://igdrones.com</a>"
                onLinkActivated:    (link) => Qt.openUrlExternally(link)
            }

            QGCLabel { text: qsTr("IG GCS FLY Website") }
            QGCLabel {
                linkColor:          qgcPal.text
                text:               "<a href=\"https://igdrones.com\">https://igdrones.com</a>"
                onLinkActivated:    (link) => Qt.openUrlExternally(link)
            }

            QGCLabel { text: qsTr("PX4 Users Discussion Forum") }
            QGCLabel {
                linkColor:          qgcPal.text
                text:               "<a href=\"http://discuss.px4.io/c/qgroundcontrol\">http://discuss.px4.io/c/qgroundcontrol</a>"
                onLinkActivated:    (link) => Qt.openUrlExternally(link)
            }

            QGCLabel { text: qsTr("ArduPilot Users Discussion Forum") }
            QGCLabel {
                linkColor:          qgcPal.text
                text:               "<a href=\"https://discuss.ardupilot.org/c/ground-control-software/qgroundcontrol\">https://discuss.ardupilot.org/c/ground-control-software/qgroundcontrol</a>"
                onLinkActivated:    (link) => Qt.openUrlExternally(link)
            }

            
        }
    }
}
