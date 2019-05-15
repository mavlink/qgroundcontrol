/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
QGCLabel {
    anchors.top:        parent.top
    anchors.bottom:     parent.bottom
    verticalAlignment:  Text.AlignVCenter
    text:               _fwdFlight ? qsTr("VTOL: Fixed Wing") : qsTr("VTOL: Multi-Rotor")
    font.pointSize:     ScreenTools.mediumFontPointSize
    color:              qgcPal.buttonText
    visible:            activeVehicle ? activeVehicle.vtol && activeVehicle.px4Firmware : false
    width:              visible ? implicitWidth : 0

    property bool _fwdFlight: activeVehicle ? activeVehicle.vtolInFwdFlight : false

    QGCMouseArea {
        fillItem: parent
        onClicked: activeVehicle.vtolInFwdFlight ? toolBar.vtolTransitionToMRFlight() : toolBar.vtolTransitionToFwdFlight()
    }
}
