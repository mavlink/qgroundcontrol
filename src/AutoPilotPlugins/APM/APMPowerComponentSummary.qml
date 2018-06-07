/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
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

    property bool _batt2MonitorAvailable:   controller.parameterExists(-1, "BATT2_MONITOR")
    property bool _batt2CapacityAvailable:  controller.parameterExists(-1, "BATT2_CAPACITY")

    property Fact _battCapacity:            controller.getParameterFact(-1, "BATT_CAPACITY")
    property Fact _batt2Capacity:           controller.getParameterFact(-1, "BATT2_CAPACITY", false /* reportMissing */)
    property Fact _battMonitor:             controller.getParameterFact(-1, "BATT_MONITOR")
    property Fact _batt2Monitor:            controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Battery monitor")
            valueText: _battMonitor.enumStringValue
        }

        VehicleSummaryRow {
            labelText: qsTr("Battery capacity")
            valueText: _battCapacity.valueString + " " + _battCapacity.units
        }

        VehicleSummaryRow {
            labelText:  qsTr("Battery2 monitor")
            valueText:  _batt2MonitorAvailable ? _batt2Monitor.enumStringValue : ""
            visible:    _batt2MonitorAvailable
        }

        VehicleSummaryRow {
            labelText:  qsTr("Battery2 capacity")
            valueText:  _batt2CapacityAvailable ? _batt2Capacity.valueString + " " + _battCapacity.units : ""
            visible:    _batt2CapacityAvailable
        }
    }
}
