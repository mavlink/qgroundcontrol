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
import QGroundControl.Airmap        1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

// This is the ui overlay layer for the widgets/tools for Fly View
Item {
    id: _root

    property var   parentToolInsets
    property var   totalToolInsets:         _totalToolInsets
    property var   mapControl

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _planMasterController:  mainWindow.planMasterControllerFlyView
    property var    _missionController:     _planMasterController.missionController
    property var    _geoFenceController:    _planMasterController.geoFenceController
    property var    _rallyPointController:  _planMasterController.rallyPointController
    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75
    property rect   _centerViewport:        Qt.rect(0, 0, width, height)
    property real   _rightPanelWidth:       ScreenTools.defaultFontPixelWidth * 30

    QGCToolInsets {
        id:                     _totalToolInsets
        leftEdgeCenterInset:    toolStrip.leftInset
        leftEdgeTopInset:       toolStrip.leftInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeCenterInset:   instrumentPanel.rightInset
        rightEdgeTopInset:      instrumentPanel.rightInset
        rightEdgeBottomInset:   instrumentPanel.rightInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeCenterInset:  mapScale.centerInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }

    FlyViewMissionCompleteDialog {
        missionController:      _missionController
        geoFenceController:     _geoFenceController
        rallyPointController:   _rallyPointController
        guidedController:       _root.guidedActionsController
    }

    FlyViewInstrumentPanel {
        id:                         instrumentPanel
        anchors.margins:            _toolsMargin
        anchors.top:                parent.top
        anchors.bottom:             parent.bottom
        anchors.right:              parent.right
        width:                      _rightPanelWidth
        spacing:                    _toolsMargin
        visible:                    QGroundControl.corePlugin.options.flyView.showInstrumentPanel
        guidedActionsController:    _guidedController
        availableHeight:            parent.height - y - _toolsMargin

        property real rightInset: visible ? parent.width - x : 0
    }

    //-- Virtual Joystick
    Loader {
        id:                         virtualJoystickMultiTouch
        z:                          QGroundControl.zOrderTopMost + 1
        width:                      parent.width  - (_pipOverlay.width / 2)
        height:                     Math.min(parent.height * 0.25, ScreenTools.defaultFontPixelWidth * 16)
        visible:                    (_virtualJoystick ? _virtualJoystick.value : false) && !(_activeVehicle ? _activeVehicle.highLatencyLink : false)
        anchors.bottom:             parent.bottom
        anchors.bottomMargin:       parentToolInsets.leftEdgeBottomInset + ScreenTools.defaultFontPixelHeight * 2
        anchors.horizontalCenter:   parent.horizontalCenter
        source:                     "qrc:/qml/VirtualJoystick.qml"
        active:                     (_virtualJoystick ? _virtualJoystick.value : false) && !(_activeVehicle ? _activeVehicle.highLatencyLink : false)

        property bool centralizeThrottle:   _virtualJoystickCentralized ? _virtualJoystickCentralized.value : false
        property var  parentToolInsets:     _totalToolInsets

        property Fact _virtualJoystick:             QGroundControl.settingsManager.appSettings.virtualJoystick
        property Fact _virtualJoystickCentralized:  QGroundControl.settingsManager.appSettings.virtualJoystickCentralized
    }

    FlyViewToolStrip {
        id:                         toolStrip
        anchors.leftMargin:         _toolsMargin + parentToolInsets.leftEdgeCenterInset
        anchors.topMargin:          _toolsMargin + parentToolInsets.leftEdgeTopInset
        anchors.left:               parent.left
        anchors.top:                parent.top
        z:                          QGroundControl.zOrderWidgets
        maxHeight:                  parent.height - y - parentToolInsets.leftEdgeBottomInset - _toolsMargin
        guidedActionsController:    _guidedController
        guidedActionList:           _guidedActionList
        visible:                    !QGroundControl.videoManager.fullScreen

        onDisplayPreFlightChecklist: preFlightChecklistPopup.open()

        property real leftInset: x + width
    }

    FlyViewAirspaceIndicator {
        anchors.top:                parent.top
        anchors.topMargin:          ScreenTools.defaultFontPixelHeight * 0.25
        anchors.horizontalCenter:   parent.horizontalCenter
        z:                          QGroundControl.zOrderWidgets
        show:                       mapControl.pipState.state !== mapControl.pipState.pipState
    }

    VehicleWarnings {
        anchors.centerIn:   parent
        z:                  QGroundControl.zOrderTopMost
    }

    MapScale {
        id:                     mapScale
        anchors.leftMargin:     parentToolInsets.leftEdgeBottomInset + _toolsMargin
        anchors.bottomMargin:   parentToolInsets.bottomEdgeCenterInset + _toolsMargin
        anchors.left:           parent.left
        anchors.bottom:         parent.bottom
        mapControl:             _mapControl
        buttonsOnLeft:          true
        visible:                !ScreenTools.isTinyScreen && QGroundControl.corePlugin.options.flyView.showMapScale && mapControl.pipState.state !== mapControl.pipState.pipState

        property real centerInset: visible ? parent.height - y : 0
    }

    FlyViewPreFlightChecklistPopup {
        id: preFlightChecklistPopup
        x:  toolStrip.x + toolStrip.width + (ScreenTools.defaultFontPixelWidth * 2)
        y:  toolStrip.y
    }
}
