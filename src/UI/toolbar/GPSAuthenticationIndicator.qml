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
import QGroundControl.ScreenTools
import QGroundControl.Palette

//-------------------------------------------------------------------------
//-- GPS Authentication Indicator
Item {
    id:             control
    width:          height
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: _activeVehicle.gps.authenticationState.value > 0

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    function authenticationIconColor() {
        if (_activeVehicle.gps.authenticationState.value === 0) {
            return qgcPal.colorGrey
        } else if (_activeVehicle.gps.authenticationState.value === 1) {
            return qgcPal.colorYellow
        } else if (_activeVehicle.gps.authenticationState.value === 2) {
            return qgcPal.colorRed
        } else if (_activeVehicle.gps.authenticationState.value === 3) {
            return qgcPal.colorGreen
        } else if (_activeVehicle.gps.authenticationState.value === 4) {
            return qgcPal.colorGrey
        }
    }

    function getAuthenticationText(){
        if (_activeVehicle.gps.authenticationState.value === 0) {
            return qsTr("Unkown")
        } else if (_activeVehicle.gps.authenticationState.value === 1) {
            return qsTr("Initializing...")
        } else if (_activeVehicle.gps.authenticationState.value === 2) {
            return qsTr("Failed")
        } else if (_activeVehicle.gps.authenticationState.value === 3) {
            return qsTr("OK")
        } else if (_activeVehicle.gps.authenticationState.value === 4) {
            return qsTr("Disabled")
        }
        return qsTr("n/a")
    }

    QGCColoredImage {
        id:                 gpsAuthenticationIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        source:             "/qmlimages/GpsAuthentication.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        opacity:            1
        color:              authenticationIconColor()
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(authenticationContentComponent, control)
    }

    Component{
        id: authenticationContentComponent

        ColumnLayout{
            spacing: ScreenTools.defaultFontPixelHeight / 2

            property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

            SettingsGroupLayout {
                heading: qsTr("GPS Authentication")
                contentSpacing: 0
                showDividers: false

                LabelledLabel {
                    label: qsTr("Status")
                    labelText: getAuthenticationText()
                }

            }
        }
    }
}
