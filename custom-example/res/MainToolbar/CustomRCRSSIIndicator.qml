/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- RC RSSI Indicator
Item {
    id:             _root
    width:          rssiRow.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    visible:        activeVehicle ? activeVehicle.supportsRadio : true

    property bool   _rcRSSIAvailable:   activeVehicle ? (activeVehicle.rcRSSI > 0 && activeVehicle.rcRSSI <= 100) : false
    property bool   _mhRSSIAvailable:   activeVehicle ? QGroundControl.microhardManager.downlinkRSSI < 0 : false

    Component {
        id: rcRSSIInfo

        Rectangle {
            width:  rcrssiCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: rcrssiCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 rcrssiCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             rssiLabel
                    text:           qsTr("RSSI Status")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                QGCLabel {
                    visible:        !rcrssiGrid.visible
                    text:           activeVehicle ? qsTr("RSSI Data Unavailable") : qsTr("N/A", "No data available")
                }

                GridLayout {
                    id:                 rcrssiGrid
                    visible:            _rcRSSIAvailable || _mhRSSIAvailable
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    columns:            2
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel { text: qsTr("RC RSSI:"); visible: _rcRSSIAvailable; }
                    QGCLabel { text: activeVehicle ? (activeVehicle.rcRSSI + "%") : 0; visible: _rcRSSIAvailable; }
                    QGCLabel { text: qsTr("Uplink RSSI:"); visible: QGroundControl.microhardManager.uplinkRSSI < 0; }
                    QGCLabel { text: activeVehicle ? (QGroundControl.microhardManager.uplinkRSSI + "dBm") : 0; visible: QGroundControl.microhardManager.uplinkRSSI < 0; }
                    QGCLabel { text: qsTr("Downlink RSSI:"); visible: _mhRSSIAvailable; }
                    QGCLabel { text: activeVehicle ? (QGroundControl.microhardManager.downlinkRSSI + "dBm") : 0; visible: _mhRSSIAvailable; }
                }
            }
        }
    }

    Row {
        id:             rssiRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth

        QGCColoredImage {
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            sourceSize.height:  height
            source:             "/qmlimages/RC.svg"
            fillMode:           Image.PreserveAspectFit
            opacity:            _rcRSSIAvailable ? 1 : 0.5
            color:              qgcPal.buttonText
        }

        SignalStrength {
            anchors.verticalCenter: parent.verticalCenter
            size:                   parent.height * 0.5
            percent:                _rcRSSIAvailable ? activeVehicle.rcRSSI : (_mhRSSIAvailable ? QGroundControl.microhardManager.downlinkRSSIPct : 0)
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked: {
            mainWindow.showPopUp(_root, rcRSSIInfo)
        }
    }
}
