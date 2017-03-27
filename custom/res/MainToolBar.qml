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

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0
import QGroundControl.CameraControl         1.0

import TyphoonHQuickInterface               1.0

Rectangle {
    id:         toolBar
    color:      qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.8) : Qt.rgba(0,0,0,0.75)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.connectionLost : false
    property var    _camController:     TyphoonHQuickInterface.cameraControl
    property var    _sepColor:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.5) : Qt.rgba(1,1,1,0.5)
    property bool   _cameraAutoMode:    _camController ? _camController.aeMode === CameraControl.AE_MODE_AUTO : false;
    property bool   _cameraVideoMode:   _camController ? _camController.cameraMode === CameraControl.CAMERA_MODE_VIDEO : false
    property bool   _cameraPresent:     _camController && _camController.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED

    signal showSettingsView
    signal showSetupView
    signal showPlanView
    signal showFlyView
    signal showAnalyzeView

    function checkSettingsButton() {
        rootLoader.sourceComponent = null
        settingsButton.checked = true
    }

    function checkSetupButton() {
        rootLoader.sourceComponent = null
        setupButton.checked = true
    }

    function checkPlanButton() {
        rootLoader.sourceComponent = null
        planButton.checked = true
    }

    function checkFlyButton() {
        rootLoader.sourceComponent = null
        homeButton.checked = true
    }

    function checkAnalyzeButton() {

    }

    Component.onCompleted: {
        homeButton.checked = true
    }

    /// Bottom single pixel divider
    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        height:         1
        color:          "black"
        visible:        qgcPal.globalTheme === QGCPalette.Light
    }

    // Easter egg mechanism
    MouseArea {
        anchors.fill: parent
        onClicked: {
            console.log("easter egg click", ++_clickCount)
            eggTimer.restart()
            if (_clickCount == 5) {
                QGroundControl.corePlugin.showAdvancedUI = true
            } else if (_clickCount == 7) {
                QGroundControl.corePlugin.showTouchAreas = true
            }
        }

        property int _clickCount: 0

        Timer {
            id:             eggTimer
            interval:       1000
            onTriggered:    parent._clickCount = 0
        }
    }

    ExclusiveGroup { id: mainActionGroup }

    QGCToolBarButton {
        id:                 homeButton
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.leftMargin: 10
        exclusiveGroup:     mainActionGroup
        source:             "/typhoonh/Home.svg"
        onClicked:          toolBar.showFlyView()
    }

    Row {
        id:                     mainRow
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   1
        anchors.left:           homeButton.right
        anchors.leftMargin:     50
        spacing:                50 //-- Hard coded to fit the ST16 Screen

        QGCToolBarButton {
            id:                 setupButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Gears.svg"
            onClicked:          toolBar.showSetupView()
        }

        QGCToolBarButton {
            id:                 planButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Plan.svg"
            onClicked:          toolBar.showPlanView()
        }

        Rectangle {
            height:             parent.height * 0.75
            width:              1
            color:              qgcPal.text
            opacity:            0.5
            anchors.verticalCenter: parent.verticalCenter
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/toolbar/MessageIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/ModeIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/YGPSIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/BatteryIndicator.qml"
        }

        Rectangle {
            height:             parent.height * 0.75
            width:              1
            color:              qgcPal.text
            opacity:            0.5
            anchors.verticalCenter: parent.verticalCenter
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/RCIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/WIFIRSSIIndicator.qml"
        }

    }

    // Progress bar
    Rectangle {
        id:             progressBar
        anchors.bottom: parent.bottom
        height:         toolBar.height * 0.05
        width:          _activeVehicle ? _activeVehicle.parameterManager.loadProgress * parent.width : 0
        color:          qgcPal.colorGreen
    }

    //-- Camera Status
    Rectangle {
        width:          camRow.width + (ScreenTools.defaultFontPixelWidth * 2)
        height:         camRow.height * (_cameraVideoMode ? 1.25 : 1.5)
        color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0.15,1,0.15,0.85) : Qt.rgba(0,0.15,0,0.85)
        visible:        _cameraPresent && homeButton.checked && indicatorDropdown.sourceComponent === null && !messageArea.visible && !criticalMmessageArea.visible
        radius:         3
        anchors.top:    parent.bottom
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
                text: _camController ? _camController.evList[_camController.currentEV] : "";
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
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; }
            QGCLabel { text: qsTr("Metering:"); anchors.verticalCenter: parent.verticalCenter;}
            QGCLabel { text: _camController ? _camController.meteringList[_camController.currentMetering] : ""; anchors.verticalCenter: parent.verticalCenter;}
            //-- Recording Time
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraVideoMode; }
            QGCLabel {
                text: _camController ? (_camController.videoStatus === CameraControl.VIDEO_CAPTURE_STATUS_RUNNING ? TyphoonHQuickInterface.cameraControl.recordTimeStr : "00:00:00") : "";
                font.pointSize: ScreenTools.mediumFontPointSize
                visible: _cameraVideoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; visible: _cameraVideoMode;}
            //-- Video Res
            QGCLabel {
                text: _camController ? _camController.videoResList[_camController.currentVideoRes] : "";
                visible: _cameraVideoMode;
                anchors.verticalCenter: parent.verticalCenter;
            }
            //-- SD Card
            Rectangle { width: 1; height: camRow.height * 0.75; color: _sepColor; anchors.verticalCenter: parent.verticalCenter; }
            QGCLabel { text: qsTr("SD:"); anchors.verticalCenter: parent.verticalCenter;}
            QGCLabel { text: _camController ? _camController.sdFreeStr : ""; anchors.verticalCenter: parent.verticalCenter;}
        }
    }

}
