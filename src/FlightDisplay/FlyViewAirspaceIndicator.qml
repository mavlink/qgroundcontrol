/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.12
import QtQuick.Controls         2.4

import QGroundControl               1.0
import QGroundControl.Airspace      1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    id:             _root
    width:          airspaceRow.width + (ScreenTools.defaultFontPixelWidth * 3)
    height:         airspaceRow.height * 1.25
    color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
    visible:        show && QGroundControl.airmapSupported && _flightPermit && _flightPermit !== AirspaceFlightPlanProvider.PermitNone
    radius:         3
    border.width:   1
    border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)

    property bool show: false

    property var    _flightPermit: QGroundControl.airmapSupported ? QGroundControl.airspaceManager.flightPlan.flightPermitStatus : null
    property string _providerName: QGroundControl.airspaceManager.providerName

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Row {
        id:                 airspaceRow
        spacing:            ScreenTools.defaultFontPixelWidth
        anchors.centerIn:   parent

        QGCLabel { text: _providerName+":"; anchors.verticalCenter: parent.verticalCenter; }
        QGCLabel {
            text: {
                if(_flightPermit) {
                    if(_flightPermit === AirspaceFlightPlanProvider.PermitPending)
                        return qsTr("Approval Pending")
                    if(_flightPermit === AirspaceFlightPlanProvider.PermitAccepted || _flightPermit === AirspaceFlightPlanProvider.PermitNotRequired)
                        return qsTr("Flight Approved")
                    if(_flightPermit === AirspaceFlightPlanProvider.PermitRejected)
                        return qsTr("Flight Rejected")
                }
                return ""
            }
            color: {
                if(_flightPermit) {
                    if(_flightPermit === AirspaceFlightPlanProvider.PermitPending)
                        return qgcPal.colorOrange
                    if(_flightPermit === AirspaceFlightPlanProvider.PermitAccepted || _flightPermit === AirspaceFlightPlanProvider.PermitNotRequired)
                        return qgcPal.colorGreen
                }
                return qgcPal.colorRed
            }
            anchors.verticalCenter: parent.verticalCenter;
        }
    }
}
