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

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette

//-------------------------------------------------------------------------
//-- Mode Indicator
QGCComboBox {
    anchors.verticalCenter: parent.verticalCenter
    alternateText:          _activeVehicle ? _activeVehicle.flightMode : ""
    model:                  _flightModes
    font.pointSize:         ScreenTools.mediumFontPointSize
    currentIndex:           -1
    sizeToContents:         true

    property bool showIndicator: true

    property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property var _flightModes:      _activeVehicle ? _activeVehicle.flightModes : [ ]

    onActivated: (index) => {
        _activeVehicle.flightMode = _flightModes[index]
        currentIndex = -1
    }
}
