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
//-- Armed Indicator
QGCLabel {
    anchors.top:        parent.top
    anchors.bottom:     parent.bottom
    verticalAlignment:  Text.AlignVCenter
    text:               _armed ? qsTr("Armed") : qsTr("Disarmed")
    font.pointSize:     ScreenTools.mediumFontPointSize
    color:              qgcPal.buttonText

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool   _armed:         _activeVehicle ? _activeVehicle.armed : false

    QGCPalette { id: qgcPal }

    QGCMouseArea {
        fillItem: parent
        onClicked: _armed ? toolBar.disarmVehicle() : toolBar.armVehicle()
    }
}
