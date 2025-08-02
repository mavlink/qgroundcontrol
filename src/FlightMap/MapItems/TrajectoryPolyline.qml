/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls

MapPolyline {
    line {
        width: 3
        color: "red"
    }
    z: QGroundControl.zOrderTrajectoryLines
    path: _trajectoryPoints ? _trajectoryPoints.path : []

    property Vehicle _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property TrajectoryPoints _trajectoryPoints: _activeVehicle ? _activeVehicle.trajectoryPoints : null
}
