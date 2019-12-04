/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

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
import QGroundControl.Airspace              1.0

import CustomQuickInterface                 1.0
import Custom.Widgets                       1.0

Item {
    anchors.fill:                           parent
    visible:                                !QGroundControl.videoManager.fullScreen

    readonly property string scaleState:    "topMode"
    readonly property string noGPS:         qsTr("NO GPS")
    readonly property real   indicatorValueWidth:   ScreenTools.defaultFontPixelWidth * 7

    property real   _indicatorDiameter:     ScreenTools.defaultFontPixelWidth * 18
    property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
    property var    _sepColor:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.5) : Qt.rgba(1,1,1,0.5)
    property color  _indicatorsColor:       qgcPal.text

    property bool   _communicationLost:     activeVehicle ? activeVehicle.connectionLost : false
    property bool   _isVehicleGps:          activeVehicle && activeVehicle.gps && activeVehicle.gps.count.rawValue > 1 && activeVehicle.gps.hdop.rawValue < 1.4
    property var    _dynamicCameras:        activeVehicle ? activeVehicle.dynamicCameras : null
    property bool   _isCamera:              _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property int    _curCameraIndex:        _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property var    _camera:                _isCamera ? _dynamicCameras.cameras.get(_curCameraIndex) : null
    property bool   _cameraPresent:         _camera && _camera.cameraMode !== QGCCameraControl.CAM_MODE_UNDEFINED
    property var    _flightPermit:          QGroundControl.airmapSupported ? QGroundControl.airspaceManager.flightPlan.flightPermitStatus : null
    property bool   _hasGimbal:             activeVehicle && activeVehicle.gimbalData

    property bool   _airspaceIndicatorVisible: QGroundControl.airmapSupported && mainIsMap && _flightPermit && _flightPermit !== AirspaceFlightPlanProvider.PermitNone

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
    //-------------------------------------------------------------------------
    MessageDialog {
        id:                 connectionLostDisarmedDialog
        title:              qsTr("Communication Lost")
        text:               qsTr("Connection to vehicle has been lost and closed.")
        standardButtons:    StandardButton.Ok
        onAccepted: {
            connectionLostDisarmedDialog.close()
        }
    }
    //-------------------------------------------------------------------------
    //-- Heading Indicator
    Rectangle {
        id:             compassBar
        height:         ScreenTools.defaultFontPixelHeight * 1.5
        width:          ScreenTools.defaultFontPixelWidth  * 50
        color:          "#DEDEDE"
        radius:         2
        clip:           true
        anchors.top:    parent.top
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * (_airspaceIndicatorVisible ? 3 : 1)
        anchors.horizontalCenter: parent.horizontalCenter
        visible:        !mainIsMap
        Repeater {
            model: 720
            visible:    !mainIsMap
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
                color:          "#75505565"
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
        id:                         headingIndicator
        height:                     ScreenTools.defaultFontPixelHeight
        width:                      ScreenTools.defaultFontPixelWidth * 4
        color:                      qgcPal.windowShadeDark
        visible:                    !mainIsMap
        anchors.bottom:             compassBar.top
        anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight * -0.1
        anchors.horizontalCenter:   parent.horizontalCenter
        QGCLabel {
            text:                   _heading
            color:                  qgcPal.text
            font.pointSize:         ScreenTools.smallFontPointSize
            anchors.centerIn:       parent
        }
    }
    Image {
        height:                     _indicatorsHeight
        width:                      height
        source:                     "/custom/img/compass_pointer.svg"
        visible:                    !mainIsMap
        fillMode:                   Image.PreserveAspectFit
        sourceSize.height:          height
        anchors.top:                compassBar.bottom
        anchors.topMargin:          ScreenTools.defaultFontPixelHeight * -0.5
        anchors.horizontalCenter:   parent.horizontalCenter
    }
    //-------------------------------------------------------------------------
    //-- Camera Control
    Loader {
        id:                     camControlLoader
        visible:                !mainIsMap && _cameraPresent && _camera.paramComplete
        source:                 visible ? "/custom/CustomCameraControl.qml" : ""
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.top:            parent.top
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight
    }
    //-------------------------------------------------------------------------
    //-- Map Scale
    MapScale {
        id:                     mapScale
        anchors.left:           parent.left
        anchors.top:            parent.top
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.5
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth  * 16
        mapControl:             mainWindow.flightDisplayMap
        visible:                rootBackground.visible && mainIsMap
    }
    //-------------------------------------------------------------------------
    //-- Vehicle Indicator
    Rectangle {
        id:                     vehicleIndicator
        color:                  qgcPal.window
        width:                  vehicleStatusGrid.width  + (ScreenTools.defaultFontPixelWidth  * 3)
        height:                 vehicleStatusGrid.height + (ScreenTools.defaultFontPixelHeight * 1.5)
        radius:                 2
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   ScreenTools.defaultFontPixelWidth
        anchors.right:          attitudeIndicator.visible ? attitudeIndicator.left : parent.right
        anchors.rightMargin:    attitudeIndicator.visible ? -ScreenTools.defaultFontPixelWidth : ScreenTools.defaultFontPixelWidth

        readonly property bool  _showGps: CustomQuickInterface.showAttitudeWidget


        GridLayout {
            id:                     vehicleStatusGrid
            columnSpacing:          ScreenTools.defaultFontPixelWidth  * 1.5
            rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.5
            columns:                7
            anchors.centerIn:       parent

            //-- Latitude
            QGCLabel {
                height:                 _indicatorsHeight
                width:                  height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
                text:                   "Lat:"
                visible:                vehicleIndicator._showGps
            }
            QGCLabel {
                id:                     firstLabel
                text:                   activeVehicle ? activeVehicle.gps.lat.value.toFixed(activeVehicle.gps.lat.decimalPlaces) : "-"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    Text.AlignLeft
                visible:                vehicleIndicator._showGps
            }
            //-- Longitude
            QGCLabel {
                height:                 _indicatorsHeight
                width:                  height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
                text:                   "Lon:"
                visible:                vehicleIndicator._showGps
            }
            QGCLabel {
                text:                   activeVehicle ? activeVehicle.gps.lon.value.toFixed(activeVehicle.gps.lon.decimalPlaces) : "-"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    firstLabel.horizontalAlignment
                visible:                vehicleIndicator._showGps
            }
            //-- HDOP
            QGCLabel {
                height:                 _indicatorsHeight
                width:                  height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
                text:                   "HDOP:"
                visible:                vehicleIndicator._showGps
            }
            QGCLabel {
                text:                   activeVehicle ? activeVehicle.gps.hdop.value.toFixed(activeVehicle.gps.hdop.decimalPlaces) : "-"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    firstLabel.horizontalAlignment
                visible:                vehicleIndicator._showGps
            }

            //-- Compass
            Item {
                Layout.rowSpan:         3
                Layout.column:          6
                Layout.minimumWidth:    mainIsMap ? parent.height * 1.25 : 0
                Layout.fillHeight:      true
                Layout.fillWidth:       true
                //-- Large circle
                Rectangle {
                    height:             mainIsMap ? parent.height : 0
                    width:              mainIsMap ? height : 0
                    radius:             height * 0.5
                    border.color:       qgcPal.text
                    border.width:       1
                    color:              Qt.rgba(0,0,0,0)
                    anchors.centerIn:   parent
                    visible:            mainIsMap
                }
                //-- North Label
                Rectangle {
                    height:             mainIsMap ? ScreenTools.defaultFontPixelHeight * 0.75 : 0
                    width:              mainIsMap ? ScreenTools.defaultFontPixelWidth  * 2 : 0
                    radius:             ScreenTools.defaultFontPixelWidth  * 0.25
                    color:              qgcPal.windowShade
                    visible:            mainIsMap
                    anchors.top:        parent.top
                    anchors.topMargin:  ScreenTools.defaultFontPixelHeight * -0.25
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        text:               "N"
                        color:              qgcPal.text
                        font.pointSize:     ScreenTools.smallFontPointSize
                        anchors.centerIn:   parent
                    }
                }
                //-- Needle
                Image {
                    id:                 compassNeedle
                    anchors.centerIn:   parent
                    height:             mainIsMap ? parent.height * 0.75 : 0
                    width:              height
                    source:             "/custom/img/compass_needle.svg"
                    fillMode:           Image.PreserveAspectFit
                    visible:            mainIsMap
                    sourceSize.height:  height
                    transform: [
                        Rotation {
                            origin.x:   compassNeedle.width  / 2
                            origin.y:   compassNeedle.height / 2
                            angle:      _heading
                        }]
                }
                //-- Heading
                Rectangle {
                    height:             mainIsMap ? ScreenTools.defaultFontPixelHeight * 0.75 : 0
                    width:              mainIsMap ? ScreenTools.defaultFontPixelWidth  * 3.5 : 0
                    radius:             ScreenTools.defaultFontPixelWidth  * 0.25
                    color:              qgcPal.windowShade
                    visible:            mainIsMap
                    anchors.bottom:         parent.bottom
                    anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * -0.25
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        text:               _heading
                        color:              qgcPal.text
                        font.pointSize:     ScreenTools.smallFontPointSize
                        anchors.centerIn:   parent
                    }
                }
            }
            //-- Second Row
            //-- Chronometer
            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                 "/custom/img/chronometer.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
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
                horizontalAlignment:    firstLabel.horizontalAlignment
            }
            //-- Ground Speed
            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                 "/custom/img/horizontal_speed.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
            }
            QGCLabel {
                text:                   activeVehicle ? activeVehicle.groundSpeed.value.toFixed(1) + ' ' + activeVehicle.groundSpeed.units : "0.0"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    firstLabel.horizontalAlignment
            }
            //-- Vertical Speed
            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                 "/custom/img/vertical_speed.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text

            }
            QGCLabel {
                text:                   activeVehicle ? activeVehicle.climbRate.value.toFixed(1) + ' ' + activeVehicle.climbRate.units : "0.0"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    firstLabel.horizontalAlignment
            }
            //-- Third Row
            //-- Odometer
            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                 "/custom/img/odometer.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text

            }
            QGCLabel {
                text:                   activeVehicle ? ('00000' + activeVehicle.flightDistance.value.toFixed(0)).slice(-5) + ' ' + activeVehicle.flightDistance.units : "00000"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    firstLabel.horizontalAlignment
            }
            //-- Altitude
            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                 "/custom/img/altitude.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text

            }
            QGCLabel {
                text:                   _altitude
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    firstLabel.horizontalAlignment
            }
            //-- Distance
            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                 "/custom/img/distance.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text

            }
            QGCLabel {
                text:                   _distance ? _distanceStr : noGPS
                color:                  _distance ? _indicatorsColor : qgcPal.colorOrange
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    firstLabel.horizontalAlignment
            }
        }
        MouseArea {
            anchors.fill:       parent
            onDoubleClicked:    CustomQuickInterface.showAttitudeWidget = !CustomQuickInterface.showAttitudeWidget
        }
    }
    //-------------------------------------------------------------------------
    //-- Attitude Indicator
    Rectangle {
        color:                  qgcPal.window
        width:                  attitudeIndicator.width * 0.5
        height:                 vehicleIndicator.height
        visible:                CustomQuickInterface.showAttitudeWidget
        anchors.top:            vehicleIndicator.top
        anchors.left:           vehicleIndicator.right
    }
    Rectangle {
        id:                     attitudeIndicator
        anchors.bottom:         vehicleIndicator.bottom
        anchors.bottomMargin:   ScreenTools.defaultFontPixelWidth * -0.5
        anchors.right:  parent.right
        anchors.rightMargin:  ScreenTools.defaultFontPixelWidth
        height:                 ScreenTools.defaultFontPixelHeight * 6
        width:                  height
        radius:                 height * 0.5
        color:                  qgcPal.windowShade
        visible:                CustomQuickInterface.showAttitudeWidget
            CustomAttitudeWidget {
            size:               parent.height * 0.95
            vehicle:            activeVehicle
                showHeading:        false
            anchors.centerIn:   parent
        }
    }
    //-------------------------------------------------------------------------
    //-- Multi Vehicle Selector
    Row {
        id:                     multiVehicleSelector
        spacing:                ScreenTools.defaultFontPixelWidth
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   ScreenTools.defaultFontPixelWidth * 1.5
        anchors.right:          vehicleIndicator.left
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        visible:                QGroundControl.multiVehicleManager.vehicles.count > 1
        Repeater {
            model:              QGroundControl.multiVehicleManager.vehicles.count
            CustomVehicleButton {
                property var _vehicle: QGroundControl.multiVehicleManager.vehicles.get(modelData)
                vehicle:        _vehicle
                checked:        (_vehicle && activeVehicle) ? _vehicle.id === activeVehicle.id : false
                onClicked: {
                    QGroundControl.multiVehicleManager.activeVehicle = _vehicle
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    //-- Gimbal Control
    Rectangle {
        id:                     gimbalControl
        visible:                camControlLoader.visible && CustomQuickInterface.showGimbalControl && _hasGimbal
        anchors.bottom:         camControlLoader.bottom
        anchors.right:          camControlLoader.left
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * (QGroundControl.videoManager.hasThermal ? -1 : 1)
        height:                 parent.width * 0.125
        width:                  height
        color:                  Qt.rgba(1,1,1,0.25)
        radius:                 width * 0.5

        property real _currentPitch:    0
        property real _currentYaw:      0
        property real time_last_seconds:0
        property real _lastHackedYaw:   0
        property real speedMultiplier:  5

        property real maxRate:          20
        property real exponentialFactor:0.6
        property real kPFactor:         3

        property real reportedYawDeg:   activeVehicle ? activeVehicle.gimbalYaw   : NaN
        property real reportedPitchDeg: activeVehicle ? activeVehicle.gimbalPitch : NaN

        Timer {
            interval:   100  //-- 10Hz
            running:    gimbalControl.visible && activeVehicle
            repeat:     true
            onTriggered: {
                if (activeVehicle) {
                    var yaw = gimbalControl._currentYaw
                    var oldYaw = yaw;
                    var pitch = gimbalControl._currentPitch
                    var oldPitch = pitch;
                    var pitch_stick = (stick.yAxis * 2.0 - 1.0)
                    if(_camera && _camera.vendor === "NextVision") {
                        var time_current_seconds = ((new Date()).getTime())/1000.0
                        if(gimbalControl.time_last_seconds === 0.0)
                            gimbalControl.time_last_seconds = time_current_seconds
                        var pitch_angle = gimbalControl._currentPitch
                        // Preparing stick input with exponential curve and maximum rate
                        var pitch_expo = (1 - gimbalControl.exponentialFactor) * pitch_stick + gimbalControl.exponentialFactor * pitch_stick * pitch_stick * pitch_stick
                        var pitch_rate = pitch_stick * gimbalControl.maxRate
                        var pitch_angle_reported = gimbalControl.reportedPitchDeg
                        // Integrate the angular rate to an angle time abstracted
                        pitch_angle += pitch_rate * (time_current_seconds - gimbalControl.time_last_seconds)
                        // Control the angle quicker by driving the gimbal internal angle controller into saturation
                        var pitch_angle_error = pitch_angle - pitch_angle_reported
                        pitch_angle_error = Math.round(pitch_angle_error)
                        var pitch_setpoint = pitch_angle + pitch_angle_error * gimbalControl.kPFactor
                        //console.info("error: " + pitch_angle_error + "; angle_state: " + pitch_angle)
                        pitch = pitch_setpoint
                        yaw += stick.xAxis * gimbalControl.speedMultiplier

                        yaw = clamp(yaw, -180, 180)
                        pitch = clamp(pitch, -90, 45)
                        pitch_angle = clamp(pitch_angle, -90, 45)

                        //console.info("P: " + pitch + "; Y: " + yaw)
                        activeVehicle.gimbalControlValue(pitch, yaw);
                        gimbalControl._currentYaw = yaw
                        gimbalControl._currentPitch = pitch_angle
                        gimbalControl.time_last_seconds = time_current_seconds
                    } else {
                        yaw += stick.xAxis * gimbalControl.speedMultiplier
                        var hackedYaw = yaw + (stick.xAxis * gimbalControl.speedMultiplier * 50)
                        pitch += pitch_stick * gimbalControl.speedMultiplier
                        hackedYaw = clamp(hackedYaw, -180, 180)
                        yaw = clamp(yaw, -180, 180)
                        pitch = clamp(pitch, -90, 90)
                        if(gimbalControl._lastHackedYaw !== hackedYaw || gimbalControl.hackedYaw !== oldYaw || pitch !== oldPitch) {
                            activeVehicle.gimbalControlValue(pitch, hackedYaw)
                            gimbalControl._lastHackedYaw = hackedYaw
                            gimbalControl._currentPitch = pitch
                            gimbalControl._currentYaw = yaw
                        }
                    }
                }
            }
            function clamp(num, min, max) {
                return Math.min(Math.max(num, min), max);
            }
        }
        JoystickThumbPad {
            id:                     stick
            anchors.fill:           parent
            lightColors:            qgcPal.globalTheme === QGCPalette.Light
            yAxisThrottle:          true
            yAxisThrottleCentered:  true
            xAxis:                  0
            yAxis:                  0.5
        }
    }
    //-------------------------------------------------------------------------
    //-- Object Avoidance
    Item {
        visible:                activeVehicle && activeVehicle.objectAvoidance.available && activeVehicle.objectAvoidance.enabled
        anchors.centerIn:       parent
        width:                  parent.width  * 0.5
        height:                 parent.height * 0.5
        Repeater {
            model:              activeVehicle && activeVehicle.objectAvoidance.gridSize > 0 ? activeVehicle.objectAvoidance.gridSize : []
            Rectangle {
                width:          ScreenTools.defaultFontPixelWidth
                height:         width
                radius:         width * 0.5
                color:          distance < 0.25 ? "red" : "orange"
                x:              (parent.width  * activeVehicle.objectAvoidance.grid(modelData).x) + (parent.width  * 0.5)
                y:              (parent.height * activeVehicle.objectAvoidance.grid(modelData).y) + (parent.height * 0.5)
                property real distance: activeVehicle.objectAvoidance.distance(modelData)
            }
        }
    }
    //-------------------------------------------------------------------------
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
