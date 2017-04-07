/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Switch assignment summary
///     @author Gus Grubba <mavlink@grubba.com>

import QtQuick 2.3
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
    function getModeString() {
        var mode = controller.getParameterFact(-1, "COM_FLTMODE6").value
        if(mode < 12) {
            return controller.getParameterFact(-1, "COM_FLTMODE6").enumStringValue
        } else if(mode === 13) {
            return 'Smart'
        }
        return 'Unknown'
    }
    Column {
        anchors.fill:       parent
        VehicleSummaryRow {
            labelText: qsTr("Custom Switch Mode:")
            valueText: getModeString()
        }
    }
}
