/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Battery, propeller and magnetometer summary
///     @author Gus Grubba <gus@auterion.com>

import QtQuick
import QtQuick.Controls

import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.Palette

Item {
    anchors.fill:   parent

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; }

    property Fact batVChargedFact:  controller.getParameterFact(-1, "BAT1_V_CHARGED")
    property Fact batVEmptyFact:    controller.getParameterFact(-1, "BAT1_V_EMPTY")
    property Fact batCellsFact:     controller.getParameterFact(-1, "BAT1_N_CELLS")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Battery Full")
            valueText: batVChargedFact ? batVChargedFact.valueString + " " + batVChargedFact.units : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Battery Empty")
            valueText: batVEmptyFact ? batVEmptyFact.valueString + " " + batVEmptyFact.units : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Number of Cells")
            valueText: batCellsFact ? batCellsFact.valueString : ""
        }
    }
}
