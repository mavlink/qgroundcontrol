/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.ScreenTools   1.0

Item {
    width:                            availableWidth
    property bool _autotuningEnabled: true // used to restore setting when switching between tabs

    FactPanelController {
        id:         controller
    }

    Loader {
        id:                loader
        source:            pages[bar.currentIndex]
        width:             parent.width
        anchors.fill:      parent
        onLoaded: {
            if (typeof loader.item.autotuningEnabled !== "undefined") {
                loader.item.autotuningEnabled = _autotuningEnabled;
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        width: parent.width
        height: bar.height
        y: parent.y + parent.height - bar.height - 38
        color: "white"
        z: 1
    }

    ComboBox {
        id:             bar

        width:          450
        x:              parent.width/2 - 190
        y:              parent.y + parent.height - bar.height - 38
        z: 2
        model: ListModel {
            ListElement {
                text: "Average speed"
            }
            ListElement {
                text: "Distance traveled"
            }
            ListElement {
                text: "Remaining battery"
            }
            ListElement {
                text: "Height above water"
            }
            ListElement {
                text: "Temperature"
            }
            ListElement {
                text: "Motor temperature"
            }
            ListElement {
                text: "Motor controller temperature"
            }
            ListElement {
                text: "Battery temperature"
            }
            ListElement {
                text: "Servo temperature"
            }
        }
        onActivated: {
            if (typeof loader.item.autotuningEnabled !== "undefined") {
                _autotuningEnabled = loader.item.autotuningEnabled;
            }
        }
    }

    property var pages:  [
        "AvgSpeed.qml",
        "DistanceTraveled.qml",
        "RemainingBattery.qml",
        "HeightAboveWater.qml",
        "Temperature.qml",
        "MotorTemperature.qml",
        "MotorControllerTemperature.qml",
        "BatteryTemperature.qml",
        "ServoTemperature.qml"
    ]
}
