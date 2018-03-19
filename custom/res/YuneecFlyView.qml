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
import QtGraphicalEffects   1.0

import QGroundControl                       1.0
import QGroundControl.Controllers           1.0
import QGroundControl.Controls              1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Vehicle               1.0

import TyphoonHQuickInterface               1.0
import TyphoonHQuickInterface.Widgets       1.0

Item {
    anchors.fill: parent
    visible:    !QGroundControl.videoManager.fullScreen

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property string scaleState:    "topMode"
    readonly property string noGPS:         qsTr("NO GPS")

    property real   _indicatorDiameter: ScreenTools.defaultFontPixelWidth * 16
    property var    _sepColor:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.5) : Qt.rgba(1,1,1,0.5)
    property color  _indicatorColor:    qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.connectionLost : false
    property var    _dynamicCameras:    _activeVehicle ? _activeVehicle.dynamicCameras : null
    property bool   _isCamera:          _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _camera:            _isCamera ? _dynamicCameras.cameras.get(0) : null // Single camera support for the time being
    property bool   _cameraVideoMode:   _camera ?  _camera.cameraMode === QGCCameraControl.CAM_MODE_VIDEO : false
    property bool   _cameraPhotoMode:   _camera ?  (_camera.cameraMode === QGCCameraControl.CAM_MODE_PHOTO || _camera.cameraMode === QGCCameraControl.CAM_MODE_SURVEY) : false
    property bool   _cameraPresent:     _camera && _camera.cameraMode !== QGCCameraControl.CAM_MODE_UNDEFINED
    property bool   _noSdCard:          _camera && _camera.storageTotal === 0
    property bool   _fullSD:            _camera && _camera.storageTotal !== 0 && _camera.storageFree > 0 && _camera.storageFree < 250 // We get kiB from the camera
    property bool   _isVehicleGps:      _activeVehicle && _activeVehicle.gps && _activeVehicle.gps.count.rawValue > 1 && activeVehicle.gps.hdop.rawValue < 1.4
    property bool   _recordingVideo:    _cameraVideoMode && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING
    property bool   _cameraIdle:        !_cameraPhotoMode || _camera.photoStatus === QGCCameraControl.PHOTO_CAPTURE_IDLE

    property var    _expModeFact:       _camera && _camera.exposureMode
    property var    _evFact:            _camera && _camera.ev
    property var    _isoFact:           _camera && _camera.iso
    property var    _shutterFact:       _camera && _camera.shutterSpeed
    property var    _wbFact:            _camera && _camera.wb
    property var    _meteringFact:      _camera && _camera.meteringMode
    property var    _videoResFact:      _camera && _camera.videoRes
    property var    _irPaletteFact:     _camera && _camera.irPalette
    property var    _isThermal:         _camera && _camera.isThermal
    property var    _isCGOET:           _camera && _camera.isCGOET

    property bool   _cameraAutoMode:    _expModeFact  ? _expModeFact.rawValue === 0 : true
    property string _altitude:          _activeVehicle   ? (isNaN(_activeVehicle.altitudeRelative.value) ? "0.0" : _activeVehicle.altitudeRelative.value.toFixed(1)) + ' ' + _activeVehicle.altitudeRelative.units : "0.0"
    property string _distanceStr:       isNaN(_distance) ? "0" : _distance.toFixed(0) + ' ' + (_activeVehicle ? _activeVehicle.altitudeRelative.units : "")
    property real   _heading:           _activeVehicle   ? _activeVehicle.heading.rawValue : 0
    property bool   _st16GPS:           false
    property real   _gimbalPitch:       _camera ? -_camera.gimbalPitch : 0
    property real   _gimbalYaw:         _camera ? -_camera.gimbalYaw : 0
    property bool   _gimbalVisible:     _camera ? _camera.gimbalData && camControlLoader.visible : false

    property real   _distance:              0.0
    property bool   _noSdCardMsgShown:      false
    property bool   _fullSdCardMsgShown:    false
    property bool   _showAttitude:          false
    property int    _eggCount:              0
    property string _messageTitle:          ""
    property string _messageText:           ""
    property bool   _formatComplete:        false

    function showSimpleAlert(title, message) {
        _messageTitle   = title;
        _messageText    = message;
        rootLoader.sourceComponent = simpleAlert
    }

    function showNoSDCardMessage() {
        showSimpleAlert(
            qsTr("No MicroSD Card in Camera"),
            qsTr("No images will be captured or videos recorded."))
    }

    function showFullSDCardMessage() {
        showSimpleAlert(
            qsTr("MicroSD Card in Camera is Full"),
            qsTr("No images will be captured or videos recorded."))
    }

    function indicatorClicked() {
        var count = _showAttitude ? 5 : 15
        vehicleStatus.visible = !vehicleStatus.visible
        _eggCount++
        toggleTimer.restart()
        if (_eggCount === count) {
            vehicleStatus.visible = true
            _showAttitude = !_showAttitude
        }
    }

    Timer {
        id:         ssidChanged
        interval:   5000
        running:    false;
        repeat:     false;
        onTriggered: {
            //-- If connected to something other than a camera
            if(ScreenTools.isAndroid && TyphoonHQuickInterface.connectedCamera === "" && TyphoonHQuickInterface.connectedSSID !== "") {
                //-- If we haven't done it already
                if(TyphoonHQuickInterface.wifiAlertEnabled) {
                    showSimpleAlert(
                        qsTr("Connected to Standard Wi-Fi"),
                        qsTr("The ST16 is connected to a standard Wi-Fi and not a vehicle."))
                }
            }
        }
    }

    Timer {
        id:         toggleTimer
        interval:   500
        onTriggered: {
            toggleTimer.stop()
            parent._eggCount = 0
        }
    }

    Timer {
        id:        connectionTimer
        interval:  5000
        running:   false;
        repeat:    false;
        onTriggered: {
            //-- Vehicle is gone
            if(_activeVehicle) {
                //-- Let video stream close
                QGroundControl.settingsManager.videoSettings.rtspTimeout.rawValue = 1
                if(!_activeVehicle.armed) {
                    //-- If it wasn't already set to auto-disconnect
                    if(!_activeVehicle.autoDisconnect) {
                        if (guidedController.showResumeMission) {
                            qgcView.showDialog(connectionLostBatteryResume, qsTr("Connection lost"), qgcView.showDialogDefaultWidth, StandardButton.Close)
                        } else {
                            //-- Vehicle is not armed. Close connection and tell user.
                            _activeVehicle.disconnectInactiveVehicle()
                            connectionLostDisarmedDialog.open()
                        }
                    }
                } else {
                    //-- Vehicle is armed. Show doom dialog.
                    rootLoader.sourceComponent = connectionLostArmed
                    mainWindow.disableToolbar()
                }
            }
        }
    }

    Connections {
        target: TyphoonHQuickInterface.mobileSync
        onDesktopConnectedChanged: {
            if(TyphoonHQuickInterface.mobileSync.desktopConnected) {
                if(rootLoader.sourceComponent !== desktopConnectedDlg) {
                    rootLoader.sourceComponent = desktopConnectedDlg
                    mainWindow.disableToolbar()
                }
            } else {
                rootLoader.sourceComponent = null
                mainWindow.enableToolbar()
            }
        }
    }

    Connections {
        target: TyphoonHQuickInterface
        //-- Position from Controller GPS (M4)
        onControllerLocationChanged: {
            if(_activeVehicle) {
                if(TyphoonHQuickInterface.latitude == 0.0 && TyphoonHQuickInterface.longitude == 0.0) {
                    //-- Unless you really are in the middle of the Atlantic, this means we don't have location.
                    _distance = 0.0
                    _st16GPS = false
                } else {
                    var gcs = QtPositioning.coordinate(TyphoonHQuickInterface.latitude, TyphoonHQuickInterface.longitude, TyphoonHQuickInterface.altitude)
                    var veh = _activeVehicle.coordinate;
                    _distance = QGroundControl.metersToAppSettingsDistanceUnits(gcs.distanceTo(veh));
                    //-- Ignore absurd values
                    if(_distance > 99999)
                        _distance = 0;
                    if(_distance < 0)
                        _distance = 0;
                    _st16GPS = true
                }
            }
        }
        //-- Who are we connected to
        onConnectedSSIDChanged: {
            ssidChanged.start()
        }
        //-- Big Red Button down for > 1 second
        onPowerHeld: {
            if(_activeVehicle) {
                rootLoader.sourceComponent = panicDialog
                mainWindow.disableToolbar()
            }
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle
        onConnectionLostChanged: {
            if(!_communicationLost) {
                //-- Communication regained
                connectionTimer.stop();
                mainWindow.enableToolbar()
                rootLoader.sourceComponent = null
                //-- Reset stream timeout
                QGroundControl.settingsManager.videoSettings.rtspTimeout.rawValue = 60
            } else {
                if(_activeVehicle && !_activeVehicle.autoDisconnect) {
                    //-- Communication lost
                    connectionTimer.start();
                }
            }
        }
        onFirmwareCustomVersionChanged: {
            if(_activeVehicle && !_activeVehicle.armed) {
                if(TyphoonHQuickInterface.shouldWeShowUpdate()) {
                    if(rootLoader.sourceComponent !== initialSettingsDialog) {
                        rootLoader.sourceComponent = updateDialog
                    }
                }
            }
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        onVehicleAdded: {
            //-- Reset No SD Card message.
            _noSdCardMsgShown = false;
            //-- And comm lost dialog if open
            connectionLostDisarmedDialog.close()
        }
        onParameterReadyVehicleAvailableChanged: {
            if(QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable) {
                if(_activeVehicle && !_activeVehicle.armed) {
                    //-- Check for first run
                    if(TyphoonHQuickInterface.firstRun) {
                        rootLoader.sourceComponent = initialSettingsDialog
                    //-- Check if we should update
                    } else if(TyphoonHQuickInterface.shouldWeShowUpdate()) {
                        rootLoader.sourceComponent = updateDialog
                    }
                }
            }
        }
    }

    //-- Handle MicroSD card loaded in camera
    Connections {
        target: _camera
        onStorageTotalChanged: {
            if(_noSdCard) {
                if(!_noSdCardMsgShown) {
                    showNoSDCardMessage();
                    _noSdCardMsgShown = true;
                }
            } else {
                _noSdCardMsgShown = false;
                if(rootLoader.sourceComponent === simpleAlert) {
                    mainWindow.enableToolbar()
                    rootLoader.sourceComponent = null
                }
                if(camControlLoader.item && camControlLoader.item.formatInProgress) {
                    _formatComplete = true
                    showSimpleAlert(
                        qsTr("Format MicroSD Card"),
                        qsTr("Format Completed"))
                }
            }
            //-- Anything will reset this
            if(camControlLoader.item) {
                camControlLoader.item.formatInProgress = false
            }
        }
        onStorageFreeChanged: {
            if(_fullSD) {
                if(!_fullSdCardMsgShown) {
                    showFullSDCardMessage();
                    _fullSdCardMsgShown = true;
                }
            } else {
                _fullSdCardMsgShown = false;
                if(rootLoader.sourceComponent === simpleAlert && !_formatComplete) {
                    mainWindow.enableToolbar()
                    rootLoader.sourceComponent = null
                }
            }
            _formatComplete = false
        }
    }

    Component.onCompleted: {
        if(ScreenTools.isAndroid && TyphoonHQuickInterface.connectedCamera === "" && TyphoonHQuickInterface.connectedSSID !== "") {
            ssidChanged.start()
        }
    }

    Component {
        id: connectionLostBatteryResume

        QGCViewDialog {
            function reject() {
                _activeVehicle.disconnectInactiveVehicle()
                hideDialog()
            }

            Connections {
                target: QGroundControl.multiVehicleManager.activeVehicle
                onConnectionLostChanged: {
                    if (!connectionLost) {
                        hideDialog()
                    }
                }
            }

            Column {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        ScreenTools.defaultFontPixelHeight / 2

                QGCLabel {
                    text:           qsTr("Connection to the vehicle has been lost.")
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                }

                QGCLabel {
                    text:           qsTr("If you are in the process of switching batteries to continue the mission, this dialog will go away automatically once the vehicle restarts.")
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                }

                QGCLabel {
                    text:           qsTr("Otherwise you can click Close to disconnect from the vehicle.")
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                }
            }
        }
    }

    MessageDialog {
        id:                 connectionLostDisarmedDialog
        title:              TyphoonHQuickInterface.newPasswordSet ? qsTr("Password Changed") : qsTr("Communication Lost")
        text:               TyphoonHQuickInterface.newPasswordSet ? qsTr("Please power cycle the vehicle for the new password to take effect.") : qsTr("Connection to vehicle has been lost and closed.")
        standardButtons:    StandardButton.Ok
        onAccepted: {
            TyphoonHQuickInterface.newPasswordSet = false
            connectionLostDisarmedDialog.close()
        }
    }

    //-- Camera Status
    Rectangle {
        id:             camStatus
        width:          camRow.width + (ScreenTools.defaultFontPixelWidth * 3)
        height:         camRow.height * 1.25
        color:          _indicatorColor
        visible:        !_mainIsMap && _cameraPresent && indicatorDropdown.sourceComponent === null && !messageArea.visible && !criticalMmessageArea.visible && _camera.paramComplete
        radius:         3
        border.width:   1
        border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
        anchors.top:    parent.top
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.5
        anchors.horizontalCenter: parent.horizontalCenter
        Row {
            id: camRow
            spacing: ScreenTools.defaultFontPixelWidth
            anchors.centerIn: parent
            //-- AE
            QGCLabel { text: qsTr("AE:"); anchors.verticalCenter: parent.verticalCenter; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                indexModel: false
                fact:       _expModeFact
                enabled:    _cameraIdle
            }
            //-- EV
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode; }
            QGCLabel { text: qsTr("EV:"); visible: _cameraAutoMode && !_isThermal; anchors.verticalCenter: parent.verticalCenter; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                visible: _cameraAutoMode && !_isThermal;
                indexModel: false
                fact:       _evFact
                enabled:    _cameraIdle
            }
            //-- ISO
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: !_cameraAutoMode; }
            QGCLabel { text: qsTr("ISO:"); visible: !_cameraAutoMode; anchors.verticalCenter: parent.verticalCenter; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                visible:    !_cameraAutoMode;
                indexModel: false
                fact:       _isoFact
                enabled:    _cameraIdle
            }
            //-- Shutter Speed
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; visible: !_cameraAutoMode; anchors.verticalCenter: parent.verticalCenter; }
            QGCLabel {text: qsTr("Shutter:"); visible: !_cameraAutoMode; anchors.verticalCenter: parent.verticalCenter; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                visible:    !_cameraAutoMode;
                indexModel: false
                fact:       _shutterFact
                enabled:    _cameraIdle
            }
            //-- WB
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: !_isThermal; }
            QGCLabel { text: qsTr("WB:"); anchors.verticalCenter: parent.verticalCenter; visible: !_isThermal; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                indexModel: false
                fact:       _wbFact
                enabled:    _cameraIdle
                visible:    !_isThermal
            }
            //-- Metering
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode && !_isThermal; }
            QGCLabel { text: qsTr("Metering:"); anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode && !_isThermal; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                visible:    _cameraAutoMode && !_isThermal;
                indexModel: false
                fact:       _meteringFact
                enabled:    _cameraIdle
            }
            //-- Video Res
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraVideoMode && !_isThermal && !_noSdCard; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                visible:    _cameraVideoMode && !_isThermal && !_noSdCard;
                indexModel: false
                enabled:    !_recordingVideo
                fact:       _videoResFact
            }
            //-- CGOET/E10T Palette
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: !_cameraAutoMode && _isThermal; }
            QGCLabel { text: qsTr("Palette:"); anchors.verticalCenter: parent.verticalCenter; visible: _isThermal; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                visible:    _isThermal;
                indexModel: false
                fact:       _irPaletteFact
            }
            //-- CGOET/E10T ROI
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _isThermal; }
            QGCLabel { text: qsTr("ROI:"); anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode && _isThermal; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                visible:    _isThermal;
                indexModel: false
                fact:       _camera ? _camera.irROI : null
            }
            //-- CGOET Presets
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _isCGOET; }
            QGCLabel { text: qsTr("Presets:"); anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode && _isCGOET; }
            CameraMenu {
                anchors.verticalCenter: parent.verticalCenter
                visible:    _isCGOET;
                indexModel: false
                fact:       _camera ? _camera.irPresets : null
            }
            //-- SD Card
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; }
            QGCLabel { text: qsTr("SD:"); anchors.verticalCenter: parent.verticalCenter;}
            QGCLabel { text: _camera ? _camera.storageFreeStr : ""; anchors.verticalCenter: parent.verticalCenter; visible: !_noSdCard && !_fullSD}
            QGCLabel { text: qsTr("NONE"); color: qgcPal.colorOrange; anchors.verticalCenter: parent.verticalCenter; visible: _noSdCard}
            QGCLabel { text: qsTr("FULL"); color: qgcPal.colorOrange; anchors.verticalCenter: parent.verticalCenter; visible: _fullSD}
        }
    }

    //-- OBS
    Item {
        id:             obdIndicator
        width:          ScreenTools.defaultFontPixelWidth  * 34
        height:         width * 0.5
        visible:        !_mainIsMap && TyphoonHQuickInterface.obsState && TyphoonHQuickInterface.distSensorMax !== 0 && obdIndicator.distCur < 1.0 && !messageArea.visible && !criticalMmessageArea.visible
        //visible:        !_mainIsMap && TyphoonHQuickInterface.distSensorMax !== 0 && obdIndicator.distCur < 1.0 && !messageArea.visible && !criticalMmessageArea.visible
        anchors.top:    camStatus.bottom
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.5
        anchors.horizontalCenter: parent.horizontalCenter
        Image {
            anchors.fill:       parent
            source:             "/typhoonh/img/obsArc.svg"
            fillMode:           Image.Stretch
            sourceSize.width:   width
            //color:              obdIndicator.distCur > 0.75 ? qgcPal.colorGreen : (obdIndicator.distCur > 0.25 ? qgcPal.colorOrange : qgcPal.colorRed)
        }
        Rectangle {
            id:             obsRect
            height:         ScreenTools.defaultFontPixelWidth  * 10
            width:          ScreenTools.defaultFontPixelHeight * 2
            anchors.top:    parent.top
            anchors.topMargin: ScreenTools.defaultFontPixelHeight * 3
            anchors.horizontalCenter: parent.horizontalCenter
            gradient: Gradient {
                GradientStop { position: 0;     color: Qt.rgba(0.5, 0, 0, 0.25) }
                GradientStop { position: 0.25;  color: Qt.rgba(0.5, 0, 0, 1) }
                GradientStop { position: 0.75;  color: Qt.rgba(0.5, 0, 0, 1) }
                GradientStop { position: 1;     color: Qt.rgba(0.5, 0, 0, 0.25) }
            }
            rotation: 90
        }
        QGCLabel {
            text:               obdIndicator.distValue.toFixed(1) + (_activeVehicle ? _activeVehicle.flightDistance.units : "")
            color:              "white"
            font.family:        ScreenTools.demiboldFontFamily
            anchors.centerIn:   obsRect
        }
        property real distCur: TyphoonHQuickInterface.distSensorMax ? TyphoonHQuickInterface.distSensorCur / TyphoonHQuickInterface.distSensorMax : 0
        property real distValue: QGroundControl.metersToAppSettingsDistanceUnits(TyphoonHQuickInterface.distSensorCur / 100);
    }

    //-- Camera Control
    Loader {
        id:                     camControlLoader
        visible:                !_mainIsMap && rootLoader.sourceComponent !== initialSettingsDialog
        source:                 _mainIsMap ? "" : "/typhoonh/cameraControl.qml"
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.top:            parent.top
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight
    }

    //-- Gimbal Indicator
    Item {
        id:                     gimbalIndicator
        width:                  gimbalCol.width
        height:                 gimbalCol.height
        visible:                _gimbalVisible
        anchors.bottom:         camControlLoader.bottom
        anchors.right:          camControlLoader.left
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        Column {
            id:                 gimbalCol
            spacing:            ScreenTools.defaultFontPixelHeight * 0.25
            anchors.horizontalCenter: parent.horizontalCenter
            Rectangle {
                width:          ScreenTools.defaultFontPixelWidth * 4
                height:         camControlLoader.height * 0.65
                color:          _indicatorColor
                radius:         ScreenTools.defaultFontPixelWidth * 0.5
                border.width:   1
                border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
                Image {
                    id:                 pitchScale
                    height:             parent.height * 0.9
                    anchors.centerIn:   parent
                    source:             "/typhoonh/img/gimbalPitch.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    smooth:             true
                    mipmap:             true
                    Image {
                        id:                 yawIndicator
                        width:              ScreenTools.defaultFontPixelWidth * 3
                        source:             "/typhoonh/img/gimbalYaw.svg"
                        fillMode:           Image.PreserveAspectFit
                        sourceSize.width:   width
                        y:                  (parent.height * _pitch / 105) + (parent.height * 0.15) - (ScreenTools.defaultFontPixelWidth * 1.5)
                        smooth:             true
                        mipmap:             true
                        anchors.horizontalCenter: parent.horizontalCenter
                        transform: Rotation {
                            origin.x:       yawIndicator.width  / 2
                            origin.y:       yawIndicator.height / 2
                            angle:          _gimbalYaw
                        }
                        property real _pitch: _gimbalPitch < -15 ? -15  : (_gimbalPitch > 90 ? 90 : _gimbalPitch)
                    }
                }
            }
            Rectangle {
                width:              ScreenTools.defaultFontPixelWidth * 4
                height:             gimbalLabel.height * 1.5
                color:              _indicatorColor
                radius:             ScreenTools.defaultFontPixelWidth * 0.5
                border.width:       1
                border.color:       qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    id:             gimbalLabel
                    text:           _gimbalPitch ? _gimbalPitch.toFixed(0) : 0
                    color:          qgcPal.text
                    anchors.centerIn: parent
                }
            }
        }
    }

    //-- Vehicle Status
    Rectangle {
        id:     vehicleStatus
        width:  vehicleStatusGrid.width  + (ScreenTools.defaultFontPixelWidth * 4)
        height: vehicleStatusGrid.height + ScreenTools.defaultFontPixelHeight * 0.5
        radius: ScreenTools.defaultFontPixelWidth * 0.5
        color:  qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
        border.width:   1
        border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
        visible: true
        anchors.bottom: parent.bottom
        anchors.right:  parent.right
        anchors.rightMargin:  _indicatorDiameter * 0.5
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight
        GridLayout {
            id:                 vehicleStatusGrid
            columnSpacing:      ScreenTools.defaultFontPixelWidth  * 1.5
            rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.25
            columns:            5
            anchors.centerIn:   parent
            //-- Odometer
            QGCColoredImage {
                height:             ScreenTools.defaultFontPixelHeight
                width:              height
                sourceSize.height:  height
                source:             "/typhoonh/img/odometer.svg"
                fillMode:           Image.PreserveAspectFit
                color:              qgcPal.colorBlue
            }
            QGCLabel {
                text:   _activeVehicle ? ('00000' + _activeVehicle.flightDistance.value.toFixed(0)).slice(-5) + ' ' + _activeVehicle.flightDistance.units : "00000"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
            //-- Chronometer
            QGCColoredImage {
                height:             ScreenTools.defaultFontPixelHeight
                width:              height
                sourceSize.height:  height
                source:             "/typhoonh/img/time.svg"
                fillMode:           Image.PreserveAspectFit
                color:              qgcPal.colorBlue
            }
            QGCLabel {
                text:   _activeVehicle ? TyphoonHQuickInterface.flightTime : "00:00:00"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
            Item { width: 1; height: 1; }
            //-- Separator
            Rectangle {
                height: 1
                width:  parent.width
                color:  qgcPal.globalTheme === QGCPalette.Dark ? Qt.rgba(1,1,1,0.25) : Qt.rgba(0,0,0,0.25)
                Layout.fillWidth:  true
                Layout.columnSpan: 5
            }
            //-- Latitude
            QGCLabel {
                text:       qsTr("Lat:")
            }
            QGCLabel {
                text:       _isVehicleGps ? _activeVehicle.latitude.toFixed(6) : noGPS
                color:      _isVehicleGps ? qgcPal.text : qgcPal.colorOrange
                Layout.fillWidth:   true
                horizontalAlignment: _isVehicleGps ? Text.AlignRight : Text.AlignHCenter
            }
            //-- Longitude
            QGCLabel {
                text:       qsTr("Lon:")
            }
            QGCLabel {
                text:       _isVehicleGps ? _activeVehicle.longitude.toFixed(6) : noGPS
                color:      _isVehicleGps ? qgcPal.text : qgcPal.colorOrange
                Layout.fillWidth:   true
                horizontalAlignment: _isVehicleGps ? Text.AlignRight : Text.AlignHCenter
            }
            Item { width: 1; height: 1; }
            //-- Altitude
            QGCLabel {
                text:       qsTr("H:")
                visible:    _showAttitude
            }
            QGCLabel {
                text:       _altitude
                visible:    _showAttitude
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
            //-- Ground Speed
            QGCLabel {
                text:       qsTr("H.S:")
                visible:    _showAttitude
            }
            QGCLabel {
                text:       _activeVehicle ? _activeVehicle.groundSpeed.rawValue.toFixed(1) + ' ' + _activeVehicle.groundSpeed.units : "0.0"
                visible:    _showAttitude
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
            Item { width: 1; height: 1; visible: _showAttitude; }
            //-- Distance
            QGCLabel {
                text:   qsTr("D:")
            }
            QGCLabel {
                text:               _st16GPS ? _distanceStr : noGPS
                color:              _st16GPS ? qgcPal.text : qgcPal.colorOrange
                Layout.fillWidth:   true
                horizontalAlignment: _st16GPS ? Text.AlignRight : Text.AlignHCenter
            }
            //-- Vertical Speed
            QGCLabel {
                text:   qsTr("V.S:")
            }
            QGCLabel {
                text:           _activeVehicle ? _activeVehicle.climbRate.value.toFixed(1) + ' ' + _activeVehicle.climbRate.units : "0.0"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
            Item { width: 1; height: 1; }
            //-- Right edge, under indicator thingy
            Item {
                width:          1
                height:         1
                Layout.columnSpan: 4
            }
            Item {
                width:          _indicatorDiameter * 0.5
                height:         1
            }
        }
    }

    //-- Heading
    Rectangle {
        width:   headingCol.width  * 1.5
        height:  headingCol.height * 1.25
        radius:  ScreenTools.defaultFontPixelWidth * 0.5
        color:   "black"
        visible: !_showAttitude
        anchors.bottom: compassAttitudeCombo.top
        anchors.bottomMargin: -ScreenTools.defaultFontPixelHeight
        anchors.horizontalCenter: compassAttitudeCombo.horizontalCenter
        Column {
            id: headingCol
            anchors.centerIn: parent
            QGCLabel {
                text:           ('000' + _heading.toFixed(0)).slice(-3)
                color:          "white"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Item {
                width:  1
                height: ScreenTools.defaultFontPixelHeight
            }
        }
    }
    //-- Indicator thingy
    Item {
        id:             compassAttitudeComboAlt
        width:          _indicatorDiameter
        height:         outerCompassAlt.height
        visible:        _showAttitude
        anchors.bottom: vehicleStatus.bottom
        anchors.right:  parent.right
        anchors.rightMargin:  ScreenTools.defaultFontPixelWidth
        CompassRing {
            id:             outerCompassAlt
            size:           parent.width * 1.05
            vehicle:        _activeVehicle
            anchors.horizontalCenter: parent.horizontalCenter
            QGCAttitudeWidget {
                id:                 attitudeWidget
                size:               parent.width * 0.85
                vehicle:            _activeVehicle
                showHeading:        true
                anchors.centerIn:   outerCompassAlt
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                indicatorClicked()
            }
        }
    }

    Item {
        id:             compassAttitudeCombo
        width:          _indicatorDiameter
        height:         outerCompass.height
        visible:        !_showAttitude
        anchors.bottom: vehicleStatus.bottom
        anchors.right:  parent.right
        anchors.rightMargin:  ScreenTools.defaultFontPixelWidth
        CompassRing {
            id:                 outerCompass
            size:               parent.width * 1.05
            vehicle:            _activeVehicle
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Rectangle {
            width:  outerCompass.width
            height: width
            radius: width * 0.5
            color:  Qt.rgba(0,0,0,0)
            border.color: Qt.rgba(1,1,1,0.35)
            border.width: 1
            anchors.centerIn:   outerCompass
        }
        Column {
            spacing: ScreenTools.defaultFontPixelHeight * 0.5
            anchors.centerIn:   outerCompass
            Label {
                text:           _activeVehicle ? _activeVehicle.groundSpeed.value.toFixed(0) + ' ' + _activeVehicle.groundSpeed.units : "0 m/s"
                color:          "white"
                width:          compassAttitudeCombo.width * 0.8
                font.family:    ScreenTools.demiboldFontFamily
                fontSizeMode:   Text.HorizontalFit
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text:           _altitude
                color:          "white"
                width:          compassAttitudeCombo.width * 0.8
                font.family:    ScreenTools.demiboldFontFamily
                fontSizeMode:   Text.HorizontalFit
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                indicatorClicked()
            }
        }
    }

    //-- Simple alert message
    Component {
        id:             simpleAlert
        Item {
            id:         simpleAlertItem
            width:      mainWindow.width
            height:     mainWindow.height
            z:          1000000
            DeadMouseArea {
                anchors.fill:   parent
            }
            Rectangle {
                id:             simpleAlertShadow
                anchors.fill:   simpleAlertRect
                radius:         simpleAlertRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       simpleAlertShadow
                visible:            simpleAlertRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             simpleAlertShadow
            }
            Rectangle {
                id:                 simpleAlertRect
                width:              mainWindow.width * 0.65
                height:             simpleAlertCol.height * 1.5
                radius:             ScreenTools.defaultFontPixelWidth
                color:              qgcPal.alertBackground
                border.color:       qgcPal.alertBorder
                border.width:       2
                anchors.centerIn:   parent
                Column {
                    id:                 simpleAlertCol
                    width:              simpleAlertRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 3
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           _messageTitle
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           _messageText
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCButton {
                        text:           qsTr("Close")
                        width:          ScreenTools.defaultFontPixelWidth  * 10
                        height:         ScreenTools.defaultFontPixelHeight * 2
                        onClicked: {
                            mainWindow.enableToolbar()
                            rootLoader.sourceComponent = null
                        }
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = simpleAlertItem.width
                rootLoader.height = simpleAlertItem.height
            }
        }
    }

    //-- Panic Button Dialog
    Component {
        id:             panicDialog
        Item {
            id:         panicDialogItem
            width:      mainWindow.width
            height:     mainWindow.height
            z:          1000000
            DeadMouseArea {
                anchors.fill:   parent
            }
            Rectangle {
                id:             panicDialogShadow
                anchors.fill:   panicDialogRect
                radius:         panicDialogRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       panicDialogShadow
                visible:            panicDialogRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             panicDialogShadow
            }
            Rectangle {
                id:     panicDialogRect
                width:  mainWindow.width   * 0.65
                height: nosdcardCol.height * 1.5
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 nosdcardCol
                    width:              panicDialogRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 3
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("Emergency Vehicle Stop")
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           qsTr("Warning: Motors Will Be Shut Down!")
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           qsTr("Confirm Emergency Vehicle Stop?")
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCButton {
                            text:           qsTr("Stop Vehicle")
                            width:          ScreenTools.defaultFontPixelWidth  * 16
                            height:         ScreenTools.defaultFontPixelHeight * 2
                            onClicked: {
                                if(_activeVehicle) {
                                    _activeVehicle.emergencyStop()
                                }
                                mainWindow.enableToolbar()
                                rootLoader.sourceComponent = null
                            }
                        }
                        QGCButton {
                            text:           qsTr("Cancel")
                            width:          ScreenTools.defaultFontPixelWidth  * 16
                            height:         ScreenTools.defaultFontPixelHeight * 2
                            onClicked: {
                                mainWindow.enableToolbar()
                                rootLoader.sourceComponent = null
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = panicDialogItem.width
                rootLoader.height = panicDialogItem.height
                mainWindow.disableToolbar()
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
                id:             connectionLostArmedShadow
                anchors.fill:   connectionLostArmedRect
                radius:         connectionLostArmedRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       connectionLostArmedShadow
                visible:            connectionLostArmedRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             connectionLostArmedShadow
            }
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
            DeadMouseArea {
                anchors.fill:   parent
            }
            Component.onCompleted: {
                rootLoader.width  = connectionLostArmedItem.width
                rootLoader.height = connectionLostArmedItem.height
                mainWindow.disableToolbar()
            }
        }
    }
    //-- Desktop is connected
    Component {
        id:         desktopConnectedDlg
        Item {
            id:         desktopConnectedDlgItem
            z:          1000000
            width:      mainWindow.width
            height:     mainWindow.height
            Rectangle {
                id:             desktopConnectedDlgShadow
                anchors.fill:   desktopConnectedDlgRect
                radius:         desktopConnectedDlgRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       desktopConnectedDlgShadow
                visible:            desktopConnectedDlgRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             desktopConnectedDlgShadow
            }
            Rectangle {
                id:     desktopConnectedDlgRect
                width:  mainWindow.width   * 0.65
                height: desktopConnectedDlgCol.height * 3
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 desktopConnectedDlgCol
                    width:              desktopConnectedDlgRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 3
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("Connected to Desktop")
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           qsTr("User interface disabled while connected to desktop.")
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            DeadMouseArea {
                anchors.fill:   parent
            }
            Component.onCompleted: {
                rootLoader.width  = desktopConnectedDlgItem.width
                rootLoader.height = desktopConnectedDlgItem.height
                mainWindow.disableToolbar()
            }
        }
    }
    //-- Update Dialog
    Component {
        id: updateDialog
        Loader {
            anchors.fill: parent
            source: "/typhoonh/UpdateDialog.qml"
        }
    }
    //-- Initial Settings Dialog
    Component {
        id: initialSettingsDialog
        Loader {
            anchors.fill: parent
            source: "/typhoonh/InitialSettings.qml"
        }
    }
}
