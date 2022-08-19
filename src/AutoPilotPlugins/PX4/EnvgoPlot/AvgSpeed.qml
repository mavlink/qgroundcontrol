/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

ColumnLayout {
    width: parent.width
    anchors.fill: parent
    
    PIDTuning {
        width: parent.width
        id:    pidTuning

        property var roll: QtObject {
            id: my_graph
            property string name: qsTr("")
            property var plot: [
                { name: "Average Speed", value: globals.activeVehicle.groundSpeed.value } // here! - currently graphing ground speed
            ]
        }

        title: ""
        tuningMode: Vehicle.ModeRateAndAttitude
        unit: "km/h"
        axis: [ roll ]
        chartDisplaySec: 3
        showAutoModeChange: true
        showAutoTuning:     true
    }
}

