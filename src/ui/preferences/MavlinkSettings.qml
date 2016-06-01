/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.1

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:             __mavlinkRoot
    color:          qgcPal.window
    anchors.fill:   parent

    QGCPalette { id: qgcPal }

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        contentHeight:      settingsColumn.height
        contentWidth:       settingsColumn.width

        Column {
            id:                 settingsColumn
            spacing:            ScreenTools.defaultFontPixelHeight
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            anchors.left:       parent.left
            anchors.top:        parent.top
            QGCLabel {
                text:   qsTr("MavLink Settings")
                font.pointSize: ScreenTools.mediumFontPointSize
            }
            Rectangle {
                height: 1
                width:  parent.width
                color:  qgcPal.button
            }
            //-----------------------------------------------------------------
            //-- System ID
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:   qsTr("Ground Station MavLink System ID:")
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCTextField {
                    id:     sysidField
                    text:   QGroundControl.mavlinkSystemID.toString()
                    width:  ScreenTools.defaultFontPixelWidth * 6
                    inputMethodHints:       Qt.ImhFormattedNumbersOnly
                    anchors.verticalCenter: parent.verticalCenter
                    onEditingFinished: {
                        QGroundControl.mavlinkSystemID = parseInt(sysidField.text)
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Mavlink Heartbeats
            QGCCheckBox {
                text:       qsTr("Emit heartbeat")
                checked:    QGroundControl.multiVehicleManager.gcsHeartBeatEnabled
                onClicked: {
                    QGroundControl.multiVehicleManager.gcsHeartBeatEnabled = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Mavlink Multiplexing
            QGCCheckBox {
                text:       qsTr("Enable multiplexing (forward packets to all other links)")
                checked:    QGroundControl.isMultiplexingEnabled
                onClicked: {
                    QGroundControl.isMultiplexingEnabled = checked
                }
            }
            //-----------------------------------------------------------------
            //-- Mavlink Version Check
            QGCCheckBox {
                text:       qsTr("Only accept MAVs with same protocol version")
                checked:    QGroundControl.isVersionCheckEnabled
                onClicked: {
                    QGroundControl.isVersionCheckEnabled = checked
                }
            }
        }
    }
}
