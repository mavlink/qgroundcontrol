pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

// Vehicle GPS status, or the GNSS receiver's while the vehicle reports no GPS, plus correction and resilience state.
Item {
    id:             control
    objectName:     "toolbar_gpsIndicator"
    width:          gpsIndicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool   showIndicator:  true
    property Vehicle _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property VehicleGPSFactGroup _gps: _activeVehicle ? _activeVehicle.gps : null
    property VehicleGPSFactGroup _gps2: _activeVehicle ? _activeVehicle.gps2 : null
    readonly property bool _vehicleGps: !!_gps && _gps.telemetryAvailable
    property GPSReceiver _receiver: QGroundControl.gpsManager.receiver
    readonly property GPSReceiverFactGroup _receiverFacts: _receiver.facts
    property bool   _receiverConnected: _receiverFacts.telemetryAvailable
    readonly property bool _receiverWarning: _receiverConnected && _receiverFacts.receiverWarning
    readonly property int _receiverSatellites: _receiverFacts.numSatellitesUsed.rawValue
    readonly property string _receiverDetail: _receiver.summaryLabel
    property int    _correctionState: QGroundControl.gpsManager.corrections.state
    readonly property bool _showRtk: _correctionState !== GPSCorrectionManager.Inactive
    // The icons show the worse of the two receivers.
    readonly property VehicleGPSFactGroup _authenticationSource:
        _gps2 && (!_gps || _gps2.authenticationSeverity > _gps.authenticationSeverity) ? _gps2 : _gps
    readonly property int _authenticationState: _authenticationSource && _authenticationSource.authenticationReported
                                                ? _authenticationSource.authenticationState.value : 0
    readonly property int _interferenceState: {
        const first = control._gps ? control._gps.interferenceState : 0
        const second = control._gps2 ? control._gps2.interferenceState : 0
        return control._interferenceSeverity(second) > control._interferenceSeverity(first) ? second : first
    }
    readonly property color _authenticationColor: {
        switch (_authenticationState) {
        case VehicleGPSFactGroup.AuthenticationState.Initializing: return qgcPal.colorYellow
        case VehicleGPSFactGroup.AuthenticationState.Error: return qgcPal.colorRed
        case VehicleGPSFactGroup.AuthenticationState.Ok: return qgcPal.colorGreen
        default: return qgcPal.colorGrey
        }
    }
    readonly property color _interferenceColor: {
        switch (_interferenceState) {
        case VehicleGPSFactGroup.InterferenceState.NotDetected: return qgcPal.colorGreen
        case VehicleGPSFactGroup.InterferenceState.Mitigated: return qgcPal.colorOrange
        default: return qgcPal.colorRed
        }
    }

    function _interferenceSeverity(state) {
        switch (state) {
        case VehicleGPSFactGroup.InterferenceState.NotDetected: return 1
        case VehicleGPSFactGroup.InterferenceState.Mitigated: return 2
        case VehicleGPSFactGroup.InterferenceState.Detected: return 3
        default: return 0
        }
    }

    QGCPalette { id: qgcPal }

    Row {
        id:             gpsIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        Row {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            spacing:        -ScreenTools.defaultFontPixelWidth / 2

            QGCLabel {
                id:                     gpsLabel
                objectName:             "gpsCorrectionsLabel"
                rotation:               90
                text:                   control._showRtk ? qsTr("RTK") : qsTr("GNSS")
                color:                  control._receiverWarning || control._correctionState === GPSCorrectionManager.Waiting
                                        ? qgcPal.colorOrange : qgcPal.text
                anchors.verticalCenter: parent.verticalCenter
                visible:                control._receiverConnected || control._showRtk
            }

            QGCColoredImage {
                id:                 gpsIcon
                width:              height
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             "/qmlimages/GPS.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                opacity:            (control._vehicleGps ? control._gps.count.value > 0
                                                         : control._receiverConnected && control._receiverSatellites > 0) ? 1 : 0.5
                color:              qgcPal.text
            }
        }

        // Satellites and HDOP from the vehicle; satellites used and fix from the receiver without vehicle GPS.
        Column {
            id:                     gpsValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            visible:                control._vehicleGps ? !isNaN(control._gps.hdop.value) : control._receiverConnected
            spacing:                0

            QGCLabel {
                objectName:                 "gpsSatelliteCount"
                anchors.horizontalCenter:   gpsDetail.horizontalCenter
                color:                      qgcPal.text
                text:                       control._vehicleGps ? control._gps.count.valueString
                                            : control._receiverSatellites >= 0 ? String(control._receiverSatellites) : "–"
            }

            QGCLabel {
                id:         gpsDetail
                objectName: "gpsDetail"
                color:      qgcPal.text
                text:       control._vehicleGps ? control._gps.hdop.valueString : control._receiverDetail
            }
        }

        Item {
            width:          height
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            visible:        control._authenticationState > 0 || control._interferenceState > 0

            QGCColoredImage {
                width:              parent.height * 0.95
                height:             width
                anchors.centerIn:   parent
                objectName:         "gpsAuthenticationIcon"
                source:             "/qmlimages/GPSAuthentication.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              control._authenticationColor
                visible:            control._authenticationState > 0
            }

            QGCColoredImage {
                objectName:         "gpsInterferenceIcon"
                width:              parent.height * 0.55
                height:             width
                anchors.centerIn:   parent
                source:             "/qmlimages/GPSInterference.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              control._interferenceColor
                visible:            control._interferenceState > 0
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(gpsIndicatorPage, control)
    }

    Component {
        id: gpsIndicatorPage

        GPSIndicatorPage { }
    }
}
