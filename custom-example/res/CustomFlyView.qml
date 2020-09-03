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

    property var    mapCenter:              QGroundControl.flightMapPosition

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
                var veh = QGroundControl.multiVehicleManager.lastKnownLocation;
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
        visible:                (!mainIsMap || videoOnSecondScreen) && _cameraPresent && _camera.paramComplete
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

    onMapCenterChanged: {
        mapCenterLabel.text = QGroundControl.positionToMGRSFormat(mapCenter)
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
                id:                     latLabelValue
                text:                   activeVehicle ? activeVehicle.gps.lat.value.toFixed(activeVehicle.gps.lat.decimalPlaces) : (QGroundControl.multiVehicleManager.lastKnownLocation.isValid ? QGroundControl.multiVehicleManager.lastKnownLocation.latitude : "-")
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
                id:                     lonLabelValue
                text:                   activeVehicle ? activeVehicle.gps.lon.value.toFixed(activeVehicle.gps.lon.decimalPlaces) : (QGroundControl.multiVehicleManager.lastKnownLocation.isValid ? QGroundControl.multiVehicleManager.lastKnownLocation.longitude : "-")
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    latLabelValue.horizontalAlignment
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
                id:                     hdopLabelValue
                text:                   activeVehicle ? activeVehicle.gps.hdop.value.toFixed(activeVehicle.gps.hdop.decimalPlaces) : "-"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    latLabelValue.horizontalAlignment
                visible:                vehicleIndicator._showGps
            }

            //-- Compass
            Item {
                Layout.rowSpan:         3
                Layout.column:          6
                Layout.minimumWidth:    parent.height * 1.25
                Layout.fillHeight:      true
                Layout.fillWidth:       true
                //-- Large circle
                Rectangle {
                    height:             parent.height
                    width:              height
                    radius:             height * 0.5
                    border.color:       qgcPal.text
                    border.width:       1
                    color:              Qt.rgba(0,0,0,0)
                    anchors.centerIn:   parent
                    visible:            true
                }
                //-- North Label
                Rectangle {
                    height:             ScreenTools.defaultFontPixelHeight * 0.75
                    width:              ScreenTools.defaultFontPixelWidth  * 2
                    radius:             ScreenTools.defaultFontPixelWidth  * 0.25
                    color:              qgcPal.windowShade
                    visible:            true
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
                    height:             parent.height * 0.75
                    width:              height
                    source:             "/custom/img/compass_needle.svg"
                    fillMode:           Image.PreserveAspectFit
                    visible:            true
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
                    height:             ScreenTools.defaultFontPixelHeight * 0.75
                    width:              ScreenTools.defaultFontPixelWidth  * 3.5
                    radius:             ScreenTools.defaultFontPixelWidth  * 0.25
                    color:              qgcPal.windowShade
                    visible:            true
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
                id:                     flightTimeLabelValue
                text: {
                        if(activeVehicle)
                            return secondsToHHMMSS(activeVehicle.getFact("flightTime").value)
                    return "00:00:00"
                }
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    latLabelValue.horizontalAlignment
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
                id:                     groundSpeedLabelValue
                text:                   activeVehicle ? activeVehicle.groundSpeed.value.toFixed(1) + ' ' + activeVehicle.groundSpeed.units : "0.0"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    latLabelValue.horizontalAlignment
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
                id:                     verticalSpeedLabelValue
                text:                   activeVehicle ? activeVehicle.climbRate.value.toFixed(1) + ' ' + activeVehicle.climbRate.units : "0.0"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    latLabelValue.horizontalAlignment
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
                id:                     odometerLabelValue
                text:                   activeVehicle ? ('00000' + activeVehicle.flightDistance.value.toFixed(0)).slice(-5) + ' ' + activeVehicle.flightDistance.units : "00000"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    latLabelValue.horizontalAlignment
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
                id:                     altitudeLabelValue
                text:                   _altitude
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    latLabelValue.horizontalAlignment
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
                id:                     distanceLabelValue
                text:                   _distance ? _distanceStr : noGPS
                color:                  _distance ? _indicatorsColor : qgcPal.colorOrange
                font.pointSize:         ScreenTools.smallFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                horizontalAlignment:    latLabelValue.horizontalAlignment
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
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
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
    //-- Flight Coordinates View
    Item {
        id:             _flightCoordinates
        z:              _mapAndVideo.z + 3
        width:          layoutRow.width
        height:         layoutRow.height
        anchors.left:   parent.left
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight
        anchors.bottom: parent.bottom
        visible:        QGroundControl.settingsManager.appSettings.displayMGRSCoordinates.rawValue

        property real _fontSize: ScreenTools.defaultFontPointSize * 0.75

        Connections {
            target: QGroundControl.qgcPositionManger
            onGcsPositionChanged: {
                if (activeVehicle && gcsPosition.latitude && Math.abs(gcsPosition.latitude)  > 0.001 && gcsPosition.longitude && Math.abs(gcsPosition.longitude)  > 0.001) {
                    var gcs = QtPositioning.coordinate(gcsPosition.latitude, gcsPosition.longitude)
                    gcsPositionLabel.text = QGroundControl.positionToMGRSFormat(gcs)
                } else {
                    gcsPositionLabel.text = ""
                }
            }
        }

        Rectangle {
            id:               coordinatesRectangle
            anchors.fill:     parent
            color:            qgcPal.window
            radius:           ScreenTools.defaultFontPixelWidth * 0.5

            Row {
                id: layoutRow
                spacing:          ScreenTools.defaultFontPixelWidth * 0.5
                anchors.centerIn: parent
                padding:          ScreenTools.defaultFontPixelWidth * 0.25

                QGCLabel {
                    text:                    "MGRS"
                    color:                   qgcPal.text
                    font.pointSize:          _flightCoordinates._fontSize
                }
                QGCColoredImage {
                     height:                 vehiclePositionLabel.contentHeight * 0.75
                     width:                  height
                     color:                  qgcPal.text
                     source:                 "/custom/img/map-center.svg"
                     anchors.verticalCenter: parent.verticalCenter
                     visible:                mapCenterLabel.text !== ""
                }
                QGCLabel {
                    id:                      mapCenterLabel
                    color:                   qgcPal.text
                    font.pointSize:          _flightCoordinates._fontSize
                }
                QGCColoredImage {
                     height:                 vehiclePositionLabel.contentHeight * 0.75
                     width:                  height
                     color:                  qgcPal.text
                     source:                 "/qmlimages/PaperPlane.svg"
                     anchors.verticalCenter: parent.verticalCenter
                     visible:                vehiclePositionLabel.text !== ""
                }
                QGCLabel {
                    id:                      vehiclePositionLabel
                    text:                    QGroundControl.multiVehicleManager.lastKnownLocation.isValid ? QGroundControl.positionToMGRSFormat(QGroundControl.multiVehicleManager.lastKnownLocation) : ""
                    color:                   qgcPal.text
                    font.pointSize:          _flightCoordinates._fontSize
                }
                QGCColoredImage {
                     height:                 vehiclePositionLabel.contentHeight * 0.75
                     width:                  height
                     color:                  qgcPal.text
                     source:                 "/custom/img/male-solid.svg"
                     anchors.verticalCenter: parent.verticalCenter
                     visible:                gcsPositionLabel.text !== ""
                }
                QGCLabel {
                    id:                      gcsPositionLabel
                    color:                   qgcPal.text
                    font.pointSize:          _flightCoordinates._fontSize
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Gimbal Control
    Rectangle {
        id:                     gimbalControl
        visible:                camControlLoader.visible && _hasGimbal && CustomQuickInterface.showGimbalControl && (CustomQuickInterface.enableNewGimbalControls || !CustomQuickInterface.useEmbeddedGimbal)
        anchors.bottom:         camControlLoader.bottom
        anchors.right:          camControlLoader.left
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * (QGroundControl.videoManager.hasThermal ? -1 : 1)
        height:                 parent.width * 0.125
        width:                  height
        color:                  Qt.rgba(1,1,1,0.25)
        radius:                 width * 0.5

        property real _mountPitch:       _hasGimbal ? activeVehicle.gimbalPitch : 0
        property real _mountYaw:         _hasGimbal ? activeVehicle.gimbalYaw : 0
        property real time_last_seconds:0

        property bool _centerGimbal: false
        property bool _haveJoystick: joystickManager.activeJoystick
        property real _joystickPitch: 0
        property bool _doJoystickPitchStep: false
        property real _joystickYaw: 0
        property bool _doJoystickYawStep: false

        property bool _doVirtualJoystickControl: false
        property bool _doJoystickPitchContinuous: false

        property real _cameraPitchControlAngleRange: 45

        property var    _flyViewSettings:           QGroundControl.settingsManager.flyViewSettings
        property real gimbalSpeedFactor: _flyViewSettings ? _flyViewSettings.gimbalSpeedFactor.rawValue : 2.5
        property real gimbalSliderSpeedFactor: _flyViewSettings ? _flyViewSettings.gimbalSliderSpeedFactor.rawValue : 0.1

        // PID global variables
        property real _KpPitch: _flyViewSettings ? _flyViewSettings.gimbalPitchKPFactor.rawValue : 1.0
        property real _KiPitch: _flyViewSettings ? _flyViewSettings.gimbalPitchKIFactor.rawValue : 1.0
        property real _KdPitch: _flyViewSettings ? _flyViewSettings.gimbalPitchKDFactor.rawValue : 1.0
        property real _KpYaw: _flyViewSettings ? _flyViewSettings.gimbalYawKPFactor.rawValue : 1.0
        property real _KiYaw: _flyViewSettings ? _flyViewSettings.gimbalYawKIFactor.rawValue : 1.0
        property real _KdYaw: _flyViewSettings ? _flyViewSettings.gimbalYawKDFactor.rawValue : 1.0

        // PID global variables
        property real _previousErrorPitch: 0.0
        property real _previousErrorYaw: 0.0
        property real _integralPitch: 0.0
        property real _integralYaw: 0.0
        property real _dt: 0.01

        property bool _firstTime: true

        // Old gimbal control
        property real speedMultiplier:  2.5
        property real _currentPitch:       _hasGimbal ? activeVehicle.gimbalPitch : 0
        property real _currentYaw:         _hasGimbal ? activeVehicle.gimbalYaw : 0
        property real _lastPitch:       _hasGimbal ? activeVehicle.gimbalPitch : 0
        property real _lastYaw:         _hasGimbal ? activeVehicle.gimbalYaw : 0

        // Loop to run gimbal controls at 10Hz. If no input is modified then nothing will happen.
        Timer {
            interval:   100  //-- 10Hz
            running:    _camera && _camera.paramComplete && activeVehicle && (CustomQuickInterface.useEmbeddedGimbal || CustomQuickInterface.showGimbalControl || gimbalControl._haveJoystick)
            repeat:     true

            function _pid(setpoint, mountFeedback, pitch) {
                var previousError = (pitch ? gimbalControl._previousErrorPitch : gimbalControl._previousErrorYaw)
                var error = setpoint - mountFeedback
                var integral = error * gimbalControl._dt

                var pout = error * (pitch ? gimbalControl._KpPitch : gimbalControl._KpYaw)
                var iout = integral * (pitch ? gimbalControl._KiPitch : gimbalControl._KiYaw)
                var dout = (error - previousError) * (pitch ? gimbalControl._KdPitch : gimbalControl._KdYaw)
                var inc = pout + iout + dout

                if(pitch){
                    gimbalControl._previousErrorPitch = error
                    gimbalControl._integralPitch = integral
                } else {
                    gimbalControl._previousErrorYaw = error
                    gimbalControl._integralYaw = integral
                }

                return inc
            }

            onTriggered: {
                var gimbalRateMode = true;
                // TODO: send DO_MOUNT_CONFIGURE every time we detect a new gimbal
                // if(gimbalControl._firstTime){
                //     activeVehicle.gimbalConfigure()
                //     gimbalControl._firstTime = false
                // }
                if (activeVehicle) {
                    // New camera controls with mount orientation feedback and optional PID controller to smoothen setpoints
                    if(CustomQuickInterface.enableNewGimbalControls){
                        var yaw = 0
                        var pitch = 0
                        var pitch_stick = 0
                        var yaw_stick = 0
                        var updateSetpoint = false

                        // Is the Gimbal virtual joystick not centered? Then do joystick control
                        gimbalControl._doVirtualJoystickControl = (stick.xAxis !== 0) && (stick.yAxis !== 0.5)

                        // Gimbal slider pitch control: if gimbal slider is loaded, enabled and pressed
                        if(camControlLoader.status === Loader.Ready && CustomQuickInterface.useEmbeddedGimbal && camControlLoader.item.joystickPitchActive) {
                            pitch_stick = camControlLoader.item.joystickPitchNormalized * gimbalControl._cameraPitchControlAngleRange;
                            gimbalRateMode = false;
                            updateSetpoint = true;
                        }
                        // Gimbal virtual joystick control: if virtual joystick is not centered
                        if(gimbalControl.visible && gimbalControl._doVirtualJoystickControl) {
                            pitch_stick = (stick.yAxis * 2.0 - 1.0);
                            yaw_stick = stick.xAxis;
                            gimbalRateMode = true;
                            updateSetpoint = true;
                        }
                        // Gimbal physical joystick control: if either continuous or step pitch/yaw is triggered via joystick button
                        if(gimbalControl._haveJoystick && (gimbalControl._doJoystickPitchContinuous || gimbalControl._doJoystickPitchStep || gimbalControl._doJoystickYawStep)) {
                            pitch_stick = gimbalControl._joystickPitch;
                            yaw_stick = gimbalControl._joystickYaw;
                            if(gimbalControl._doJoystickPitchStep) {
                                gimbalControl._joystickPitch = 0;
                                gimbalControl._doJoystickPitchStep = false;
                            }
                            if(gimbalControl._doJoystickYawStep) {
                                gimbalControl._joystickYaw = 0;
                                gimbalControl._doJoystickYawStep = false;
                            }
                            gimbalRateMode = true;
                            updateSetpoint = true;
                        }
                        // Center gimbal from physical joystick: reset setpoint
                        if(gimbalControl._centerGimbal) {
                            updateSetpoint = true;
                        }

                        // Send mavlink gimbal setpoint only if a variation is detected
                        if(updateSetpoint) {
                            // Center gimbal from physical joystick: reset setpoint
                            if(gimbalControl._centerGimbal) {
                                gimbalControl._centerGimbal = false
                                gimbalRateMode = false;
                                pitch = 0.0
                                yaw = 0.0
                            } else {
                                if(CustomQuickInterface.gimbalYawPidEnabled && gimbalRateMode){
                                    var yaw_setpoint = CustomQuickInterface.gimbalYawInverted ? gimbalControl._mountYaw - yaw_stick : gimbalControl._mountYaw + yaw_stick
                                    var yaw_inc = _pid(yaw_setpoint, gimbalControl._mountYaw, false)
                                } else {
                                    var yaw_inc = yaw_stick * (gimbalRateMode ? gimbalControl.gimbalSpeedFactor : 1.0) * (CustomQuickInterface.gimbalYawInverted ? -1.0 : 1.0)
                                }
                                yaw = gimbalControl._mountYaw + yaw_inc

                                if(CustomQuickInterface.gimbalPitchPidEnabled && gimbalRateMode){
                                    var pitch_setpoint = CustomQuickInterface.gimbalPitchInverted ? gimbalControl._mountPitch - pitch_stick : gimbalControl._mountPitch + pitch_stick
                                    var pitch_inc = _pid(pitch_setpoint, gimbalControl._mountPitch, true)
                                } else {
                                    var pitch_inc = pitch_stick * (gimbalRateMode ? gimbalControl.gimbalSpeedFactor : gimbalControl.gimbalSliderSpeedFactor) * (CustomQuickInterface.gimbalPitchInverted ? -1.0 : 1.0)
                                }
                                pitch = gimbalControl._mountPitch + pitch_inc
                            }

                            // Clamp setpoints to gimbal angle ranges
                            yaw = clamp(yaw, -180, 180)
                            pitch = clamp(pitch, -90, 90)
                            activeVehicle.gimbalControlValue(pitch, yaw)
                        }
                    // Old gimbal controls with no feedback from gimbal and only open loop angle control
                    } else {
                        if(gimbalControl._centerGimbal) {
                            gimbalControl._currentYaw = 0
                            gimbalControl._currentPitch = 0
                            gimbalControl._centerGimbal = false
                        }

                        var yaw = gimbalControl._currentYaw
                        var pitch = gimbalControl._currentPitch

                        var pitch_stick = 0
                        var yaw_stick = 0
                        if(gimbalControl.visible) {
                            pitch_stick = (stick.yAxis * 2.0 - 1.0);
                            yaw_stick = stick.xAxis;
                        }
                        else if(CustomQuickInterface.useEmbeddedGimbal && camControlLoader.status === Loader.Ready) {
                            pitch_stick = camControlLoader.item.joystickPitchNormalized * gimbalControl._cameraPitchControlAngleRange;
                            yaw_stick = stick.xAxis;
                            gimbalRateMode = false;
                        }
                        else if(gimbalControl._haveJoystick) {
                            pitch_stick = gimbalControl._joystickPitch;
                            yaw_stick = gimbalControl._joystickYaw;
                            if(gimbalControl._doJoystickPitchStep) {
                                gimbalControl._joystickPitch = 0;
                                gimbalControl._doJoystickPitchStep = false;
                            }
                            if(gimbalControl._doJoystickYawStep) {
                                gimbalControl._joystickYaw = 0;
                                gimbalControl._doJoystickYawStep = false;
                            }
                        }

                        yaw += yaw_stick * (gimbalRateMode ? gimbalControl.speedMultiplier : 1)
                        pitch += pitch_stick * (gimbalRateMode ? gimbalControl.speedMultiplier : 1)
                        yaw = clamp(yaw, -180, 180)
                        pitch = clamp(pitch, -90, 90)
                        if(Math.abs(yaw - gimbalControl._lastYaw) > 0.001 || Math.abs(pitch - gimbalControl._lastPitch) > 0.001) {
                            activeVehicle.gimbalControlValue(pitch, yaw)
                            // Break the initial bindings
                            gimbalControl._lastPitch = pitch;
                            gimbalControl._lastYaw = yaw;
                            if(gimbalRateMode) {
                                gimbalControl._currentPitch = pitch
                                gimbalControl._currentYaw = yaw
                            }
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

        // Physical joystick connections
        Connections {
            enabled: gimbalControl._haveJoystick
            target: joystickManager.activeJoystick
            onStartContinuousGimbalPitch: {
                gimbalControl._joystickPitch = direction
                gimbalControl._doJoystickPitchContinuous = true;
            }
            onStopContinuousGimbalPitch: {
                gimbalControl._joystickPitch = 0
                gimbalControl._doJoystickPitchContinuous = false;
            }
            onGimbalPitchStep: {
                gimbalControl._joystickPitch = direction;
                gimbalControl._doJoystickPitchStep = true;
            }
            onGimbalYawStep: {
                gimbalControl._joystickYaw = direction;
                gimbalControl._doJoystickYawStep = true;
            }
            onCenterGimbal: gimbalControl._centerGimbal = true;
        }

        Connections {
            enabled: camControlLoader.status === Loader.Ready && _hasGimbal && !CustomQuickInterface.enableNewGimbalControls
            target: camControlLoader.item
            onJoystickPitchActiveChanged: {
                if(camControlLoader.item.joystickPitchActive) {
                    gimbalControl._currentPitch = activeVehicle.gimbalPitch
                    gimbalControl._currentYaw = activeVehicle.gimbalYaw
                }
                else {
                    gimbalControl._currentPitch = gimbalControl._lastPitch
                    gimbalControl._currentYaw = gimbalControl._lastYaw
                }
            }
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
