pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

// Vehicle GPS status, or the GNSS receiver's while the vehicle reports no GPS, plus correction and resilience state.
// The label beside the icon is the vehicle's fix: "RTK" only once the vehicle reports an RTK fixed fix.
Item {
    id:             control

    property bool   showIndicator:  true
    readonly property Vehicle _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property VehicleGPSFactGroup _gps: _activeVehicle ? _activeVehicle.gps : null
    readonly property VehicleGPSFactGroup _gps2: _activeVehicle ? _activeVehicle.gps2 : null
    readonly property bool _vehicleGps: !!_gps && _gps.telemetryAvailable
    readonly property GPSReceiver _receiver: QGroundControl.gpsManager.receiver
    readonly property GPSReceiverFactGroup _receiverFacts: _receiver.facts
    readonly property bool _receiverConnected: _receiver.state === GPSReceiver.Connected
    readonly property bool _receiverWarning: _receiverConnected && _receiverFacts.receiverWarning
    readonly property int _receiverSatellites: _receiverFacts.numSatellitesUsed.rawValue
    readonly property string _receiverDetail: _receiver.summaryLabel
    readonly property int _correctionState: QGroundControl.gpsManager.corrections.state
    readonly property bool _correctionsActive: _correctionState !== GPSCorrectionManager.Inactive
    // The MAVLink GPS_FIX_TYPE the vehicle reports: 5 is an RTK float fix and 6 an RTK fixed one.
    readonly property int _vehicleFixType: _vehicleGps ? _gps.lock.rawValue : 0
    readonly property bool _vehicleRtkFloat: _vehicleFixType === 5
    readonly property bool _vehicleRtkFixed: _vehicleFixType === 6
    // The vehicle's fix where it reports its GPS. Without that, whether the ground has a correction source.
    readonly property string _fixLabel: {
        if (!_vehicleGps) {
            return _correctionsActive ? qsTr("RTK") : qsTr("GNSS")
        }
        if (_vehicleRtkFixed) {
            return qsTr("RTK")
        }
        //: An RTK float fix, in as few letters as possible: the label stands upright beside the GPS icon
        return _vehicleRtkFloat ? qsTr("Float") : qsTr("GNSS")
    }
    // Corrections that the vehicle has no RTK fix from, or that stopped coming, want attention as a receiver warning
    // does.
    readonly property bool _fixLabelWarning: _receiverWarning || _correctionState === GPSCorrectionManager.Waiting
                                             || (_vehicleGps && _correctionsActive
                                                 && !_vehicleRtkFloat && !_vehicleRtkFixed)
    // The icons show the worse of the two receivers.
    readonly property VehicleGPSFactGroup _authenticationSource:
        _gps2 && (!_gps || _gps2.authenticationSeverity > _gps.authenticationSeverity) ? _gps2 : _gps
    readonly property int _authenticationState: _authenticationSource && _authenticationSource.authenticationReported
                                                ? _authenticationSource.authenticationState.value : 0
    readonly property int _interferenceState: Math.max(control._gps ? control._gps.interferenceState : 0,
                                                       control._gps2 ? control._gps2.interferenceState : 0)
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

    objectName:     "toolbar_gpsIndicator"
    width:          gpsIndicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

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
                objectName:             "gpsCorrectionsLabel"
                rotation:               90
                text:                   control._fixLabel
                textFormat:             Text.PlainText
                color:                  control._fixLabelWarning ? qgcPal.colorOrange : qgcPal.textPrimary
                anchors.verticalCenter: parent.verticalCenter
                visible:                control._receiverConnected || control._correctionsActive
                                        || control._vehicleRtkFloat || control._vehicleRtkFixed
            }

            QGCColoredImage {
                width:              height
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             "/qmlimages/GPS.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                opacity:            (control._vehicleGps ? control._gps.count.value > 0
                                                         : control._receiverConnected && control._receiverSatellites > 0) ? 1 : 0.5
                color:              qgcPal.textPrimary
            }
        }

        // Satellites and HDOP from the vehicle; satellites used and fix from the receiver without vehicle GPS.
        Column {
            anchors.verticalCenter: parent.verticalCenter
            visible:                control._vehicleGps ? !isNaN(control._gps.hdop.value) : control._receiverConnected
            spacing:                0

            QGCLabel {
                objectName:                 "gpsSatelliteCount"
                anchors.horizontalCenter:   gpsDetail.horizontalCenter
                color:                      qgcPal.textPrimary
                text:                       control._vehicleGps ? control._gps.count.valueString
                                            : control._receiverSatellites >= 0 ? String(control._receiverSatellites) : "–"
                textFormat:                 Text.PlainText
            }

            QGCLabel {
                id:         gpsDetail
                objectName: "gpsDetail"
                color:      qgcPal.textPrimary
                text:       control._vehicleGps ? control._gps.hdop.valueString : control._receiverDetail
                textFormat: Text.PlainText
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
