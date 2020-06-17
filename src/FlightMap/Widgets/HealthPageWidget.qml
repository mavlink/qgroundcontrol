/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

/// Health page for Instrument Panel PageWidget
Column {
    width: pageWidth

    property bool showSettingsIcon: false

    property var _unhealthySensors: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.unhealthySensors : [ ]

    QGCLabel {
        width:                  parent.width
        horizontalAlignment:    Text.AlignHCenter
        text:                   qsTr("All systems healthy")
        visible:                healthRepeater.count == 0
    }

    Repeater {
        id:     healthRepeater
        model:  _unhealthySensors

        Row {
            Image {
                source:             "/qmlimages/Yield.svg"
                height:             ScreenTools.defaultFontPixelHeight
                sourceSize.height:  height
                fillMode:           Image.PreserveAspectFit
            }

            QGCLabel {
                text:   modelData
            }
        }
    }
}
