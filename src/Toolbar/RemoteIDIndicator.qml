import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

//-------------------------------------------------------------------------
//-- Remote ID Indicator
Item {
    id:             control
    width:          remoteIDIcon.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool   showIndicator:      remoteIDManager ? remoteIDManager.available : false

    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property var    remoteIDManager:    activeVehicle ? activeVehicle.remoteIDManager : null

    property bool   gpsFlag:            remoteIDManager ? remoteIDManager.gcsGPSGood         : false
    property bool   basicIDFlag:        remoteIDManager ? remoteIDManager.basicIDGood        : false
    property bool   armFlag:            remoteIDManager ? remoteIDManager.armStatusGood      : false
    property bool   commsFlag:          remoteIDManager ? remoteIDManager.commsGood          : false
    property bool   emergencyDeclared:  remoteIDManager ? remoteIDManager.emergencyDeclared  : false
    property bool   operatorIDFlag:     remoteIDManager ? remoteIDManager.operatorIDGood     : false
    property bool   sendOperatorID:     QGroundControl.settingsManager.remoteIDSettings.sendOperatorID.value
    property int    remoteIDState:      getRemoteIDState(activeVehicle, commsFlag, armFlag, emergencyDeclared, gpsFlag, basicIDFlag, regionOperation, sendOperatorID, operatorIDFlag)

    property int    regionOperation:    QGroundControl.settingsManager.remoteIDSettings.region.value

    enum RIDState {
        HEALTHY,
        WARNING,
        ERROR,
        UNAVAILABLE
    }

    function getRidColor() {
        switch (remoteIDState) {
            case RemoteIDIndicator.RIDState.HEALTHY:
                return qgcPal.colorGreen
            case RemoteIDIndicator.RIDState.WARNING:
                return qgcPal.colorYellow
            case RemoteIDIndicator.RIDState.ERROR:
                return qgcPal.colorRed
            case RemoteIDIndicator.RIDState.UNAVAILABLE:
                return qgcPal.colorGrey
            default:
                return qgcPal.colorGrey
        }
    }

    function getRemoteIDState(vehicle, commsOk, armOk, emergencyActive, gpsOk, basicIDOk, region, sendOperatorIDEnabled, operatorIDOk) {
        if (!vehicle) {
            return RemoteIDIndicator.RIDState.UNAVAILABLE
        }
        // We need to have comms and arm healthy to even be in any other state other than ERROR
        if (!commsOk || !armOk || emergencyActive) {
            return RemoteIDIndicator.RIDState.ERROR
        }
        if (!gpsOk || !basicIDOk) {
            return RemoteIDIndicator.RIDState.WARNING
        }
        if (region == RemoteIDSettings.RegionOperation.EU || sendOperatorIDEnabled) {
            if (!operatorIDOk) {
                return RemoteIDIndicator.RIDState.WARNING
            }
        }
        return RemoteIDIndicator.RIDState.HEALTHY
    }

    function goToSettings() {
        if (mainWindow.allowViewSwitch()) {
            globals.commingFromRIDIndicator = true
            mainWindow.showSettingsTool()
        }
    }

    QGCPalette { id: qgcPal }

    QGCColoredImage {
        id:                 remoteIDIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        source:             "/qmlimages/RidIconMan.svg"
        color:              getRidColor()
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height

        QGCColoredImage {
            width:              height
            anchors.fill:       parent
            sourceSize.height:  height
            source:             "/qmlimages/RidIconText.svg"
            fillMode:           Image.PreserveAspectFit
            color:              qgcPal.text
        }
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
