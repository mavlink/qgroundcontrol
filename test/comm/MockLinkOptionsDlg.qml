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
import QtQuick.Controls
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controllers

QGCPopupDialog {
    title:      qsTr("MockLink Options")
    buttons:    Dialog.Close

    property var link

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight / 2

        QGCButton {
            Layout.fillWidth:   true
            text:               qsTr("Stop Heartbeats")
            onClicked: {
                link.setCommLost(true)
                close()
            }
        }

        QGCButton {
            Layout.fillWidth:   true
            text:               qsTr("Start Heartbeats")
            onClicked: {
                link.setCommLost(false)
                close()
            }
        }

        QGCButton {
            Layout.fillWidth:   true
            text:               qsTr("Connection Removed")
            onClicked: {
                link.simulateConnectionRemoved()
                close()
            }
        }
    }
}
