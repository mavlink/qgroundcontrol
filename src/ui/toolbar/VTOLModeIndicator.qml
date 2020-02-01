/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

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

    onActivated: {
        if (index == 0) {
            if (_fwdFlight) {
                mainWindow.vtolTransitionToMRFlight()
            }
        } else {
            if (!_fwdFlight) {
                mainWindow.vtolTransitionToFwdFlight()
            }
        }
        currentIndex = -1
    }
}
