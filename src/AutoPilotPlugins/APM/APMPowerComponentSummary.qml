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

import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.Palette

Item {
    anchors.fill:   parent

    FactPanelController { id: controller; }

    property Fact _batt1Monitor:            controller.getParameterFact(-1, "BATT_MONITOR")
    property Fact _batt2Monitor:            controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
    property bool _batt2MonitorAvailable:   controller.parameterExists(-1, "BATT2_MONITOR")
    property bool _batt1MonitorEnabled:     _batt1Monitor.rawValue !== 0
    property bool _batt2MonitorEnabled:     _batt2MonitorAvailable && _batt2Monitor.rawValue !== 0
    property Fact _battCapacity:            controller.getParameterFact(-1, "BATT_CAPACITY", false /* reportMissing */)
    property Fact _batt2Capacity:           controller.getParameterFact(-1, "BATT2_CAPACITY", false /* reportMissing */)
    property bool _battCapacityAvailable:   controller.parameterExists(-1, "BATT_CAPACITY")

    function translatedMonitorState(fact) {
        if (!fact) {
            return ""
        }
        var enumText = fact.enumStringValue.toString().trim()
        if (enumText === "Disabled") {
            return qsTr("Disabled")
        }
        if (enumText === "Enabled") {
            return qsTr("Enabled")
        }
        return enumText
    }

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Batt1 monitor")
            valueText: translatedMonitorState(_batt1Monitor)
        }

        VehicleSummaryRow {
            labelText: qsTr("Batt1 capacity")
            valueText:  _batt1MonitorEnabled ? _battCapacity.valueString + " " + _battCapacity.units : ""
            visible:    _batt1MonitorEnabled
        }

        VehicleSummaryRow {
            labelText:  qsTr("Batt2 monitor")
            valueText:  _batt2MonitorAvailable ? translatedMonitorState(_batt2Monitor) : ""
            visible:    _batt2MonitorAvailable
        }

        VehicleSummaryRow {
            labelText:  qsTr("Batt2 capacity")
            valueText:  _batt2MonitorEnabled ? _batt2Capacity.valueString + " " + _batt2Capacity.units : ""
            visible:    _batt2MonitorEnabled
        }
    }
}
