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

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.ScreenTools

Item {
    width: availableWidth

    FactPanelController {
        id:         controller
    }

    QGCTabBar {
        id:             bar
        width:          parent.width
        anchors.top:    parent.top
        QGCTabButton {
            text:       qsTr("Rate Controller")
        }
        QGCTabButton {
            text:       qsTr("Attitude Controller")
        }
    }

    property var pages:  [
        "PX4TuningComponentPlaneRate.qml",
        "PX4TuningComponentPlaneAttitude.qml",
    ]

    Loader {
        source:            pages[bar.currentIndex]
        width:             parent.width
        anchors.top:       bar.bottom
        anchors.topMargin: ScreenTools.defaultFontPixelWidth
        anchors.bottom:    parent.bottom
    }
}
