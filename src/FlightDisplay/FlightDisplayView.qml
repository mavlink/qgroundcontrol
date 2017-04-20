/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.3
import QtMultimedia             5.5
import QtQuick.Layouts          1.2

import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0

/// Flight Display View
QGCView {
    id:             root
    viewPanel:      _panel

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    property alias  guidedController:   guidedActionsController

    property bool activeVehicleJoystickEnabled: _activeVehicle ? _activeVehicle.joystickEnabled : false

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _mainIsMap:         QGroundControl.videoManager.hasVideo ? QGroundControl.loadBoolGlobalSetting(_mainIsMapKey,  true) : true
    property bool   _isPipVisible:      QGroundControl.videoManager.hasVideo ? QGroundControl.loadBoolGlobalSetting(_PIPVisibleKey, true) : false
    property real   _savedZoomLevel:    0
    property real   _margins:           ScreenTools.defaultFontPixelWidth / 2
    property real   _pipSize:           mainWindow.width * 0.2
    property alias  _guidedController:  guidedActionsController
    property alias  _altitudeSlider:    altitudeSlider


    readonly property bool      isBackgroundDark:       _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true
    readonly property real      _defaultRoll:           0
    readonly property real      _defaultPitch:          0
    readonly property real      _defaultHeading:        0
    readonly property real      _defaultAltitudeAMSL:   0
    readonly property real      _defaultGroundSpeed:    0
    readonly property real      _defaultAirSpeed:       0
    readonly property string    _mapName:               "FlightDisplayView"
    readonly property string    _showMapBackgroundKey:  "/showMapBackground"
    readonly property string    _mainIsMapKey:          "MainFlyWindowIsMap"
    readonly property string    _PIPVisibleKey:         "IsPIPVisible"

    function setStates() {
        QGroundControl.saveBoolGlobalSetting(_mainIsMapKey, _mainIsMap)
        if(_mainIsMap) {
            //-- Adjust Margins
            _flightMapContainer.state   = "fullMode"
            _flightVideo.state          = "pipMode"
            //-- Save/Restore Map Zoom Level
            if(_savedZoomLevel != 0)
                _flightMap.zoomLevel = _savedZoomLevel
            else
                _savedZoomLevel = _flightMap.zoomLevel
        } else {
            //-- Adjust Margins
            _flightMapContainer.state   = "pipMode"
            _flightVideo.state          = "fullMode"
            //-- Set Map Zoom Level
            _savedZoomLevel = _flightMap.zoomLevel
            _flightMap.zoomLevel = _savedZoomLevel - 3
        }
    }

    function setPipVisibility(state) {
        _isPipVisible = state;
        QGroundControl.saveBoolGlobalSetting(_PIPVisibleKey, state)
    }

    function px4JoystickCheck() {
        if ( _activeVehicle && !_activeVehicle.supportsManualControl && (QGroundControl.settingsManager.appSettings.virtualJoystick.value || _activeVehicle.joystickEnabled)) {
            px4JoystickSupport.open()
        }
    }

    MissionController {
        id:                     flyMissionController
        Component.onCompleted:  start(false /* editMode */)
        onResumeMissionReady:   guidedActionsController.confirmAction(guidedActionsController.actionResumeMissionReady)
    }

    GeoFenceController {
        id: flyGeoFenceController
        Component.onCompleted: start(false /* editMode */)
    }

    RallyPointController {
        id: flyRallyPointController
        Component.onCompleted: start(false /* editMode */)
    }

    MessageDialog {
        id:     px4JoystickSupport
        text:   qsTr("Joystick support requires MAVLink MANUAL_CONTROL support. ") +
                qsTr("The firmware you are running does not normally support this. ") +
                qsTr("It will only work if you have modified the firmware to add MANUAL_CONTROL support.")
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        onActiveVehicleChanged: px4JoystickCheck()
    }

    Connections {
        target:         QGroundControl.settingsManager.appSettings.virtualJoystick
        onValueChanged: px4JoystickCheck()
    }

    onActiveVehicleJoystickEnabledChanged: px4JoystickCheck()

    Component.onCompleted: {
        setStates()
        px4JoystickCheck()
        if(QGroundControl.corePlugin.options.flyViewOverlay.toString().length) {
            flyViewOverlay.source = QGroundControl.corePlugin.options.flyViewOverlay
        }
    }

    // The following code is used to track vehicle states such that we prompt to remove mission from vehicle when mission completes

    property bool vehicleArmed:                 _activeVehicle ? _activeVehicle.armed : true // true here prevents pop up from showing during shutdown
    property bool vehicleWasArmed:              false
    property bool vehicleInMissionFlightMode:   _activeVehicle ? (_activeVehicle.flightMode === _activeVehicle.missionFlightMode) : false
    property bool promptForMissionRemove:       false

    onVehicleArmedChanged: {
        if (vehicleArmed) {
            if (!promptForMissionRemove) {
                promptForMissionRemove = vehicleInMissionFlightMode
                vehicleWasArmed = true
            }
        } else {
            if (promptForMissionRemove && (flyMissionController.containsItems || flyGeoFenceController.containsItems || flyRallyPointController.containsItems)) {
                root.showDialog(removeMissionDialogComponent, qsTr("Flight complete"), showDialogDefaultWidth, StandardButton.No | StandardButton.Yes)
            }
            promptForMissionRemove = false
        }
    }

    onVehicleInMissionFlightModeChanged: {
        if (!promptForMissionRemove && vehicleArmed) {
            promptForMissionRemove = true
        }
    }

    Component {
        id: removeMissionDialogComponent

        QGCViewMessage {
            message: qsTr("Do you want to remove the mission from the vehicle?")

            function accept() {
                flyMissionController.removeAllFromVehicle()
                flyGeoFenceController.removeAllFromVehicle()
                flyRallyPointController.removeAllFromVehicle()
                hideDialog()

            }
        }
    }

    QGCMapPalette { id: mapPal; lightColors: _mainIsMap ? _flightMap.isSatelliteMap : true }

    QGCViewPanel {
        id:             _panel
        anchors.fill:   parent

        //-- Map View
        //   For whatever reason, if FlightDisplayViewMap is the _panel item, changing
        //   width/height has no effect.
        Item {
            id: _flightMapContainer
            z:  _mainIsMap ? _panel.z + 1 : _panel.z + 2
            anchors.left:   _panel.left
            anchors.bottom: _panel.bottom
            visible:        _mainIsMap || _isPipVisible
            width:          _mainIsMap ? _panel.width  : _pipSize
            height:         _mainIsMap ? _panel.height : _pipSize * (9/16)
            states: [
                State {
                    name:   "pipMode"
                    PropertyChanges {
                        target:             _flightMapContainer
                        anchors.margins:    ScreenTools.defaultFontPixelHeight
                    }
                },
                State {
                    name:   "fullMode"
                    PropertyChanges {
                        target:             _flightMapContainer
                        anchors.margins:    0
                    }
                }
            ]
            FlightDisplayViewMap {
                id:                         _flightMap
                anchors.fill:               parent
                missionController:          flyMissionController
                geoFenceController:         flyGeoFenceController
                rallyPointController:       flyRallyPointController
                guidedActionsController:    _guidedController
                flightWidgets:              flightDisplayViewWidgets
                rightPanelWidth:            ScreenTools.defaultFontPixelHeight * 9
                qgcView:                    root
                scaleState:                 (_mainIsMap && flyViewOverlay.item) ? (flyViewOverlay.item.scaleState ? flyViewOverlay.item.scaleState : "bottomMode") : "bottomMode"
            }
        }

        //-- Video View
        Item {
            id:             _flightVideo
            z:              _mainIsMap ? _panel.z + 2 : _panel.z + 1
            width:          !_mainIsMap ? _panel.width  : _pipSize
            height:         !_mainIsMap ? _panel.height : _pipSize * (9/16)
            anchors.left:   _panel.left
            anchors.bottom: _panel.bottom
            visible:        QGroundControl.videoManager.hasVideo && (!_mainIsMap || _isPipVisible)
            states: [
                State {
                    name:   "pipMode"
                    PropertyChanges {
                        target: _flightVideo
                        anchors.margins:    ScreenTools.defaultFontPixelHeight
                    }
                },
                State {
                    name:   "fullMode"
                    PropertyChanges {
                        target: _flightVideo
                        anchors.margins:    0
                    }
                }
            ]
            //-- Video Streaming
            FlightDisplayViewVideo {
                anchors.fill:   parent
                visible:        QGroundControl.videoManager.isGStreamer
            }
            //-- UVC Video (USB Camera or Video Device)
            Loader {
                id:             cameraLoader
                anchors.fill:   parent
                visible:        !QGroundControl.videoManager.isGStreamer
                source:         QGroundControl.videoManager.uvcEnabled ? "qrc:/qml/FlightDisplayViewUVC.qml" : "qrc:/qml/FlightDisplayViewDummy.qml"
            }
        }

        QGCPipable {
            id:                 _flightVideoPipControl
            z:                  _flightVideo.z + 3
            width:              _pipSize
            height:             _pipSize * (9/16)
            anchors.left:       _panel.left
            anchors.bottom:     _panel.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            visible:            QGroundControl.videoManager.hasVideo
            isHidden:           !_isPipVisible
            isDark:             isBackgroundDark
            onActivated: {
                _mainIsMap = !_mainIsMap
                setStates()
            }
            onHideIt: {
                setPipVisibility(!state)
            }
        }

        Row {
            id:                     singleMultiSelector
            anchors.topMargin:      ScreenTools.toolbarHeight + _margins
            anchors.rightMargin:    _margins
            anchors.right:          parent.right
            anchors.top:            parent.top
            spacing:                ScreenTools.defaultFontPixelWidth
            z:                      _panel.z + 4
            visible:                QGroundControl.multiVehicleManager.vehicles.count > 1

            ExclusiveGroup { id: multiVehicleSelectorGroup }

            QGCRadioButton {
                id:             singleVehicleView
                exclusiveGroup: multiVehicleSelectorGroup
                text:           qsTr("Single")
                checked:        true
                color:          mapPal.text
            }

            QGCRadioButton {
                exclusiveGroup: multiVehicleSelectorGroup
                text:           qsTr("Multi-Vehicle (WIP)")
                color:          mapPal.text
            }
        }

        FlightDisplayViewWidgets {
            id:                 flightDisplayViewWidgets
            z:                  _panel.z + 4
            height:             ScreenTools.availableHeight
            anchors.left:       parent.left
            anchors.right:      altitudeSlider.visible ? altitudeSlider.left : parent.right
            anchors.bottom:     parent.bottom
            qgcView:            root
            useLightColors:     isBackgroundDark
            missionController:  _flightMap.missionController
            visible:            singleVehicleView.checked
        }

        //-------------------------------------------------------------------------
        //-- Loader helper for plugins to overlay elements over the fly view
        Loader {
            id:                 flyViewOverlay
            z:                  flightDisplayViewWidgets.z + 1
            height:             ScreenTools.availableHeight
            anchors.left:       parent.left
            anchors.right:      altitudeSlider.visible ? altitudeSlider.left : parent.right
            anchors.bottom:     parent.bottom

            property var qgcView: root
        }

        // Button to start/stop video recording
        Item {
            z:                  _flightVideoPipControl.z + 1
            anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
            anchors.bottom:     _flightVideo.bottom
            anchors.right:      _flightVideo.right
            height:             ScreenTools.defaultFontPixelHeight * 2
            width:              height
            visible:            QGroundControl.videoManager.videoRunning && QGroundControl.videoManager.recordingEnabled
            opacity:            0.75

            Rectangle {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                width:              height
                radius:             QGroundControl.videoManager && QGroundControl.videoManager.videoReceiver && QGroundControl.videoManager.videoReceiver.recording ? 0 : height
                color:              "red"
            }

            QGCColoredImage {
                anchors.top:                parent.top
                anchors.bottom:             parent.bottom
                anchors.horizontalCenter:   parent.horizontalCenter
                width:                      height * 0.625
                sourceSize.width:           width
                source:                     "/qmlimages/CameraIcon.svg"
                fillMode:                   Image.PreserveAspectFit
                color:                      "white"
            }

            MouseArea {
                anchors.fill:   parent
                onClicked:      QGroundControl.videoManager.videoReceiver && QGroundControl.videoManager.videoReceiver.recording ? QGroundControl.videoManager.videoReceiver.stopRecording() : QGroundControl.videoManager.videoReceiver.startRecording()
            }
        }

        MultiVehicleList {
            anchors.margins:    _margins
            anchors.top:        singleMultiSelector.bottom
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            width:              ScreenTools.defaultFontPixelWidth * 30
            visible:            !singleVehicleView.checked
            z:                  _panel.z + 4
        }


        //-- Virtual Joystick
        Loader {
            id:                         virtualJoystickMultiTouch
            z:                          _panel.z + 5
            width:                      parent.width  - (_flightVideoPipControl.width / 2)
            height:                     Math.min(ScreenTools.availableHeight * 0.25, ScreenTools.defaultFontPixelWidth * 16)
            visible:                    _virtualJoystick ? _virtualJoystick.value : false
            anchors.bottom:             _flightVideoPipControl.top
            anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight * 2
            anchors.horizontalCenter:   flightDisplayViewWidgets.horizontalCenter
            source:                     "qrc:/qml/VirtualJoystick.qml"
            active:                     _virtualJoystick ? _virtualJoystick.value : false

            property bool useLightColors: isBackgroundDark

            property Fact _virtualJoystick: QGroundControl.settingsManager.appSettings.virtualJoystick
        }

        ToolStrip {
            visible:            _activeVehicle ? _activeVehicle.guidedModeSupported : true
            id:                 toolStrip
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth
            anchors.left:       _panel.left
            anchors.topMargin:  ScreenTools.toolbarHeight + (_margins * 2)
            anchors.top:        _panel.top
            z:                  _panel.z + 4
            title:              qsTr("Fly")
            maxHeight:          (_flightVideo.visible ? _flightVideo.y : parent.height) - toolStrip.y
            buttonVisible:      [ _guidedController.showTakeoff || !_guidedController.showLand, _guidedController.showLand && !_guidedController.showTakeoff, true, true, true, _guidedController.smartShotsAvailable ]
            buttonEnabled:      [ _guidedController.showTakeoff, _guidedController.showLand, _guidedController.showRTL, _guidedController.showPause, _anyActionAvailable, _anySmartShotAvailable ]

            property bool _anyActionAvailable: _guidedController.showStartMission || _guidedController.showResumeMission || _guidedController.showChangeAlt || _guidedController.showLandAbort
            property bool _anySmartShotAvailable: _guidedController.showOrbit
            property var _actionModel: [
                {
                    title:      _guidedController.startMissionTitle,
                    text:       _guidedController.startMissionMessage,
                    action:     _guidedController.actionStartMission,
                    visible:    _guidedController.showStartMission
                },
                {
                    title:      _guidedController.resumeMissionTitle,
                    text:       _guidedController.resumeMissionMessage,
                    action:     _guidedController.actionResumeMission,
                    visible:    _guidedController.showResumeMission
                },
                {
                    title:      _guidedController.changeAltTitle,
                    text:       _guidedController.changeAltMessage,
                    action:     _guidedController.actionChangeAlt,
                    visible:    _guidedController.showChangeAlt
                },
                {
                    title:      _guidedController.landAbortTitle,
                    text:       _guidedController.landAbortMessage,
                    action:     _guidedController.actionLandAbort,
                    visible:    _guidedController.showLandAbort
                }
            ]
            property var _smartShotModel: [
                {
                    title:      _guidedController.orbitTitle,
                    text:       _guidedController.orbitMessage,
                    action:     _guidedController.actionOrbit,
                    visible:    _guidedController.showOrbit
                }
            ]

            model: [
                {
                    name:       _guidedController.takeoffTitle,
                    iconSource: "/res/takeoff.svg",
                    action:     _guidedController.actionTakeoff
                },
                {
                    name:       _guidedController.landTitle,
                    iconSource: "/res/land.svg",
                    action:     _guidedController.actionLand
                },
                {
                    name:       _guidedController.rtlTitle,
                    iconSource: "/res/rtl.svg",
                    action:     _guidedController.actionRTL
                },
                {
                    name:       _guidedController.pauseTitle,
                    iconSource: "/res/pause-mission.svg",
                    action:     _guidedController.actionPause
                },
                {
                    name:       qsTr("Action"),
                    iconSource: "/res/action.svg",
                    action:     -1
                },
                /*
                  No firmware support any smart shots yet
                {
                    name:       qsTr("Smart"),
                    iconSource: "/qmlimages/MapCenter.svg",
                    action:     -1
                },
                */
            ]

            onClicked: {
                //-- Dismiss any other dialog
                rootLoader.sourceComponent  = null
                guidedActionConfirm.visible = false
                guidedActionList.visible    = false
                altitudeSlider.visible      = false
                var action = model[index].action
                if (action === -1) {
                    if (index == 4) {
                        guidedActionList.model   = _actionModel
                        guidedActionList.visible = true
                    } else if (index == 5) {
                        guidedActionList.model   = _smartShotModel
                        guidedActionList.visible = true
                    }
                } else {
                    _guidedController.confirmAction(action)
                }
            }
        }

        GuidedActionsController {
            id:                 guidedActionsController
            missionController:  flyMissionController
            confirmDialog:      guidedActionConfirm
            z:                  _flightVideoPipControl.z + 1

            onShowStartMissionChanged: {
                if (showStartMission && !showResumeMission) {
                    confirmAction(actionStartMission)
                }
            }

            onShowContinueMissionChanged: {
                if (showContinueMission) {
                    confirmAction(actionContinueMission)
                }
            }

            onShowResumeMissionChanged: {
                if (showResumeMission) {
                    confirmAction(actionResumeMission)
                }
            }
        }

        GuidedActionConfirm {
            id:                         guidedActionConfirm
            anchors.margins:            _margins
            anchors.bottom:             parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            guidedController:           _guidedController
            altitudeSlider:             _altitudeSlider
        }

        GuidedActionList {
            id:                         guidedActionList
            anchors.margins:            _margins
            anchors.bottom:             parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            guidedController:           _guidedController
            altitudeSlider:             _altitudeSlider
        }

        //-- Altitude slider
        GuidedAltitudeSlider {
            id:                 altitudeSlider
            anchors.margins:    _margins
            anchors.right:      parent.right
            anchors.topMargin:  ScreenTools.toolbarHeight + _margins
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            z:                  _guidedController.z
            radius:             ScreenTools.defaultFontPixelWidth / 2
            width:              ScreenTools.defaultFontPixelWidth * 10
            color:              qgcPal.window
            visible:            false
        }
    }
}
