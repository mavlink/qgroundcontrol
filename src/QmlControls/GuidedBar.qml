/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.5

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

//-- Guided mode bar

Rectangle {
    id:     guidedModeBar
    width:  guidedModeColumn.width  + (_margins * 2)
    height: guidedModeColumn.height + (_margins * 2)
    radius: ScreenTools.defaultFontPixelHeight * 0.25
    color:  backgroundColor

    property var    activeVehicle                                           ///< Vehicle to show guided bar for
    property real   fontPointSize:      ScreenTools.defaultFontPointSize    ///< point size for fonts in control
    property color  backgroundColor:    qgcPal.windowShadeDark              ///< Background color for bar

    // Values for _confirmActionCode
    readonly property int confirmHome:          1
    readonly property int confirmLand:          2
    readonly property int confirmTakeoff:       3
    readonly property int confirmArm:           4
    readonly property int confirmDisarm:        5
    readonly property int confirmEmergencyStop: 6
    readonly property int confirmChangeAlt:     7
    readonly property int confirmGoTo:          8
    readonly property int confirmRetask:        9
    readonly property int confirmOrbit:         10

    property int    _confirmActionCode
    property real   _showMargin:    _margins
    property real   _hideMargin:    _margins - guidedModeBar.height
    property real   _barMargin:     _showMargin
    property bool   _showConfirm:   false
    property string _confirmText
    property bool   _showAltitude:  false
    property real   _confirmAltitude

    function actionConfirmed(altitude) {
        _showConfirm = false
        switch (_confirmActionCode) {
        case confirmHome:
            activeVehicle.guidedModeRTL()
            break;
        case confirmLand:
            activeVehicle.guidedModeLand()
            break;
        case confirmTakeoff:
            activeVehicle.guidedModeTakeoff(altitude)
            break;
        case confirmArm:
            activeVehicle.armed = true
            break;
        case confirmDisarm:
            activeVehicle.armed = false
            break;
        case confirmEmergencyStop:
            activeVehicle.emergencyStop()
            break;
        case confirmChangeAlt:
            activeVehicle.guidedModeChangeAltitude(altitude)
            break;
        case confirmGoTo:
            activeVehicle.guidedModeGotoLocation(_flightMap._gotoHereCoordinate)
            break;
        case confirmRetask:
            activeVehicle.setCurrentMissionSequence(_flightMap._retaskSequence)
            break;
        case confirmOrbit:
            //-- All parameters controlled by RC
            activeVehicle.guidedModeOrbit()
            //-- Center on current flight map position and orbit with a 50m radius (velocity/direction controlled by the RC)
            //activeVehicle.guidedModeOrbit(QGroundControl.flightMapPosition, 50.0)
            break;
        default:
            console.warn(qsTr("Internal error: unknown _confirmActionCode"), _confirmActionCode)
        }
    }

    function actionRejected() {
        _showConfirm = false
        /*
            altitudeSlider.visible = false
            _flightMap._gotoHereCoordinate = QtPositioning.coordinate()
            guidedModeHideTimer.restart()
            */
    }

    function confirmAction(actionCode) {
        //guidedModeHideTimer.stop()
        _confirmActionCode = actionCode
        _showAltitude = false
        switch (_confirmActionCode) {
        case confirmArm:
            _confirmText = qsTr("Arm vehicle?")
            break;
        case confirmDisarm:
            _confirmText = qsTr("Disarm vehicle?")
            break;
        case confirmEmergencyStop:
            _confirmText = qsTr("STOP ALL MOTORS?")
            break;
        case confirmTakeoff:
            _showAltitude = true
            setInitialValueMeters(3)
            _confirmText = qsTr("Takeoff vehicle?")
            break;
        case confirmLand:
            _confirmText = qsTr("Land vehicle?")
            break;
        case confirmHome:
            _confirmText = qsTr("Return to land?")
            break;
        case confirmChangeAlt:
            _showAltitude = true
            setInitialValueAppSettingsDistanceUnits(activeVehicle.altitudeRelative.value)
            _confirmText = qsTr("Change altitude?")
            break;
        case confirmGoTo:
            _confirmText = qsTr("Move vehicle?")
            break;
        case confirmRetask:
            _confirmText = qsTr("Change active waypoint?")
            break;
        case confirmOrbit:
            _confirmText = qsTr("Enter orbit mode?")
            break;
        }
        _showConfirm = true
    }

    function setInitialValueMeters(meters) {
        _confirmAltitude = QGroundControl.metersToAppSettingsDistanceUnits(meters)
    }

    function setInitialValueAppSettingsDistanceUnits(height) {
        _confirmAltitude = height
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Column {
        id:                 guidedModeColumn
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.left:       parent.left
        spacing:            _margins

        /*
            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                color:      _lightWidgetBorders ? qgcPal.mapWidgetBorderDark : qgcPal.mapWidgetBorderLight
                text:       "Click in map to move vehicle"
                visible:    gotoEnabled
            }*/

        Row {
            spacing:    _margins * 2
            visible:    !_showConfirm

            QGCButton {
                pointSize:  fontPointSize
                text:       (activeVehicle && activeVehicle.armed) ? (activeVehicle.flying ? qsTr("Emergency Stop") : qsTr("Disarm")) :  qsTr("Arm")
                visible:    activeVehicle
                onClicked:  confirmAction(activeVehicle.armed ? (activeVehicle.flying ? confirmEmergencyStop : confirmDisarm) : confirmArm)
            }

            QGCButton {
                pointSize:  fontPointSize
                text:       qsTr("RTL")
                visible:    (activeVehicle && activeVehicle.armed) && activeVehicle.guidedModeSupported && activeVehicle.flying
                onClicked:  confirmAction(confirmHome)
            }

            QGCButton {
                pointSize:  fontPointSize
                text:       (activeVehicle && activeVehicle.flying) ?  qsTr("Land"):  qsTr("Takeoff")
                visible:    activeVehicle && activeVehicle.guidedModeSupported && activeVehicle.armed
                onClicked:  confirmAction(activeVehicle.flying ? confirmLand : confirmTakeoff)
            }

            QGCButton {
                pointSize:  fontPointSize
                text:       qsTr("Pause")
                visible:    (activeVehicle && activeVehicle.armed) && activeVehicle.pauseVehicleSupported && activeVehicle.flying
                onClicked:  {
                    guidedModeHideTimer.restart()
                    activeVehicle.pauseVehicle()
                }
            }

            QGCButton {
                pointSize:  fontPointSize
                text:       qsTr("Change Altitude")
                visible:    (activeVehicle && activeVehicle.flying) && activeVehicle.guidedModeSupported && activeVehicle.armed
                onClicked:  confirmAction(confirmChangeAlt)
            }

            QGCButton {
                pointSize:  fontPointSize
                text:       qsTr("Orbit")
                visible:    (activeVehicle && activeVehicle.flying) && activeVehicle.orbitModeSupported && activeVehicle.armed
                onClicked:  confirmAction(confirmOrbit)
            }

        } // Row

        Column {
            visible: _showConfirm

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                text:                       _confirmText
            }

            Row {
                anchors.horizontalCenter:   parent.horizontalCenter
                spacing:                    ScreenTools.defaultFontPixelWidth

                Row {
                    visible: _showAltitude

                    QGCLabel {
                        text: qsTr("Alt (rel)")
                    }

                    QGCLabel {
                        text: QGroundControl.appSettingsDistanceUnitsString
                    }

                    QGCTextField {
                        id:     altField
                        text:   _confirmAltitude.toFixed(1)
                    }
                }

                QGCButton {
                    text: qsTr("Yes")

                    onClicked: {
                        var value = parseFloat(altField.text)
                        if (isNaN(value)) {
                            actionRejected()
                        } else {
                            actionConfirmed(QGroundControl.appSettingsDistanceUnitsToMeters(value))
                        }
                    }
                }

                QGCButton {
                    text: qsTr("No")
                    onClicked: actionRejected()
                }
            }
        } // Column
    } // Column
} // Rectangle - Guided mode buttons
