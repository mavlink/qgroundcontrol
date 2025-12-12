/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    property bool   showIndicator: _activeVehicle && _activeVehicle.gps.authenticationState.value > 0

    function authenticationIconColor() {
        if(!_activeVehicle){
            return qgcPal.colorGrey   // Not connected
        }

        switch (_activeVehicle.gps.authenticationState.value) {
        case 1: // Initializing
            return qgcPal.colorYellow;
        case 2: // Error
            return qgcPal.colorRed;
        case 3: // OK
            return qgcPal.colorGreen;
        case 0: // Unknown
        case 4: // Disabled
        default:
            return qgcPal.colorGrey;
        }
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

            SettingsGroupLayout {
                heading: qsTr("GPS Authentication")
                contentSpacing: 0
                showDividers: false

                LabelledLabel {
                    label: qsTr("Status")
                    labelText:  _activeVehicle ? (_activeVehicle.gps.authenticationState.valueString || qsTr("n/a")) : qsTr("n/a")
                }
            }
        }
    }
}
