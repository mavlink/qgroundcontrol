/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
///     @brief Output summary
///     @author Gus Grubba <gus@auterion.com>

import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

Item {
    anchors.fill:   parent

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; }

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Foo")
            valueText: "Ok"
        }

        VehicleSummaryRow {
            labelText: qsTr("Bar")
            valueText: "Not Ok"
        }

    }
}
