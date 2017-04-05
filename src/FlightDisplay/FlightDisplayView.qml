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

    property bool activeVehicleJoystickEnabled: _activeVehicle ? _activeVehicle.joystickEnabled : false

    property var _activeVehicle:        QGroundControl.multiVehicleManager.activeVehicle
    property bool _mainIsMap:           QGroundControl.videoManager.hasVideo ? QGroundControl.loadBoolGlobalSetting(_mainIsMapKey,  true) : true
    property bool _isPipVisible:        QGroundControl.videoManager.hasVideo ? QGroundControl.loadBoolGlobalSetting(_PIPVisibleKey, true) : false
    property real _savedZoomLevel:      0
    property real _margins:             ScreenTools.defaultFontPixelWidth / 2
    property real _pipSize:             mainWindow.width * 0.2

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
        id: flyMissionController
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
                id:                     _flightMap
                anchors.fill:           parent
                missionController:      flyMissionController
                guidedActionsController: guidedController
                flightWidgets:          flightDisplayViewWidgets
                rightPanelWidth:        ScreenTools.defaultFontPixelHeight * 9
                qgcView:                root
                scaleState:             (_mainIsMap && flyViewOverlay.item) ? (flyViewOverlay.item.scaleState ? flyViewOverlay.item.scaleState : "bottomMode") : "bottomMode"
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
            id:                 toolStrip
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth
            anchors.left:       _panel.left
            anchors.topMargin:  ScreenTools.toolbarHeight + _margins
            anchors.top:        _panel.top
            z:                  _panel.z + 4
            title:              qsTr("Fly")
            maxHeight:          (_flightVideo.visible ? _flightVideo.y : parent.height) - toolStrip.y
            buttonVisible:      [ guidedController.showTakeoff || !guidedController.showLand, guidedController.showLand, true, true, true, guidedController.smartShotsAvailable ]
            buttonEnabled:      [ guidedController.showTakeoff, guidedController.showLand, guidedController.showRTL, guidedController.showPause, _anyActionAvailable, _anySmartShotAvailable ]

            property bool _anyActionAvailable: guidedController.showEmergenyStop || guidedController.showStartMission || guidedController.showResumeMission || guidedController.showChangeAlt || guidedController.showLandAbort
            property bool _anySmartShotAvailable: guidedController.showOrbit
            property var _actionModel: [
                {
                    title:      guidedController.startMissionTitle,
                    text:       guidedController.startMissionMessage,
                    action:     guidedController.actionStartMission,
                    visible:    guidedController.showStartMission
                },
                {
                    title:      guidedController.resumeMissionTitle,
                    text:       guidedController.resumeMissionMessage,
                    action:     guidedController.actionResumeMission,
                    visible:    guidedController.showResumeMission
                },
                {
                    title:      guidedController.changeAltTitle,
                    text:       guidedController.changeAltMessage,
                    action:     guidedController.actionChangeAlt,
                    visible:    guidedController.showChangeAlt
                },
                {
                    title:      guidedController.landAbortTitle,
                    text:       guidedController.landAbortMessage,
                    action:     guidedController.actionLandAbort,
                    visible:    guidedController.showLandAbort
                }
            ]
            property var _smartShotModel: [
                {
                    title:      guidedController.orbitTitle,
                    text:       guidedController.orbitMessage,
                    action:     guidedController.actionOrbit,
                    visible:    guidedController.showOrbit
                }
            ]

            model: [
                {
                    name:       guidedController.takeoffTitle,
                    iconSource: "/res/takeoff.svg",
                    action:     guidedController.actionTakeoff
                },
                {
                    name:       guidedController.landTitle,
                    iconSource: "/res/land.svg",
                    action:     guidedController.actionLand
                },
                {
                    name:       guidedController.rtlTitle,
                    iconSource: "/res/rtl.svg",
                    action:     guidedController.actionRTL
                },
                {
                    name:       guidedController.pauseTitle,
                    iconSource: "/res/Pause.svg",
                    action:     guidedController.actionPause
                },
                {
                    name:       qsTr("Action"),
                    iconSource: "/qmlimages/MapCenter.svg",
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
                guidedActionConfirm.visible = false
                guidedActionList.visible = false
                altitudeSlider.visible = false
                var action = model[index].action
                if (action === -1) {
                    if (index == 4) {
                        guidedActionList.model = _actionModel
                        guidedActionList.visible = true
                    } else if (index == 5) {
                        guidedActionList.model = _smartShotModel
                        guidedActionList.visible = true
                    }
                } else {
                    guidedController.confirmAction(action)
                }
            }
        }

        GuidedActionsController {
            id:                 guidedController
            missionController:  flyMissionController
            z:                  _flightVideoPipControl.z + 1

            onShowConfirmAction: {
                guidedActionConfirm.title =         title
                guidedActionConfirm.message =       message
                guidedActionConfirm.action =        action
                guidedActionConfirm.actionData =    actionData
                guidedActionConfirm.visible =       true
            }
        }

        Rectangle {
            id:                         guidedActionConfirm
            anchors.margins:            _margins
            anchors.bottom:             parent.bottom
            anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight * 4
            anchors.horizontalCenter:   parent.horizontalCenter
            border.color:               qgcPal.alertBorder
            border.width:               1
            width:                      confirmColumn.width  + (_margins * 4)
            height:                     confirmColumn.height + (_margins * 4)
            radius:                     ScreenTools.defaultFontPixelHeight / 2
            color:                      qgcPal.alertBackground
            opacity:                    0.9
            z:                          guidedController.z
            visible:                    false

            property alias  title:      titleText.text
            property alias  message:    messageText.text
            property int    action
            property var    actionData

            property real _margins: ScreenTools.defaultFontPixelWidth

            Column {
                id:                 confirmColumn
                anchors.margins:    _margins
                anchors.centerIn:   parent
                spacing:            _margins

                QGCLabel {
                    id:                     titleText
                    color:                  qgcPal.alertText
                    anchors.left:           slider.left
                    anchors.right:          slider.right
                    horizontalAlignment:    Text.AlignHCenter
                }

                QGCLabel {
                    id:                     messageText
                    color:                  qgcPal.alertText
                    anchors.left:           slider.left
                    anchors.right:          slider.right
                    horizontalAlignment:    Text.AlignHCenter
                    wrapMode:               Text.WordWrap
                }

                // Action confirmation control
                SliderSwitch {
                    id:             slider
                    confirmText:    qsTr("Slide to confirm")
                    width:          Math.max(implicitWidth, ScreenTools.defaultFontPixelWidth * 30)

                    onAccept: {
                        guidedActionConfirm.visible = false
                        if (altitudeSlider.visible) {
                            guidedActionConfirm.actionData = altitudeSlider.getValue()
                            altitudeSlider.visible = false
                        }
                        guidedController.executeAction(guidedActionConfirm.action, guidedActionConfirm.actionData)
                    }

                    onReject: {
                        altitudeSlider.visible = false
                        guidedActionConfirm.visible = false
                    }
                }
            }

            QGCColoredImage {
                anchors.margins:    _margins
                anchors.top:        parent.top
                anchors.right:      parent.right
                width:              ScreenTools.defaultFontPixelHeight
                height:             width
                sourceSize.height:  width
                source:             "/res/XDelete.svg"
                fillMode:           Image.PreserveAspectFit
                color:              qgcPal.alertText
                QGCMouseArea {
                    fillItem:   parent
                    onClicked: {
                        altitudeSlider.visible = false
                        guidedActionConfirm.visible = false
                    }
                }
            }
        }

        Rectangle {
            id:                         guidedActionList
            anchors.margins:            _margins
            anchors.bottom:             parent.bottom
            anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight * 4
            anchors.horizontalCenter:   parent.horizontalCenter
            width:                      actionColumn.width  + (_margins * 4)
            height:                     actionColumn.height + (_margins * 4)
            radius:                     _margins / 2
            color:                      qgcPal.window
            opacity:                    0.9
            z:                          guidedController.z
            visible:                    false

            property alias model: actionRepeater.model

            property real _margins: Math.round(ScreenTools.defaultFontPixelHeight * 0.66)

            ColumnLayout {
                id:                 actionColumn
                anchors.margins:    guidedActionList._margins
                anchors.centerIn:   parent
                spacing:            _margins

                QGCLabel {
                    text:               qsTr("Select Action")
                    Layout.alignment:   Qt.AlignHCenter
                }

                QGCFlickable {
                    contentWidth:           actionRow.width
                    contentHeight:          actionRow.height
                    Layout.minimumHeight:   actionRow.height
                    Layout.maximumHeight:   actionRow.height
                    Layout.minimumWidth:    _width
                    Layout.maximumWidth:    _width

                    property real _width: Math.min(root.width * 0.8, actionRow.width)

                    RowLayout {
                        id:         actionRow
                        spacing:    ScreenTools.defaultFontPixelHeight * 2

                        Repeater {
                            id: actionRepeater

                            ColumnLayout {
                                spacing:            ScreenTools.defaultFontPixelHeight / 2
                                visible:            modelData.visible
                                Layout.fillHeight:  true

                                QGCLabel {
                                    id:                     actionMessage
                                    text:                   modelData.text
                                    horizontalAlignment:    Text.AlignHCenter
                                    wrapMode:               Text.WordWrap
                                    Layout.minimumWidth:    _width
                                    Layout.maximumWidth:    _width
                                    Layout.fillHeight:      true

                                    property real _width: ScreenTools.defaultFontPixelWidth * 25
                                }

                                QGCButton {
                                    id:                         actionButton
                                    anchors.horizontalCenter:   parent.horizontalCenter
                                    text:                       modelData.title

                                    onClicked: {
                                        if (modelData.action === guidedController.actionChangeAlt) {
                                            altitudeSlider.visible = true
                                        }
                                        guidedActionList.visible = false
                                        guidedController.confirmAction(modelData.action)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            QGCColoredImage {
                anchors.margins:    _margins
                anchors.top:        parent.top
                anchors.right:      parent.right
                width:              ScreenTools.defaultFontPixelHeight
                height:             width
                sourceSize.height:  width
                source:             "/res/XDelete.svg"
                fillMode:           Image.PreserveAspectFit
                color:              qgcPal.text

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  guidedActionList.visible = false
                }
            }
        }

        //-- Altitude slider
        Rectangle {
            id:                 altitudeSlider
            anchors.margins:    _margins
            anchors.right:      parent.right
            anchors.topMargin:  ScreenTools.toolbarHeight + _margins
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            z:                  guidedController.z
            radius:             ScreenTools.defaultFontPixelWidth / 2
            width:              ScreenTools.defaultFontPixelWidth * 10
            color:              qgcPal.window
            visible:            false

            function setValue(value) {
                altSlider.value = value
            }

            function getValue() {
                return altSlider.value
            }

            Column {
                id:                 headerColumn
                anchors.margins:    _margins
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right

                QGCLabel {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    text:                       altSlider.value >=0 ? qsTr("Up") : qsTr("Down")
                }

                QGCLabel {
                    id:                         altField
                    anchors.horizontalCenter:   parent.horizontalCenter
                    text:                       Math.abs(altSlider.value.toFixed(1)) + " " + QGroundControl.appSettingsDistanceUnitsString
                }
            }

            QGCSlider {
                id:                 altSlider
                anchors.margins:    _margins
                anchors.top:        headerColumn.bottom
                anchors.bottom:     parent.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                orientation:        Qt.Vertical
                minimumValue:       QGroundControl.metersToAppSettingsDistanceUnits(-10)
                maximumValue:       QGroundControl.metersToAppSettingsDistanceUnits(10)
                indicatorCentered:  true
                rotation:           180

                // We want slide up to be positive values
                transform: Rotation {
                    origin.x:   altSlider.width / 2
                    origin.y:   altSlider.height / 2
                    angle:      180
                }
            }
        }
    }
}
