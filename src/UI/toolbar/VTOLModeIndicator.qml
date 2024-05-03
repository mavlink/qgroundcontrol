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
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette

//-------------------------------------------------------------------------
//-- VTOL Mode Indicator
QGCComboBox {
    anchors.verticalCenter: parent.verticalCenter
    alternateText:          _fwdFlight ? qsTr("VTOL: FW") : qsTr("VTOL: MR")
    model:                  [ qsTr("VTOL: Multi-Rotor"), qsTr("VTOL: Fixed Wing")  ]
    font.pointSize:         ScreenTools.mediumFontPointSize
    currentIndex:           -1
    sizeToContents:         true

    property bool showIndicator: _activeVehicle.vtol && _activeVehicle.px4Firmware

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool   _fwdFlight:     _activeVehicle.vtolInFwdFlight

    onActivated: (index) => {
        if (index == 0) {
            if (_fwdFlight) {
                mainWindow.vtolTransitionToMRFlightRequest()
            }
        } else {
            if (!_fwdFlight) {
                mainWindow.vtolTransitionToFwdFlightRequest()
            }
        }
        currentIndex = -1
    }
}
