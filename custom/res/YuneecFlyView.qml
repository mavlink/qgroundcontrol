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
    property string _altitude:          _activeVehicle ? (isNaN(_activeVehicle.altitudeRelative.value) ? "0.0" : _activeVehicle.altitudeRelative.value.toFixed(1)) + ' ' + _activeVehicle.altitudeRelative.units : "0.0 m"
    property string _distanceStr:       isNaN(_distance) ? "0 m" : _distance.toFixed(0) + ' ' + (_activeVehicle ? _activeVehicle.altitudeRelative.units : "m")
    property real   _heading:           _activeVehicle ? _activeVehicle.heading.rawValue : 0
    property bool   _showAttitude:      false

    Timer {
        id: ssidChanged
        interval:  5000
        running:   false;
        repeat:    false;
        onTriggered: {
            if(TyphoonHQuickInterface.wifiAlertEnabled) {
                rootLoader.sourceComponent = connectedToAP
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
        visible:                !_mainIsMap
        source:                 _mainIsMap ? "" : "/typhoonh/cameraControl.qml"
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.top:            parent.top
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 2
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
                text:   _activeVehicle ? ('00000' + _activeVehicle.flightDistance.value.toFixed(0)).slice(-5) + ' ' + _activeVehicle.flightDistance.units : "00000 m"
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
                text:       _activeVehicle ? _activeVehicle.groundSpeed.rawValue.toFixed(1) + ' ' + _activeVehicle.groundSpeed.units : "0.0 m/s"
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
                text:   _distanceStr
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
            //-- Vertical Speed
            QGCLabel {
                text:   qsTr("V.S:")
            }
            QGCLabel {
                text:           _activeVehicle ? _activeVehicle.climbRate.value.toFixed(1) + ' ' + _activeVehicle.climbRate.units : "0.0 m/s"
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
                vehicleStatus.visible = !vehicleStatus.visible
            }
            onDoubleClicked: {
                vehicleStatus.visible = true
                _showAttitude = !_showAttitude
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
                vehicleStatus.visible = !vehicleStatus.visible
            }
            onDoubleClicked: {
                vehicleStatus.visible = true
                _showAttitude = !_showAttitude
            }
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
                id:             nosdShadow
                anchors.fill:   nosdRect
                radius:         nosdRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       nosdShadow
                visible:            nosdRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             nosdShadow
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

    //-- Connected to some AP and not a Typhoon
    Component {
        id:             connectedToAP
        Item {
            id:         connectedToAPItem
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
                id:             conAPShadow
                anchors.fill:   conAPRect
                radius:         conAPRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       conAPShadow
                visible:            conAPRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             conAPShadow
            }
            Rectangle {
                id:     conAPRect
                width:  mainWindow.width   * 0.65
                height: nosdcardCol.height * 1.5
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 nosdcardCol
                    width:              conAPRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 3
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("Connected to Standard Wi-Fi")
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           qsTr("The ST16 is connected to a standard Wi-Fi and not a vehicle.")
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
                rootLoader.width  = connectedToAPItem.width
                rootLoader.height = connectedToAPItem.height
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
