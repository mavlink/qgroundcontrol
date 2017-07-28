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

Item {
    anchors.fill: parent

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property string scaleState:    "topMode"
    readonly property string _naString:     qsTr('N/A')

    property real   _indicatorDiameter: ScreenTools.defaultFontPixelWidth * 16
    property var    _sepColor:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.5) : Qt.rgba(1,1,1,0.5)

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.connectionLost : false
    property var    _dynamicCameras:    _activeVehicle ? _activeVehicle.dynamicCameras : null
    property bool   _isCamera:          _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _camera:            _isCamera ? _dynamicCameras.cameras.get(0) : null // Single camera support for the time being
    property bool   _cameraVideoMode:   _camera ?  _camera.cameraMode === QGCCameraControl.CAM_MODE_VIDEO : false
    property bool   _cameraPresent:     _camera && _camera.cameraMode !== QGCCameraControl.CAM_MODE_UNDEFINED
    property bool   _noSdCard:          _camera && _camera.storageTotal === 0
    property bool   _fullSD:            _camera && _camera.storageTotal !== 0 && _camera.storageFree > 0 && _camera.storageFree < 250 // We get kiB from the camera

    property var    _expModeFact:       _camera && _camera.exposureMode
    property var    _evFact:            _camera && _camera.ev
    property var    _isoFact:           _camera && _camera.iso
    property var    _shutterFact:       _camera && _camera.shutterSpeed
    property var    _wbFact:            _camera && _camera.wb
    property var    _meteringFact:      _camera && _camera.meteringMode
    property var    _videoResFact:      _camera && _camera.videoRes

    property string _currentAEMode:     _expModeFact  ? _expModeFact.enumStringValue : _naString
    property bool   _cameraAutoMode:    _expModeFact  ? _expModeFact.rawValue === 0 : true
    property string _currentEV:         _evFact       ? _evFact.enumStringValue : '0'
    property string _currentISO:        _isoFact      ? _isoFact.enumStringValue : _naString
    property string _currentShutter:    _shutterFact  ? _shutterFact.enumStringValue : _naString
    property string _currentWB:         _wbFact       ? _wbFact.enumStringValue : _naString
    property string _currentMetering:   _meteringFact ? _meteringFact.enumStringValue : _naString
    property string _currentVideoRes:   _videoResFact ? _videoResFact.enumStringValue : _naString

    property string _altitude:          _activeVehicle   ? (isNaN(_activeVehicle.altitudeRelative.value) ? "0.0" : _activeVehicle.altitudeRelative.value.toFixed(1)) + ' ' + _activeVehicle.altitudeRelative.units : "0.0"
    property string _distanceStr:       isNaN(_distance) ? "0" : _distance.toFixed(0) + ' ' + (_activeVehicle ? _activeVehicle.altitudeRelative.units : "")
    property real   _heading:           _activeVehicle   ? _activeVehicle.heading.rawValue : 0
    property bool   _st16GPS:           false
    property real   _gimbalPitch:       _camera ? -_camera.gimbalPitch : 0
    property real   _gimbalYaw:         _camera ? _camera.gimbalYaw : 0
    property bool   _gimbalVisible:     _camera ? _camera.gimbalData && camControlLoader.visible : false

    property real   _distance:              0.0
    property bool   _noSdCardMsgShown:      false
    property bool   _fullSdCardMsgShown:    false
    property bool   _showAttitude:          false
    property int    _eggCount:              0
    property string _messageTitle:          ""
    property string _messageText:           ""

    function showSimpleAlert(title, message) {
        _messageTitle   = title;
        _messageText    = message;
        rootLoader.sourceComponent = simpleAlert;
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

    Timer {
        id: ssidChanged
        interval:  5000
        running:   false;
        repeat:    false;
        onTriggered: {
            if(TyphoonHQuickInterface.wifiAlertEnabled) {
                showSimpleAlert(
                    qsTr("Connected to Standard Wi-Fi"),
                    qsTr("The ST16 is connected to a standard Wi-Fi and not a vehicle."))
            }
        }
    }

    Timer {
        id:         toggleTimer
        interval:   1000
        onTriggered: {
            toggleTimer.stop()
            parent._eggCount = 0
        }
    }

    function indicatorClicked() {
        vehicleStatus.visible = !vehicleStatus.visible
        _eggCount++
        toggleTimer.restart()
        if (_eggCount == 8) {
            vehicleStatus.visible = true
            _showAttitude = !_showAttitude
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
            if(ScreenTools.isAndroid && TyphoonHQuickInterface.connectedCamera === "" && TyphoonHQuickInterface.connectedSSID !== "") {
                ssidChanged.start()
            } else {
                ssidChanged.stop()
            }
        }
        //-- Big Red Button down for > 1 second
        onPowerHeld: {
            if(_activeVehicle) {
                rootLoader.sourceComponent = panicDialog
                mainWindow.disableToolbar()
            }
        }
        //-- Check for Updates
        onUpdateAlert: {
            showSimpleAlert(
                qsTr("Check For Updates"),
                qsTr("No Internet connection. Please connect to check for updates."))
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
            //-- And comm lost dialog if open
            connectionLostDisarmedDialog.close()
        }
    }

    Connections {
        target: TyphoonHQuickInterface
        onThermalImagePresentChanged: {
            if(TyphoonHQuickInterface.thermalImagePresent) {
                rootVideoLoader.sourceComponent = thermalImage
            } else {
                rootVideoLoader.sourceComponent = null
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
                    rootLoader.sourceComponent = null
                }
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
                if(rootLoader.sourceComponent === simpleAlert) {
                    rootLoader.sourceComponent = null
                }
            }
        }
    }

    //-- Camera Status
    Rectangle {
        width:          camRow.width + (ScreenTools.defaultFontPixelWidth * 3)
        height:         camRow.height * 2
        color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
        visible:        !_mainIsMap && _cameraPresent && indicatorDropdown.sourceComponent === null && !messageArea.visible && !criticalMmessageArea.visible
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
            QGCLabel { text: qsTr("AE:"); anchors.verticalCenter: parent.verticalCenter;}
            QGCLabel { text: _currentAEMode; anchors.verticalCenter: parent.verticalCenter;}
            //-- EV
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode; }
            QGCLabel {
                text: qsTr("EV:");
                visible: _cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            QGCLabel {
                text:   _currentEV
                visible: _cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            //-- ISO
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: !_cameraAutoMode; }
            QGCLabel {
                text:    qsTr("ISO:");
                visible: !_cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            QGCLabel {
                text:    _currentISO
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
                text:    _currentShutter
                visible: !_cameraAutoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            //-- WB
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; }
            QGCLabel { text: qsTr("WB:"); anchors.verticalCenter: parent.verticalCenter;}
            QGCLabel { text: _currentWB; anchors.verticalCenter: parent.verticalCenter; }
            //-- Metering
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode; }
            QGCLabel { text: qsTr("Metering:"); anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode; }
            QGCLabel { text: _currentMetering; anchors.verticalCenter: parent.verticalCenter; visible: _cameraAutoMode; }
            //-- Video Res
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraVideoMode; }
            QGCLabel {
                text:   _currentVideoRes
                visible: _cameraVideoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            //-- SD Card
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; }
            QGCLabel { text: qsTr("SD:"); anchors.verticalCenter: parent.verticalCenter;}
            QGCLabel { text: _camera ? _camera.storageFreeStr : ""; anchors.verticalCenter: parent.verticalCenter; visible: !_noSdCard && !_fullSD}
            QGCLabel { text: qsTr("NONE"); color: qgcPal.colorOrange; anchors.verticalCenter: parent.verticalCenter; visible: _noSdCard}
            QGCLabel { text: qsTr("FULL"); color: qgcPal.colorOrange; anchors.verticalCenter: parent.verticalCenter; visible: _fullSD}
        }
    }

    Component {
        id: thermalImage
        Item {
            id:                 thermalItem
            anchors.centerIn:   parent
            width:              height * 1.333333
            height:             mainWindow.height * 0.75
            visible:            !_mainIsMap
            QGCVideoBackground {
                id:             thermalVideo
                anchors.fill:   parent
                receiver:       TyphoonHQuickInterface.videoReceiver
                display:        TyphoonHQuickInterface.videoReceiver.videoSurface
                visible:        TyphoonHQuickInterface.videoReceiver.videoRunning
            }
            /*
            MouseArea {
                anchors.fill:   parent
                onClicked:      thermalVideo.visible = !thermalVideo.visible
            }
            */
            Component.onCompleted: {
                rootVideoLoader.width  = thermalItem.width
                rootVideoLoader.height = thermalItem.height
            }
        }
    }

    //-- Camera Control
    Loader {
        id:                     camControlLoader
        visible:                !_mainIsMap
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
        anchors.verticalCenter: camControlLoader.verticalCenter
        anchors.right:          camControlLoader.left
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        Column {
            id:                 gimbalCol
            spacing:            ScreenTools.defaultFontPixelHeight * 0.25
            anchors.horizontalCenter: parent.horizontalCenter
            Rectangle {
                width:          ScreenTools.defaultFontPixelWidth * 4
                height:         camControlLoader.height * 0.8
                color:          Qt.rgba(1,1,1,0.55)
                radius:         ScreenTools.defaultFontPixelWidth * 0.5
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
                        y:                  (parent.height * _gimbalPitch / 105) + (parent.height * 0.15) - (ScreenTools.defaultFontPixelWidth * 1.5)
                        smooth:             true
                        mipmap:             true
                        anchors.horizontalCenter: parent.horizontalCenter
                        transform: Rotation {
                            origin.x:       yawIndicator.width  / 2
                            origin.y:       yawIndicator.height / 2
                            angle:          _gimbalYaw + 90
                        }
                    }
                }
            }
            Rectangle {
                width:              ScreenTools.defaultFontPixelWidth * 4
                height:             gimbalLabel.height * 1.5
                color:              Qt.rgba(1,1,1,0.55)
                radius:             ScreenTools.defaultFontPixelWidth * 0.5
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    id:             gimbalLabel
                    text:           _gimbalPitch ? -_gimbalPitch.toFixed(0) : 0
                    color:          "black"
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
                text:               _st16GPS ? _distanceStr : qsTr("NO GPS")
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
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
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
                        onClicked:      rootLoader.sourceComponent = null
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
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
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
                                rootLoader.sourceComponent = null
                                mainWindow.enableToolbar()
                            }
                        }
                        QGCButton {
                            text:           qsTr("Cancel")
                            width:          ScreenTools.defaultFontPixelWidth  * 16
                            height:         ScreenTools.defaultFontPixelHeight * 2
                            onClicked: {
                                rootLoader.sourceComponent = null
                                mainWindow.enableToolbar()
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = panicDialogItem.width
                rootLoader.height = panicDialogItem.height
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
