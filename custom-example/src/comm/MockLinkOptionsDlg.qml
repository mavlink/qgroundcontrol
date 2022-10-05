/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Layouts  1.2
import QtQuick.Controls 2.5
import QtQuick.Dialogs  1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0

QGCPopupDialog {
    title:      qsTr("MockLink Options")
    buttons:    StandardButton.Close

    property var link: dialogProperties.link

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight / 2

        QGCButton {
            Layout.fillWidth:   true
            text:               qsTr("Stop Heartbeats")
            onClicked: {
                link.setCommLost(true)
                reject()
            }
        }

        QGCButton {
            Layout.fillWidth:   true
            text:               qsTr("Start Heartbeats")
            onClicked: {
                link.setCommLost(false)
                reject()
            }
        }

        QGCButton {
            Layout.fillWidth:   true
            text:               qsTr("Connection Removed")
            onClicked: {
                link.simulateConnectionRemoved()
                reject()
            }
        }
    }
}
