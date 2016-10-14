/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property Fact battCapacity: controller.getParameterFact(-1, "BATT_CAPACITY")
    property Fact battMonitor:  controller.getParameterFact(-1, "BATT_MONITOR")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Battery monitor:")
            valueText: battMonitor.enumStringValue
        }

        VehicleSummaryRow {
            labelText: qsTr("Battery capacity:")
            valueText: battCapacity.valueString + " " + battCapacity.units
        }
    }
}
