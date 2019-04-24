/****************************************************************************
 *
 * (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.11
import QtQuick.Controls         2.4
import QtQuick.Layouts          1.11
import QtQuick.Dialogs          1.3
import QtPositioning            5.2

import QGroundControl                       1.0
import QGroundControl.Controllers           1.0
import QGroundControl.Controls              1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Vehicle               1.0
import QGroundControl.QGCPositionManager    1.0

import AuterionQuickInterface               1.0
import Auterion.Widgets                     1.0

Item {
    anchors.fill: parent
    visible:    !QGroundControl.videoManager.fullScreen

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property string scaleState:            "topMode"
    readonly property string noGPS:                 qsTr("NO GPS")
    readonly property real   indicatorValueWidth:   ScreenTools.defaultFontPixelWidth * 7

    property real   _indicatorDiameter:     ScreenTools.defaultFontPixelWidth * 18
    property var    _sepColor:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.5) : Qt.rgba(1,1,1,0.5)
    property color  _indicatorsColor:       "white"

    property bool   _communicationLost:     activeVehicle ? activeVehicle.connectionLost : false
    property bool   _isVehicleGps:          activeVehicle && activeVehicle.gps && activeVehicle.gps.count.rawValue > 1 && activeVehicle.gps.hdop.rawValue < 1.4
    property var    _dynamicCameras:        activeVehicle ? activeVehicle.dynamicCameras : null
    property bool   _isCamera:              _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property int    _curCameraIndex:        _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property var    _camera:                _isCamera ? _dynamicCameras.cameras.get(_curCameraIndex) : null
    property bool   _cameraPresent:         _camera && _camera.cameraMode !== QGCCameraControl.CAM_MODE_UNDEFINED

    property string _altitude:              activeVehicle ? (isNaN(activeVehicle.altitudeRelative.value) ? "0.0" : activeVehicle.altitudeRelative.value.toFixed(1)) + ' ' + activeVehicle.altitudeRelative.units : "0.0"
    property string _distanceStr:           isNaN(_distance) ? "0" : _distance.toFixed(0) + ' ' + (activeVehicle ? activeVehicle.altitudeRelative.units : "")
    property real   _heading:               activeVehicle   ? activeVehicle.heading.rawValue : 0

    property real   _distance:              0.0
    property string _messageTitle:          ""
    property string _messageText:           ""

    function secondsToHHMMSS(timeS) {
        var sec_num = parseInt(timeS, 10);
        var hours   = Math.floor(sec_num / 3600);
        var minutes = Math.floor((sec_num - (hours * 3600)) / 60);
        var seconds = sec_num - (hours * 3600) - (minutes * 60);
        if (hours   < 10) {hours   = "0"+hours;}
        if (minutes < 10) {minutes = "0"+minutes;}
        if (seconds < 10) {seconds = "0"+seconds;}
        return hours+':'+minutes+':'+seconds;
    }

    Timer {
        id:        connectionTimer
        interval:  5000
        running:   false;
        repeat:    false;
        onTriggered: {
            //-- Vehicle is gone
            if(activeVehicle) {
                //-- Let video stream close
                QGroundControl.settingsManager.videoSettings.rtspTimeout.rawValue = 1
                if(!activeVehicle.armed) {
                    //-- If it wasn't already set to auto-disconnect
                    if(!activeVehicle.autoDisconnect) {
                        //-- Vehicle is not armed. Close connection and tell user.
                        activeVehicle.disconnectInactiveVehicle()
                        connectionLostDisarmedDialog.open()
                    }
                } else {
                    //-- Vehicle is armed. Show doom dialog.
                    connectionLostArmed.open()
                }
            }
        }
    }

    Connections {
        target: QGroundControl.qgcPositionManger
        onGcsPositionChanged: {
            if (activeVehicle && gcsPosition.latitude && Math.abs(gcsPosition.latitude)  > 0.001 && gcsPosition.longitude && Math.abs(gcsPosition.longitude)  > 0.001) {
                var gcs = QtPositioning.coordinate(gcsPosition.latitude, gcsPosition.longitude)
                var veh = activeVehicle.coordinate;
                _distance = QGroundControl.metersToAppSettingsDistanceUnits(gcs.distanceTo(veh));
                //-- Ignore absurd values
                if(_distance > 99999)
                    _distance = 0;
                if(_distance < 0)
                    _distance = 0;
            } else {
                _distance = 0;
            }
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle
        onConnectionLostChanged: {
            if(!_communicationLost) {
                //-- Communication regained
                connectionTimer.stop();
                if(connectionLostArmed.visible) {
                    connectionLostArmed.close()
                }
                //-- Reset stream timeout
                QGroundControl.settingsManager.videoSettings.rtspTimeout.rawValue = 60
            } else {
                if(activeVehicle && !activeVehicle.autoDisconnect) {
                    //-- Communication lost
                    connectionTimer.start();
                }
            }
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        onVehicleAdded: {
            //-- Dismiss comm lost dialog if open
            connectionLostDisarmedDialog.close()
        }
    }

    MessageDialog {
        id:                 connectionLostDisarmedDialog
        title:              qsTr("Communication Lost")
        text:               qsTr("Connection to vehicle has been lost and closed.")
        standardButtons:    StandardButton.Ok
        onAccepted: {
            connectionLostDisarmedDialog.close()
        }
    }

    //-- Heading Indicator
    Rectangle {
        id:             compassBar
        height:         ScreenTools.defaultFontPixelHeight * 1.5
        width:          ScreenTools.defaultFontPixelWidth  * 50
        color:          "#C5C6C9"
        radius:         2
        clip:           true
        anchors.top:    parent.top
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 2
        anchors.horizontalCenter: parent.horizontalCenter
        Repeater {
            model: 720
            QGCLabel {
                function _normalize(degrees) {
                    var a = degrees % 360
                    if (a < 0) a += 360
                    return a
                }
                property int _startAngle: modelData + 180 + _heading
                property int _angle: _normalize(_startAngle)
                anchors.verticalCenter: parent.verticalCenter
                x:              visible ? ((modelData * (compassBar.width / 360)) - (width * 0.5)) : 0
                visible:        _angle % 45 == 0
                color:          "#505565"
                font.pointSize: ScreenTools.smallFontPointSize
                text: {
                    switch(_angle) {
                    case 0:     return "N"
                    case 45:    return "NE"
                    case 90:    return "E"
                    case 135:   return "SE"
                    case 180:   return "S"
                    case 225:   return "SW"
                    case 270:   return "W"
                    case 315:   return "NW"
                    }
                    return ""
                }
            }
        }
    }
    Rectangle {
        id:             headingIndicator
        height:         ScreenTools.defaultFontPixelHeight
        width:          ScreenTools.defaultFontPixelWidth * 4
        color:          "#0B1629"
        anchors.bottom: compassBar.top
        anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight * -0.1
        anchors.horizontalCenter:   parent.horizontalCenter
        QGCLabel {
            text:               _heading
            color:              "white"
            font.pointSize:     ScreenTools.smallFontPointSize
            anchors.centerIn:   parent
        }
    }
    Image {
        height:         ScreenTools.defaultFontPixelHeight * 0.75
        width:          height
        source:         "/auterion/img/compass_pointer.svg"
        fillMode:       Image.PreserveAspectFit
        sourceSize.height:  height
        anchors.top:    compassBar.bottom
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * -0.5
        anchors.horizontalCenter: parent.horizontalCenter
    }

    //-- Camera Control
    Loader {
        id:                     camControlLoader
        visible:                !_mainIsMap && _cameraPresent && _camera.paramComplete
        source:                 visible ? "/auterion/AuterionCameraControl.qml" : ""
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.top:            parent.top
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 4
    }

    //-- Vehicle Indicator
    Rectangle {
        color:                  "#0B1629"
        width:                  vehicleStatusGrid.width  + (ScreenTools.defaultFontPixelWidth  * 3)
        height:                 vehicleStatusGrid.height + (ScreenTools.defaultFontPixelHeight * 1.5)
        radius:                 2
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   ScreenTools.defaultFontPixelWidth
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        GridLayout {
            id:                     vehicleStatusGrid
            columnSpacing:          ScreenTools.defaultFontPixelWidth  * 1.5
            rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.5
            columns:                6
            anchors.centerIn:       parent
            //-- Chronometer
            Image {
                height:                 ScreenTools.defaultFontPixelHeight * 0.75
                width:                  height
                source:                 "/auterion/img/chronometer.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
            }
            QGCLabel {
                text: {
                    if(activeVehicle)
                        return secondsToHHMMSS(activeVehicle.getFact("flightTime").value)
                    return "00:00:00"
                }
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    Text.AlignRight
            }
            //-- Ground Speed
            Image {
                height:                 ScreenTools.defaultFontPixelHeight * 0.75
                width:                  height
                source:                 "/auterion/img/horizontal_speed.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
            }
            QGCLabel {
                text:                   activeVehicle ? activeVehicle.groundSpeed.value.toFixed(1) + ' ' + activeVehicle.groundSpeed.units : "0.0"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    Text.AlignRight
            }
            //-- Vertical Speed
            Image {
                height:                 ScreenTools.defaultFontPixelHeight * 0.75
                width:                  height
                source:                 "/auterion/img/vertical_speed.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
            }
            QGCLabel {
                text:                   activeVehicle ? activeVehicle.climbRate.value.toFixed(1) + ' ' + activeVehicle.climbRate.units : "0.0"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    Text.AlignRight
            }
            //-- Second Row
            //-- Odometer
            Image {
                height:                 ScreenTools.defaultFontPixelHeight * 0.75
                width:                  height
                source:                 "/auterion/img/odometer.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
            }
            QGCLabel {
                text:                   activeVehicle ? ('00000' + activeVehicle.flightDistance.value.toFixed(0)).slice(-5) + ' ' + activeVehicle.flightDistance.units : "00000"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    Text.AlignRight
            }
            //-- Altitude
            Image {
                height:                 ScreenTools.defaultFontPixelHeight * 0.75
                width:                  height
                source:                 "/auterion/img/altitude.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
            }
            QGCLabel {
                text:                   _altitude
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    Text.AlignRight
            }
            //-- Distance
            Image {
                height:                 ScreenTools.defaultFontPixelHeight * 0.75
                width:                  height
                source:                 "/auterion/img/distance.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
            }
            QGCLabel {
                text:                   _distance ? _distanceStr : noGPS
                color:                  _distance ? _indicatorsColor : qgcPal.colorOrange
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    Text.AlignRight
            }
        }
    }

    //-- Connection Lost While Armed
    Popup {
        id:                     connectionLostArmed
        width:                  mainWindow.width  * 0.666
        height:                 connectionLostArmedCol.height * 1.5
        modal:                  true
        focus:                  true
        parent:                 Overlay.overlay
        x:                      Math.round((mainWindow.width  - width)  * 0.5)
        y:                      Math.round((mainWindow.height - height) * 0.5)
        closePolicy:            Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            anchors.fill:       parent
            color:              qgcPal.alertBackground
            border.color:       qgcPal.alertBorder
            radius:             ScreenTools.defaultFontPixelWidth
        }
        Column {
            id:                 connectionLostArmedCol
            spacing:            ScreenTools.defaultFontPixelHeight * 3
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.centerIn:   parent
            QGCLabel {
                text:           qsTr("Communication Lost")
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.largeFontPointSize
                color:          qgcPal.alertText
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCLabel {
                text:           qsTr("Warning: Connection to vehicle lost.")
                color:          qgcPal.alertText
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCLabel {
                text:           qsTr("The vehicle will automatically cancel the flight and return to land. Ensure a clear line of sight between transmitter and vehicle. Ensure the takeoff location is clear.")
                width:          connectionLostArmed.width * 0.75
                wrapMode:       Text.WordWrap
                color:          qgcPal.alertText
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
