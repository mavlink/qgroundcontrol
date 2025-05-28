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
import QGroundControl.Palette
import QGroundControl.ScreenTools

Rectangle {
    color:          qgcPal.window
    anchors.fill:   parent

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

            QGCLabel { text: qsTr("QGroundControl User Guide") }
            QGCLabel {
                linkColor:          qgcPal.text
                text:               "<a href=\"https://docs.qgroundcontrol.com\">https://docs.qgroundcontrol.com</a>"
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

            QGCLabel { text: qsTr("QGroundControl Discord Channel") }
            QGCLabel {
                linkColor:          qgcPal.text
                text:               "<a href=\"https://discord.com/channels/1022170275984457759/1022185820683255908\">https://discord.com/channels/1022170275984457759/1022185820683255908</a>"
                onLinkActivated:    (link) => Qt.openUrlExternally(link)
            }
        }
    }
}
