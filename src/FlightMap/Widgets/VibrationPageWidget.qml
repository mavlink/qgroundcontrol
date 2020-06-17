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

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl               1.0

Rectangle {
    height: barRow.y + barRow.height
    width:  pageWidth
    color:  qgcPal.window

    property bool showSettingsIcon: false

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property bool   _available:     _activeVehicle ? !isNaN(_activeVehicle.vibration.xAxis.value) : false
    property real   _margins:       ScreenTools.defaultFontPixelWidth / 2
    property real   _barWidth:      Math.round(ScreenTools.defaultFontPixelWidth * 3)

    readonly property real _barMinimum:     0.0
    readonly property real _barMaximum:     90.0
    readonly property real _barBadValue:    60.0

    QGCPalette { id:qgcPal; colorGroupEnabled: true }

    QGCLabel {
        id:     title
        text:   qsTr("Vibe")
        anchors.horizontalCenter: barRow.horizontalCenter
    }

    Row {
        id:                 barRow
        anchors.margins:    _margins
        anchors.top:        title.bottom
        anchors.left:       parent.left
        spacing:            _margins

        Column {
            ProgressBar {
                id:             xBar
                height:         50
                orientation:    Qt.Vertical
                minimumValue:   _barMinimum
                maximumValue:   _barMaximum
                value:          _activeVehicle ? _activeVehicle.vibration.xAxis.value : 0
            }

            QGCLabel {
                id:     xBarLabel
                text:   "X"
                anchors.horizontalCenter: xBar.horizontalCenter
            }
        }

        Column {
            ProgressBar {
                id:             yBar
                height:         50
                orientation:    Qt.Vertical
                minimumValue:   _barMinimum
                maximumValue:   _barMaximum
                value:          _activeVehicle ? _activeVehicle.vibration.yAxis.value : 0
            }

            QGCLabel {
                anchors.horizontalCenter: yBar.horizontalCenter
                text:   "Y"
            }
        }

        Column {
            ProgressBar {
                id:             zBar
                height:         50
                orientation:    Qt.Vertical
                minimumValue:   _barMinimum
                maximumValue:   _barMaximum
                value:          _activeVehicle ? _activeVehicle.vibration.zAxis.value : 0
            }

            QGCLabel {
                anchors.horizontalCenter: zBar.horizontalCenter
                text:   "Z"
            }
        }
    } // Row

    // Max vibe indication line at 60
    Rectangle {
        anchors.topMargin:      xBar.height * (1.0 - ((_barBadValue - _barMinimum) / (_barMaximum - _barMinimum)))
        anchors.top:            barRow.top
        anchors.left:           barRow.left
        anchors.right:          barRow.right
        width:                  barRow.width
        height:                 1
        color:                  "red"
    }

    QGCLabel {
        id:                 clipLabel
        anchors.margins:    _margins
        anchors.left:       barRow.right
        anchors.right:      parent.right
        text:               qsTr("Clip count")
        horizontalAlignment: Text.AlignHCenter
    }

    Column {
        id:             clipColumn
        anchors.top:    barRow.top
        anchors.horizontalCenter: clipLabel.horizontalCenter

        QGCLabel {
            text: qsTr("Accel 1: ") + (_activeVehicle ? _activeVehicle.vibration.clipCount1.valueString : "")
        }

        QGCLabel {
            text: qsTr("Accel 2: ") + (_activeVehicle ? _activeVehicle.vibration.clipCount2.valueString : "")
        }

        QGCLabel {
            text: qsTr("Accel 3: ") + (_activeVehicle ? _activeVehicle.vibration.clipCount3.valueString : "")
        }
    }

    // Not available overlay
    Rectangle {
        anchors.fill:   parent
        color:          qgcPal.window
        opacity:        0.75
        visible:        !_available

        QGCLabel {
            anchors.fill:           parent
            horizontalAlignment:    Text.AlignHCenter
            verticalAlignment:      Text.AlignVCenter
            text:                   qsTr("Not Available")
        }
    }
} // Item
