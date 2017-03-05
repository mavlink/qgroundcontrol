/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    color:          qgcPal.window
    anchors.fill:   parent

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCFlickable {
        anchors.fill:   parent
        contentWidth:   column.width + (_margins * 2)
        contentHeight:  column.height + (_margins * 2)
        clip:           true

        Column {
            id:                 column
            anchors.margins:    _margins
            anchors.left:       parent.left
            anchors.top:        parent.top
            spacing:            ScreenTools.defaultFontPixelHeight

            QGCButton {
                text:       qsTr("PX4 Vehicle")
                onClicked:  QGroundControl.startPX4MockLink(sendStatusText.checked)
            }
            QGCButton {
                text:       qsTr("APM ArduCopter Vehicle")
                onClicked:  QGroundControl.startAPMArduCopterMockLink(sendStatusText.checked)
            }
            QGCButton {
                text:       qsTr("APM ArduPlane Vehicle")
                onClicked:  QGroundControl.startAPMArduPlaneMockLink(sendStatusText.checked)
            }
            QGCButton {
                text:       qsTr("APM ArduSub Vehicle")
                onClicked:  QGroundControl.startAPMArduSubMockLink(sendStatusText.checked)
            }
            QGCButton {
                text:       qsTr("Generic Vehicle")
                onClicked:  QGroundControl.startGenericMockLink(sendStatusText.checked)
            }
            QGCCheckBox {
                id:     sendStatusText
                text:   qsTr("Send status text + voice")
            }
            QGCButton {
                text:       qsTr("Stop All MockLinks")
                onClicked:  QGroundControl.stopAllMockLinks()
            }
        }
    }
}
