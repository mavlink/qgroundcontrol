/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    property var    unhealthySensors:   QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.unhealthySensors : [ ]

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    Column {
        anchors.fill:       parent
        Repeater {
            id:     healthRepeater
            model:  unhealthySensors
            VehicleSummaryRow {
                labelText: modelData
                valueText: qsTr("Not Ready")
            }
        }
    }
}
