/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Battery, propeller and magnetometer summary
///     @author Gus Grubba <mavlink@grubba.com>

import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property Fact batVChargedFact:  controller.getParameterFact(-1, "BAT_V_CHARGED")
    property Fact batVEmptyFact:    controller.getParameterFact(-1, "BAT_V_EMPTY")
    property Fact batCellsFact:     controller.getParameterFact(-1, "BAT_N_CELLS")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Battery Full:")
            valueText: batVChargedFact ? batVChargedFact.valueString + " " + batVChargedFact.units : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Battery Empty:")
            valueText: batVEmptyFact ? batVEmptyFact.valueString + " " + batVEmptyFact.units : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Number of Cells:")
            valueText: batCellsFact ? batCellsFact.valueString : ""
        }
    }
}
