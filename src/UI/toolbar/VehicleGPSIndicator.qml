/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

GPSIndicator {
    property bool showIndicator: _activeVehicle.gps.telemetryAvailable

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
}
