/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

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
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: qsTr("Battery Full:")
            valueText: batVChargedFact ? batVChargedFact.valueString : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Battery Empty:")
            valueText: batVEmptyFact ? batVEmptyFact.valueString : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Number of Cells:")
            valueText: batCellsFact ? batCellsFact.valueString : ""
        }
    }
}
