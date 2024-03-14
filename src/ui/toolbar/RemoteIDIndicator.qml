/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette

//-------------------------------------------------------------------------
//-- Remote ID Indicator
Item {
    id:             control
    width:          remoteIDIcon.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool   showIndicator:      remoteIDManager.available

    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property var    remoteIDManager:    activeVehicle ? activeVehicle.remoteIDManager : null

    property bool   gpsFlag:            activeVehicle && remoteIDManager ? remoteIDManager.gcsGPSGood         : false
    property bool   basicIDFlag:        activeVehicle && remoteIDManager ? remoteIDManager.basicIDGood        : false
    property bool   armFlag:            activeVehicle && remoteIDManager ? remoteIDManager.armStatusGood      : false
    property bool   commsFlag:          activeVehicle && remoteIDManager ? remoteIDManager.commsGood          : false
    property bool   emergencyDeclared:  activeVehicle && remoteIDManager ? remoteIDManager.emergencyDeclared  : false
    property bool   operatorIDFlag:     activeVehicle && remoteIDManager ? remoteIDManager.operatorIDGood     : false
    property int    remoteIDState:      getRemoteIDState()
    
    property int    regionOperation:    QGroundControl.settingsManager.remoteIDSettings.region.value

    enum RIDState {
        HEALTHY,
        WARNING,
        ERROR,
        UNAVAILABLE
    }

    enum RegionOperation {
        FAA,
        EU
    }

    function getRIDIcon() {
        switch (remoteIDState) {
            case RemoteIDIndicator.RIDState.HEALTHY: 
                return "/qmlimages/RidIconGreen.svg"
                break
            case RemoteIDIndicator.RIDState.WARNING: 
                return "/qmlimages/RidIconYellow.svg"
                break
            case RemoteIDIndicator.RIDState.ERROR: 
                return "/qmlimages/RidIconRed.svg"
                break
            case RemoteIDIndicator.RIDState.UNAVAILABLE: 
                return "/qmlimages/RidIconGrey.svg"
                break
            default:
                return "/qmlimages/RidIconGrey.svg"
        }
    }

    function getRemoteIDState() {
        if (!activeVehicle) {
            return RemoteIDIndicator.RIDState.UNAVAILABLE
        }
        // We need to have comms and arm healthy to even be in any other state other than ERROR
        if (!commsFlag || !armFlag || emergencyDeclared) {
            return RemoteIDIndicator.RIDState.ERROR
        }
        if (!gpsFlag || !basicIDFlag) {
            return RemoteIDIndicator.RIDState.WARNING
        }
        if (regionOperation == RemoteIDIndicator.RegionOperation.EU || QGroundControl.settingsManager.remoteIDSettings.sendOperatorID.value) {
            if (!operatorIDFlag) {
                return RemoteIDIndicator.RIDState.WARNING
            }
        }
        return RemoteIDIndicator.RIDState.HEALTHY
    }

    function goToSettings() {
        if (!mainWindow.preventViewSwitch()) {
            globals.commingFromRIDIndicator = true
            mainWindow.showSettingsTool()
        }
    }

    Image {
        id:                 remoteIDIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        source:             getRIDIcon()
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(indicatorPage, control)
    }

    Component {
        id: indicatorPage

        RemoteIDIndicatorPage { }
    }
}
