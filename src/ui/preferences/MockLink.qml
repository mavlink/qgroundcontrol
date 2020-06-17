/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.11

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
        contentWidth:   column.width  + (_margins * 2)
        contentHeight:  column.height + (_margins * 2)
        clip:           true

        ColumnLayout {
            id:                 column
            anchors.margins:    _margins
            anchors.left:       parent.left
            anchors.top:        parent.top
            spacing:            ScreenTools.defaultFontPixelHeight

            QGCCheckBox {
                id:             sendStatusText
                text:           qsTr("Send status text + voice")
            }
            QGCButton {
                text:               qsTr("PX4 Vehicle")
                Layout.fillWidth:   true
                onClicked:          QGroundControl.startPX4MockLink(sendStatusText.checked)
            }
            QGCButton {
                text:               qsTr("APM ArduCopter Vehicle")
                visible:            QGroundControl.hasAPMSupport
                Layout.fillWidth:   true
                onClicked:          QGroundControl.startAPMArduCopterMockLink(sendStatusText.checked)
            }
            QGCButton {
                text:               qsTr("APM ArduPlane Vehicle")
                visible:            QGroundControl.hasAPMSupport
                Layout.fillWidth:   true
                onClicked:          QGroundControl.startAPMArduPlaneMockLink(sendStatusText.checked)
            }
            QGCButton {
                text:               qsTr("APM ArduSub Vehicle")
                visible:            QGroundControl.hasAPMSupport
                Layout.fillWidth:   true
                onClicked:          QGroundControl.startAPMArduSubMockLink(sendStatusText.checked)
            }
            QGCButton {
                text:               qsTr("APM ArduRover Vehicle")
                visible:            QGroundControl.hasAPMSupport
                Layout.fillWidth:   true
                onClicked:          QGroundControl.startAPMArduRoverMockLink(sendStatusText.checked)
            }
            QGCButton {
                text:               qsTr("Generic Vehicle")
                Layout.fillWidth:   true
                onClicked:          QGroundControl.startGenericMockLink(sendStatusText.checked)
            }
            QGCButton {
                text:               qsTr("Stop One MockLink")
                Layout.fillWidth:   true
                onClicked:          QGroundControl.stopOneMockLink()
            }
        }
    }
}
