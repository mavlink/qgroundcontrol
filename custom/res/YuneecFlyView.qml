/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.3
import QtQuick.Layouts      1.2
import QtQuick.Controls     1.2
import QtQuick.Dialogs      1.2
import QtPositioning        5.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0
import QGroundControl.CameraControl         1.0
import QGroundControl.FlightMap             1.0

import TyphoonHQuickInterface               1.0

Item {
    anchors.fill: parent

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property string scaleState:    "topMode"

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property real   _indicatorDiameter: ScreenTools.defaultFontPixelWidth * 16
    property real   _distance:          0.0
    property bool   _noSdCardMsgShown:  false
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.connectionLost : false
    property var    _camController:     TyphoonHQuickInterface.cameraControl
    property var    _sepColor:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.5) : Qt.rgba(1,1,1,0.5)
    property bool   _cameraAutoMode:    _camController ? _camController.aeMode === CameraControl.AE_MODE_AUTO : false;
    property bool   _cameraVideoMode:   _camController ? _camController.cameraMode === CameraControl.CAMERA_MODE_VIDEO : false
    property bool   _cameraPresent:     _camController && _camController.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED
    property bool   _noSdCard:          TyphoonHQuickInterface.cameraControl.sdTotal === 0
    property string _altitude:          _activeVehicle ? (isNaN(_activeVehicle.altitudeRelative.rawValue) ? "0.0" : _activeVehicle.altitudeRelative.rawValue.toFixed(1)) + _activeVehicle.altitudeRelative.units : "0.0m"
    property string _distanceStr:       isNaN(_distance) ? "0m" : _distance.toFixed(0) + (_activeVehicle ? _activeVehicle.altitudeRelative.units : "m")

    QGCLabel {
        id:             altitudeLabel
        text:           "000.0"+ (_activeVehicle ? _activeVehicle.altitudeRelative.units : "m")
        visible:        false
    }

    QGCLabel {
        id:             distanceLabel
        text:           "0000" + (_activeVehicle ? _activeVehicle.altitudeRelative.units : "m")
        visible:        false
    }

    QGCLabel {
        id:             speedText
        text:           "00.0" + (_activeVehicle ? _activeVehicle.groundSpeed.units : "m/s")
        font.family:    ScreenTools.demiboldFontFamily
        font.pointSize: ScreenTools.mediumFontPointSize
        visible:        false
    }

    //-- Position from Controller GPS (M4)
    Connections {
        target: TyphoonHQuickInterface
        onControllerLocationChanged: {
            if(_activeVehicle) {
                if(TyphoonHQuickInterface.latitude == 0.0 && TyphoonHQuickInterface.longitude == 0.0) {
                    _distance = 0.0
                } else {
                    var gcs = QtPositioning.coordinate(TyphoonHQuickInterface.latitude, TyphoonHQuickInterface.longitude, TyphoonHQuickInterface.altitude)
                    var veh = _activeVehicle.coordinate;
                    _distance = gcs.distanceTo(veh);
                    //-- Ignore absurd values
                    if(_distance > 99999)
                        _distance = 0;
                    if(_distance < 0)
                        _distance = 0;
                }
            }
        }
    }

    MessageDialog {
        id:                 connectionLostDisarmedDialog
        title:              qsTr("Communication Lost")
        text:               qsTr("Connection to vehicle has been lost and closed.")
        standardButtons:    StandardButton.Ok
        onAccepted:         connectionLostDisarmedDialog.close()
    }

    Timer {
        id: connectionTimer
        interval:  5000
        running:   false;
        repeat:    false;
        onTriggered: {
            //-- Vehicle is gone
            if(_activeVehicle) {
                if(!_activeVehicle.armed) {
                    //-- If it wasn't already set to auto-disconnect
                    if(!_activeVehicle.autoDisconnect) {
                        //-- Vehicle was not armed. Close connection and tell user.
                        _activeVehicle.disconnectInactiveVehicle()
                        connectionLostDisarmedDialog.open()
                    }
                } else {
                    //-- Vehicle was armed. Show doom dialog.
                    rootLoader.sourceComponent = connectionLostArmed
                    mainWindow.disableToolbar()
                }
            }
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle
        onConnectionLostChanged: {
            if(!_communicationLost) {
                //-- Communication regained
                connectionTimer.stop();
                rootLoader.sourceComponent = null
                mainWindow.enableToolbar()
            } else {
                if(_activeVehicle && !_activeVehicle.autoDisconnect) {
                    //-- Communication lost
                    connectionTimer.start();
                }
            }
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        onVehicleAdded: {
            //-- Reset No SD Card message.
            _noSdCardMsgShown = false;
        }
    }

    //-- Handle no MicroSD card loaded in camera
    Connections {
        target: TyphoonHQuickInterface.cameraControl
        onCameraModeChanged: {
            if(TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED) {
                if(!_noSdCardMsgShown && _noSdCard) {
                    rootLoader.sourceComponent = nosdcardComponent
                    _noSdCardMsgShown = true
                }
            }
        }
        onSdTotalChanged: {
            if(_noSdCard) {
                if(!_noSdCardMsgShown) {
                    rootLoader.sourceComponent = nosdcardComponent
                    _noSdCardMsgShown = true
                }
            } else {
                rootLoader.sourceComponent = null
            }
        }
    }

    //-- Camera Status
    Rectangle {
        width:          camRow.width + (ScreenTools.defaultFontPixelWidth * 2)
        height:         camRow.height * (_cameraVideoMode ? 1.25 : 1.5)
        color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0.15,1,0.15,0.85) : Qt.rgba(0,0.15,0,0.85)
        visible:        !_mainIsMap && _cameraPresent && indicatorDropdown.sourceComponent === null && !messageArea.visible && !criticalMmessageArea.visible
        radius:         3
        anchors.top:    parent.top
        anchors.topMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        Row {
            id: camRow
            spacing: ScreenTools.defaultFontPixelWidth
            anchors.centerIn: parent
            //-- AE
            QGCLabel { text: qsTr("AE:"); anchors.verticalCenter: parent.verticalCenter;}
            QGCLabel { text: _cameraAutoMode ? qsTr("Auto") : qsTr("Manual"); anchors.verticalCenter: parent.verticalCenter;}
            //-- EV
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode; }
            QGCLabel {
                text: qsTr("EV:");
                visible: _cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            QGCLabel {
                text: _camController ? ( _camController.currentEV < _camController.evList.length ? _camController.evList[_camController.currentEV] : "0") : "0"
                visible: _cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            //-- ISO
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: !_cameraAutoMode; }
            QGCLabel {
                text: qsTr("ISO:");
                visible: !_cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            QGCLabel {
                text: _camController ? _camController.isoList[_camController.currentIso] : "";
                visible: !_cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            //-- Shutter Speed
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; visible: !_cameraAutoMode; anchors.verticalCenter: parent.verticalCenter; }
            QGCLabel {
                text: qsTr("Shutter:");
                visible: !_cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            QGCLabel {
                text: _camController ? _camController.shutterList[_camController.currentShutter] : "";
                visible: !_cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            //-- WB
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; }
            QGCLabel { text: qsTr("WB:"); anchors.verticalCenter: parent.verticalCenter;}
            QGCLabel { text: _camController ? _camController.wbList[_camController.currentWB] : ""; anchors.verticalCenter: parent.verticalCenter; }
            //-- Metering
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode; }
            QGCLabel { text: qsTr("Metering:"); anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode; }
            QGCLabel { text: _camController ? _camController.meteringList[_camController.currentMetering] : ""; anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode; }
            //-- Video Res
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraVideoMode; }
            QGCLabel {
                text: _camController ? _camController.videoResList[_camController.currentVideoRes] : "";
                visible: _cameraVideoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            //-- SD Card
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; }
            QGCLabel { text: qsTr("SD:"); anchors.verticalCenter: parent.verticalCenter;}
            QGCLabel { text: _camController ? _camController.sdFreeStr : ""; anchors.verticalCenter: parent.verticalCenter; visible: !_noSdCard}
            QGCLabel { text: qsTr("NONE"); color: qgcPal.colorOrange; anchors.verticalCenter: parent.verticalCenter; visible: _noSdCard}
        }
    }

    //-- Camera Control
    Loader {
        visible:        !_mainIsMap
        source:         _mainIsMap ? "" : "/typhoonh/cameraControl.qml"
        anchors.right:  parent.right
        anchors.top:    parent.top
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 5
    }

    //-- Vehicle Status
    Rectangle {
        id:     vehicleStatus
        width:  vehicleStatusRow.width  + (ScreenTools.defaultFontPixelWidth * 4)
        height: vehicleStatusRow.height * 1.5
        color:  qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.8) : Qt.rgba(0,0,0,0.75)
        border.color: qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.25) : Qt.rgba(1,1,1,0.25)
        border.width: 1
        anchors.bottom: parent.bottom
        anchors.right:  parent.right
        anchors.rightMargin:  _indicatorDiameter * 0.5
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight
        Row {
            id:                 vehicleStatusRow
            spacing:            ScreenTools.defaultFontPixelWidth * 1.5
            anchors.centerIn:   parent
            QGCColoredImage {
                height:             ScreenTools.defaultFontPixelHeight * 1.5
                width:              height
                sourceSize.height:  height
                source:             "/typhoonh/img/height.svg"
                fillMode:           Image.PreserveAspectFit
                color:              qgcPal.colorBlue
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:               _altitude
                width:              altitudeLabel.width
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCColoredImage {
                height:             ScreenTools.defaultFontPixelHeight * 1.5
                width:              height
                sourceSize.height:  height
                source:             "/typhoonh/img/distance.svg"
                fillMode:           Image.PreserveAspectFit
                color:              qgcPal.colorBlue
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:               _distanceStr
                width:              distanceLabel.width
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCColoredImage {
                height:             ScreenTools.defaultFontPixelHeight * 1.5
                width:              height
                sourceSize.height:  height
                source:             "/typhoonh/img/speed.svg"
                fillMode:           Image.PreserveAspectFit
                color:              qgcPal.colorBlue
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:           qsTr("H:")
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:           _activeVehicle ? _activeVehicle.groundSpeed.rawValue.toFixed(1) + _activeVehicle.groundSpeed.units : "0.0"
                width:          speedText.width
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:           qsTr("V:")
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:           _activeVehicle ? _activeVehicle.climbRate.rawValue.toFixed(1) + _activeVehicle.climbRate.units : "0.0"
                width:          speedText.width
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCColoredImage {
                height:             ScreenTools.defaultFontPixelHeight * 1.5
                width:              height
                sourceSize.height:  height
                source:             "/typhoonh/img/time.svg"
                fillMode:           Image.PreserveAspectFit
                color:              qgcPal.colorBlue
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:           _activeVehicle ? TyphoonHQuickInterface.flightTime : "00:00:00"
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:           _activeVehicle ? ('00000' + _activeVehicle.flightDistance.rawValue.toFixed(0)).slice(-5) : "00000"  + (_activeVehicle ? _activeVehicle.altitudeRelative.units : "m")
                anchors.verticalCenter: parent.verticalCenter
            }
            Item {
                id:             vehicleStatusExtRect
                width:          _indicatorDiameter * 0.5
                height:         1
            }
        }
    }

    //-- Indicator thingy
    Item {
        id:             compassAttitudeCombo
        width:          _indicatorDiameter
        height:         outerCompass.height
        anchors.bottom: vehicleStatus.bottom
        anchors.right:  parent.right
        anchors.rightMargin:  ScreenTools.defaultFontPixelWidth
        CompassRing {
            id:                 outerCompass
            size:               parent.width * 1.05
            vehicle:            _activeVehicle
            anchors.horizontalCenter: parent.horizontalCenter
        }
        QGCAttitudeWidget {
            id:                 attitudeWidget
            size:               parent.width * 0.85
            vehicle:            _activeVehicle
            anchors.centerIn:   outerCompass
            showHeading:        true
        }
    }

    //-- No SD Card In Camera
    Component {
        id:             nosdcardComponent
        Item {
            id:         nosdcardComponentItem
            width:      mainWindow.width
            height:     mainWindow.height
            z:          1000000
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Rectangle {
                id:     nosdRect
                width:  mainWindow.width   * 0.65
                height: nosdcardCol.height * 1.5
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 nosdcardCol
                    width:              nosdRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 3
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("No MicroSD Card in Camera")
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           qsTr("No images will be captured or videos recorded.")
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCButton {
                        text:           qsTr("Close")
                        width:          ScreenTools.defaultFontPixelWidth  * 10
                        height:         ScreenTools.defaultFontPixelHeight * 2
                        onClicked:      rootLoader.sourceComponent = null
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = nosdcardComponentItem.width
                rootLoader.height = nosdcardComponentItem.height
            }
        }
    }

    //-- Connection Lost While Armed
    Component {
        id:         connectionLostArmed
        Item {
            id:         connectionLostArmedItem
            z:          1000000
            width:      mainWindow.width
            height:     mainWindow.height
            Rectangle {
                id:     connectionLostArmedRect
                width:  mainWindow.width   * 0.65
                height: connectionLostArmedCol.height * 1.5
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 connectionLostArmedCol
                    width:              connectionLostArmedRect.width
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
                        width:          connectionLostArmedRect.width * 0.75
                        wrapMode:       Text.WordWrap
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Component.onCompleted: {
                rootLoader.width  = connectionLostArmedItem.width
                rootLoader.height = connectionLostArmedItem.height
            }
        }
    }

}
