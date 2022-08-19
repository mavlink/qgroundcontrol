/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3
import CustomPlot 1.0

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0

import QGroundControl.EnvgoPlot 1.0

Item {
    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    id: plot_view_root

    EnvgoPlotClass {
        id: my_plot
        anchors.fill: parent

        Component.onCompleted: init()

        Timer {
            interval:   1000  // update graph once per sec
            running:    true
            repeat:     true
            onTriggered: {
                if (_activeVehicle) {
                    // process data here
                    // add_data(data)
                }
            }
        }
    }
}
