pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

// Vehicle GPS status, or the GNSS receiver's while the vehicle reports no GPS, plus correction and resilience state.
Item {
    id:             control
    objectName:     "toolbar_gpsIndicator"
    width:          gpsIndicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool   showIndicator:  true
    property Vehicle _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _vehicleGps: !!_activeVehicle && !!_activeVehicle.gps && _activeVehicle.gps.telemetryAvailable
    property GPSReceiver _receiver:      QGroundControl.gpsManager.receiver
    property GPSReceiverFactGroup _receiverFacts: QGroundControl.gpsManager.receiverFacts
    property bool   _rtkConnected:  _receiverFacts.connected.value
    readonly property bool _rtkInterference: _rtkConnected && _receiverFacts.interferenceWarning
    readonly property int _receiverSatellites: _receiverFacts.numSatellitesUsed.rawValue
    readonly property string _receiverDetail: _receiverFacts.summaryLabel
    property int    _correctionState: QGroundControl.gpsManager.correctionState
    readonly property bool _showRtk: _correctionState !== GPSManager.Inactive
    property VehicleGPSAggregateFactGroup _gpsAggregate: _activeVehicle ? _activeVehicle.gpsAggregate : null
    readonly property int _authenticationState: _gpsAggregate && _gpsAggregate.authenticationReported
                                                ? _gpsAggregate.authenticationState.value : 0
    readonly property int _interferenceState: _gpsAggregate ? _gpsAggregate.interferenceState : 0
    readonly property color _authenticationColor: {
        switch (_authenticationState) {
        case 1: return qgcPal.colorYellow   // Initializing
        case 2: return qgcPal.colorRed      // Error
        case 3: return qgcPal.colorGreen    // OK
        default: return qgcPal.colorGrey    // Disabled
        }
    }
    readonly property color _interferenceColor: {
        switch (_interferenceState) {
        case 1: return qgcPal.colorGreen    // Not spoofed or jammed
        case 2: return qgcPal.colorOrange   // Mitigated
        default: return qgcPal.colorRed     // Ongoing
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
                color:                  control._rtkInterference || control._correctionState === GPSManager.Waiting
                                        ? qgcPal.colorOrange : qgcPal.text
                anchors.verticalCenter: parent.verticalCenter
                visible:                control._rtkConnected || control._showRtk
            }

            QGCColoredImage {
                id:                 gpsIcon
                width:              height
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             "/qmlimages/GPS.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                opacity:            (control._vehicleGps ? control._activeVehicle.gps.count.value >= 0
                                                         : control._rtkConnected && control._receiverSatellites > 0) ? 1 : 0.5
                color:              qgcPal.text
            }
        }

        // Satellites and HDOP from the vehicle; satellites used and fix from the receiver without vehicle GPS.
        Column {
            id:                     gpsValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            visible:                control._vehicleGps ? !isNaN(control._activeVehicle.gps.hdop.value) : control._rtkConnected
            spacing:                0

            QGCLabel {
                objectName:                 "gpsSatelliteCount"
                anchors.horizontalCenter:   gpsDetail.horizontalCenter
                color:                      qgcPal.text
                text:                       control._vehicleGps ? control._activeVehicle.gps.count.valueString
                                            : control._receiverSatellites >= 0 ? String(control._receiverSatellites) : "–"
            }

            QGCLabel {
                id:         gpsDetail
                objectName: "gpsDetail"
                color:      qgcPal.text
                text:       control._vehicleGps ? control._activeVehicle.gps.hdop.valueString : control._receiverDetail
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
