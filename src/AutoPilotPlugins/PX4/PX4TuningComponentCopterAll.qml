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
    width:                            availableWidth
    property bool _autotuningEnabled: true // used to restore setting when switching between tabs

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
        QGCTabButton {
            text:       qsTr("Velocity Controller")
        }
        QGCTabButton {
            text:       qsTr("Position Controller")
        }
        onCurrentIndexChanged: {
            if (typeof loader.item.autotuningEnabled !== "undefined") {
                _autotuningEnabled = loader.item.autotuningEnabled;
            }
        }
    }

    property var pages:  [
        "PX4TuningComponentCopterRate.qml",
        "PX4TuningComponentCopterAttitude.qml",
        "PX4TuningComponentCopterVelocity.qml",
        "PX4TuningComponentCopterPosition.qml"
    ]

    Loader {
        id:                loader
        source:            pages[bar.currentIndex]
        width:             parent.width
        anchors.top:       bar.bottom
        anchors.topMargin: ScreenTools.defaultFontPixelWidth
        anchors.bottom:    parent.bottom
        onLoaded: {
            if (typeof loader.item.autotuningEnabled !== "undefined") {
                loader.item.autotuningEnabled = _autotuningEnabled;
            }
        }
    }
}
