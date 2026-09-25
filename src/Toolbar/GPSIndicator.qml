import QtQuick
import QtQuick.Layouts

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
    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _vehicleGps: !!_activeVehicle && !!_activeVehicle.gps && _activeVehicle.gps.telemetryAvailable
    property var    _receiver:      QGroundControl.gpsManager.gpsRtk
    property var    _rtkFacts:      QGroundControl.gpsManager.gpsRtkFacts
    property bool   _rtkConnected:  _rtkFacts.connected.value
    readonly property bool _rtkInterference: _rtkConnected && _rtkFacts.interferenceWarning
    readonly property int _receiverSatellites: _rtkFacts.numSatellitesUsed.rawValue
    readonly property string _receiverDetail: {
        if (_receiver.activeRole === GPSRtk.ConfiguredBase) {
            return _rtkFacts.active.value ? qsTr("Survey", "Base survey-in in progress") : qsTr("Base")
        }
        switch (_rtkFacts.fixType.rawValue) {
        case 1: return qsTr("No fix")
        case 2: return qsTr("2D")
        case 3: return qsTr("3D")
        case 4: return qsTr("DGPS")
        case 5: return qsTr("Float", "RTK float fix")
        case 6: return qsTr("Fixed", "RTK fixed fix")
        case 8: return qsTr("DR", "Dead reckoning (extrapolated) fix")
        default: return ""
        }
    }
    property int    _correctionState: QGroundControl.gpsManager.correctionState
    readonly property bool _showRtk: _correctionState !== GPSManager.Inactive
    property var    _gpsAggregate:  _activeVehicle ? _activeVehicle.gpsAggregate : null
    // Resilience states 0 and 255 mean the vehicle does not know.
    readonly property int _authenticationState: {
        const value = _gpsAggregate ? _gpsAggregate.authenticationState.value : 0
        return value > 0 && value < 255 ? value : 0
    }
    readonly property int _interferenceState: {
        if (!_gpsAggregate) return 0
        const spoofing = _gpsAggregate.spoofingState.value
        const jamming = _gpsAggregate.jammingState.value
        return Math.max(spoofing > 0 && spoofing < 255 ? spoofing : 0,
                        jamming > 0 && jamming < 255 ? jamming : 0)
    }
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
                source:             "/qmlimages/Gps.svg"
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
                text:       control._vehicleGps ? control._activeVehicle.gps.hdop.value.toFixed(1) : control._receiverDetail
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
                source:             "/qmlimages/GpsAuthentication.svg"
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
                source:             "/qmlimages/GpsInterference.svg"
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
