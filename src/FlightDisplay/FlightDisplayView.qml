/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.12
import QtQuick.Controls         2.4
import QtQuick.Dialogs          1.3
import QtQuick.Layouts          1.12

import QtLocation               5.3
import QtPositioning            5.3
import QtQuick.Window           2.2
import QtQml.Models             2.1

import QGroundControl               1.0
import QGroundControl.Airspace      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

/// Flight Display View
Item {

    PlanMasterController {
        id: _planController
        Component.onCompleted: {
            start(true /* flyView */)
            mainWindow.planMasterControllerView = _planController
        }
    }

    property bool   _mainWindowIsMap:       mapControl.pipState.state === mapControl.pipState.fullState
    property bool   _isMapDark:             _mainWindowIsMap ? mapControl.isSatelliteMap : true
    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _missionController:     _planController.missionController
    property var    _geoFenceController:    _planController.geoFenceController
    property var    _rallyPointController:  _planController.rallyPointController
    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property alias  _guidedController:      guidedActionsController
    property alias  _guidedConfirm:         guidedActionConfirm
    property alias  _guidedList:            guidedActionList
    property alias  _guidedSlider:          altitudeSlider
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75

    readonly property string _mapName:              "FlightDisplayView"

    function setPipVisibility(state) {
        _isPipVisible = state;
        QGroundControl.saveBoolGlobalSetting(_PIPVisibleKey, state)
    }

    function isInstrumentRight() {
        if(QGroundControl.corePlugin.options.instrumentWidget) {
            if(QGroundControl.corePlugin.options.instrumentWidget.source.toString().length) {
                switch(QGroundControl.corePlugin.options.instrumentWidget.widgetPosition) {
                case CustomInstrumentWidget.POS_TOP_LEFT:
                case CustomInstrumentWidget.POS_BOTTOM_LEFT:
                case CustomInstrumentWidget.POS_CENTER_LEFT:
                    return false;
                }
            }
        }
        return true;
    }

    Component.onCompleted: {
        if(QGroundControl.corePlugin.options.flyViewOverlay.toString().length) {
            flyViewOverlay.source = QGroundControl.corePlugin.options.flyViewOverlay
        }
    }

    FlyViewMissionCompleteDialog {
        missionController:      _missionController
        geoFenceController:     _geoFenceController
        rallyPointController:   _rallyPointController
        guidedController:       _guidedController
    }

    QGCMapPalette { id: mapPal; lightColors: _mainWindowIsMap ? mapControl.isSatelliteMap : true }

    Item {
        id:             _mapAndVideo
        anchors.fill:   parent

        FlightDisplayViewMap {
            id:                         mapControl
            guidedActionsController:    _guidedController
            planMasterController:       _planController
            flightWidgets:              flightDisplayViewWidgets
            rightPanelWidth:            ScreenTools.defaultFontPixelHeight * 9
            scaleState:                 (_mainWindowIsMap && flyViewOverlay.item) ? (flyViewOverlay.item.scaleState ? flyViewOverlay.item.scaleState : "bottomMode") : "bottomMode"
            mainWindowIsMap:            _mainWindowIsMap

            property var pipState: mapPipState
            QGCPipState {
                id:         mapPipState
                pipOverlay: _pipOverlay
                isDark:     _isMapDark
            }
        }

        //-- Video View
        Item {
            id:         videoControl
            visible:    QGroundControl.videoManager.hasVideo

            property var pipState: videoPipState
            QGCPipState {
                id:         videoPipState
                pipOverlay: _pipOverlay
                isDark:     true

                onWindowAboutToOpen: {
                    console.log("about to open")
                    QGroundControl.videoManager.stopVideo()
                    videoStartDelay.start()
                }

                onWindowAboutToClose: {
                    console.log("about to close")
                    QGroundControl.videoManager.stopVideo()
                    videoStartDelay.start()
                }
            }

            Timer {
              id:           videoStartDelay
              interval:     2000;
              running:      false
              repeat:       false
              onTriggered:  QGroundControl.videoManager.startVideo()
            }

            //-- Video Streaming
            FlightDisplayViewVideo {
                id:             videoStreaming
                anchors.fill:   parent
                useSmallFont:   videoControl.pipState.state !== videoControl.pipState.fullState
                visible:        QGroundControl.videoManager.isGStreamer
            }
            //-- UVC Video (USB Camera or Video Device)
            Loader {
                id:             cameraLoader
                anchors.fill:   parent
                visible:        !QGroundControl.videoManager.isGStreamer
                source:         visible ? (QGroundControl.videoManager.uvcEnabled ? "qrc:/qml/FlightDisplayViewUVC.qml" : "qrc:/qml/FlightDisplayViewDummy.qml") : ""
            }
        }

        QGCPipOverlay {
            id:                     _pipOverlay
            anchors.left:           parent.left
            anchors.bottom:         parent.bottom
            anchors.margins:        ScreenTools.defaultFontPixelHeight
            item1IsFullSettingsKey: "MainFlyWindowIsMap"
            item1:                  mapControl
            item2:                  QGroundControl.videoManager.hasVideo ? videoControl : null
            fullZOrder:             0
            pipZOrder:              QGroundControl.zOrderWidgets
        }

        MultiVehiclePanel {
            id:                         singleMultiSelector
            anchors.margins:            _toolsMargin
            anchors.top:                parent.top
            anchors.right:              parent.right
            z:                          QGroundControl.zOrderWidgets
            availableHeight:            mainWindow.availableHeight - (anchors.margins * 2)
            guidedActionsController:    _guidedController
        }

        FlightDisplayViewWidgets {
            id:                 flightDisplayViewWidgets
            z:                  QGroundControl.zOrderWidgets
            height:             availableHeight - (singleMultiSelector.visible ? singleMultiSelector.height + _toolsMargin : 0) - _toolsMargin
            anchors.left:       parent.left
            anchors.right:      altitudeSlider.visible ? altitudeSlider.left : parent.right
            anchors.bottom:     parent.bottom
            anchors.top:        singleMultiSelector.visible? singleMultiSelector.bottom : undefined
            useLightColors:     _isMapDark
            missionController:  _missionController
            visible:            singleMultiSelector.singleVehiclePanel && !QGroundControl.videoManager.fullScreen
        }

        //-------------------------------------------------------------------------
        //-- Loader helper for plugins to overlay elements over the fly view
        Loader {
            id:                 flyViewOverlay
            z:                  QGroundControl.zOrderWidgets
            visible:            !QGroundControl.videoManager.fullScreen
            height:             mainWindow.height - mainWindow.header.height
            anchors.left:       parent.left
            anchors.right:      altitudeSlider.visible ? altitudeSlider.left : parent.right
            anchors.bottom:     parent.bottom
        }

        //-- Virtual Joystick
        Loader {
            id:                         virtualJoystickMultiTouch
            z:                          QGroundControl.zOrderTopMost + 1
            width:                      parent.width  - (_pipOverlay.width / 2)
            height:                     Math.min(mainWindow.height * 0.25, ScreenTools.defaultFontPixelWidth * 16)
            visible:                    (_virtualJoystick ? _virtualJoystick.value : false) && !QGroundControl.videoManager.fullScreen && !(_activeVehicle ? _activeVehicle.highLatencyLink : false)
            anchors.bottom:             _pipOverlay.top
            anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight * 2
            anchors.horizontalCenter:   flightDisplayViewWidgets.horizontalCenter
            source:                     "qrc:/qml/VirtualJoystick.qml"
            active:                     (_virtualJoystick ? _virtualJoystick.value : false) && !(_activeVehicle ? _activeVehicle.highLatencyLink : false)

            property bool useLightColors: _isMapDark
            // The default behaviour is not centralized throttle
            property bool centralizeThrottle: _virtualJoystickCentralized ? _virtualJoystickCentralized.value : false

            property Fact _virtualJoystick:             QGroundControl.settingsManager.appSettings.virtualJoystick
            property Fact _virtualJoystickCentralized:  QGroundControl.settingsManager.appSettings.virtualJoystickCentralized
        }

        FlyViewToolStrip {
            id:                         toolStrip
            anchors.leftMargin:         isInstrumentRight() ? _toolsMargin : undefined
            anchors.left:               isInstrumentRight() ? _mapAndVideo.left : undefined
            anchors.rightMargin:        isInstrumentRight() ? undefined : ScreenTools.defaultFontPixelWidth
            anchors.right:              isInstrumentRight() ? undefined : _mapAndVideo.right
            anchors.topMargin:          _toolsMargin
            anchors.top:                parent.top
            z:                          QGroundControl.zOrderWidgets
            maxHeight:                  parent.height - toolStrip.y + (_pipOverlay.visible ? (_pipOverlay.y - parent.height) : 0)
            guidedActionsController:    _guidedController
            guidedActionList:           _guidedList
            usePreFlightChecklist:      preFlightChecklistPopup.useChecklist
            visible:                    (_activeVehicle ? _activeVehicle.guidedModeSupported : true) && !QGroundControl.videoManager.fullScreen

            onDisplayPreFlightChecklist: preFlightChecklistPopup.open()
        }

        GuidedActionsController {
            id:                 guidedActionsController
            missionController:  _missionController
            confirmDialog:      _guidedConfirm
            actionList:         _guidedList
            altitudeSlider:     _guidedSlider
        }

        GuidedActionConfirm {
            id:                         guidedActionConfirm
            anchors.margins:            _margins
            anchors.bottom:             parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            z:                          QGroundControl.zOrderTopMost
            guidedController:           _guidedController
            altitudeSlider:             _guidedSlider
        }

        GuidedActionList {
            id:                         guidedActionList
            anchors.margins:            _margins
            anchors.bottom:             parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            z:                          QGroundControl.zOrderTopMost
            guidedController:           _guidedController
        }

        //-- Altitude slider
        GuidedAltitudeSlider {
            id:                 altitudeSlider
            anchors.margins:    _margins
            anchors.right:      parent.right
            anchors.topMargin:  ScreenTools.toolbarHeight + _margins
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            z:                  QGroundControl.zOrderTopMost
            radius:             ScreenTools.defaultFontPixelWidth / 2
            width:              ScreenTools.defaultFontPixelWidth * 10
            color:              qgcPal.window
            visible:            false
        }
    }

    FlyViewAirspaceIndicator {
        anchors.top:                parent.top
        anchors.topMargin:          ScreenTools.defaultFontPixelHeight * 0.25
        anchors.horizontalCenter:   parent.horizontalCenter
        show:                       _mainWindowIsMap
    }

    FlyViewPreFlightChecklistPopup {
        id: preFlightChecklistPopup
        x:  toolStrip.x + toolStrip.width + (ScreenTools.defaultFontPixelWidth * 2)
        y:  toolStrip.y
    }
}
