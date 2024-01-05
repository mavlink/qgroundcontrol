/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.Controllers
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.ScreenTools

QGCPopupDialog {
    id:         root
    title:      qsTr("Sending plan to vehicle")
    buttons:    undefined

    property var planMasterController

    property real _margin: ScreenTools.defaultFontPixelWidth

    // Progress bar
    Connections {
        target: planMasterController.missionController

        onProgressPctChanged: (progressPct) => {
            if (progressPct === 1) {
                delayedCloseTimer.start()
                doneLabel.visible = true
            }
        }
    }

    Timer {
        id:         delayedCloseTimer
        interval:   5000
        onTriggered: {
            root.close()
            mainWindow.popView()
        }
    }

    Rectangle {
        width:          ScreenTools.defaultFontPixelWidth * 30
        height:         ScreenTools.defaultFontPixelHeight * 3
        border.width:   1
        color:          "green"

        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          parent.width * planMasterController.missionController.progressPct
            color:          "green"
        }

        QGCLabel {
            id:                 doneLabel
            anchors.centerIn:   parent
            text:               qsTr("Done")
            color:              "white"
            visible:            false
        }
    }
}
