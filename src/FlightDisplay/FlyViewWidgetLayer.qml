/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models

import QGroundControl
import QGroundControl.Controls

import QGroundControl.FlightDisplay
import QGroundControl.FlightMap

// This is the ui overlay layer for the widgets/tools for Fly View
Item {
    id: _root

    QGCPalette { id: qgcPal }

    property var    parentToolInsets
    property var    totalToolInsets:        _totalToolInsets
    property var    mapControl
    property bool   isViewer3DOpen:         false

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _planMasterController:  globals.planMasterControllerFlyView
    property var    _missionController:     _planMasterController.missionController
    property var    _geoFenceController:    _planMasterController.geoFenceController
    property var    _rallyPointController:  _planMasterController.rallyPointController
    property var    _guidedController:      globals.guidedControllerFlyView
    property var    _guidedValueSlider:     guidedValueSlider
    property var    _widgetLayer:           widgetLayer
    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75
    property rect   _centerViewport:        Qt.rect(0, 0, width, height)
    property real   _rightPanelWidth:       ScreenTools.defaultFontPixelWidth * 30
    property var    _mapControl:            mapControl
    property real   _widgetMargin:          ScreenTools.defaultFontPixelWidth * 0.75
    
    property real   _layoutSpacing:         ScreenTools.defaultFontPixelWidth
    property real   _layoutMargin:          ScreenTools.defaultFontPixelWidth
    property bool   _showSingleVehicleUI:   true
    property var    _activePmc:             mapControl && mapControl._visualsPlanMasterController ? mapControl._visualsPlanMasterController : _planMasterController
    

    property bool utmspActTrigger
    property real   leftBottomPanelWidth:   0

    QGCToolInsets {
        id:                     _totalToolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    virtualJoystickMultiTouch.visible ? virtualJoystickMultiTouch.leftEdgeBottomInset : parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      Math.max(topRightPanel.rightEdgeTopInset, toolStrip.rightEdgeTopInset)
        rightEdgeCenterInset:   Math.max(topRightPanel.rightEdgeCenterInset, toolStrip.rightEdgeCenterInset)
        rightEdgeBottomInset:   bottomRightRowLayout.rightEdgeBottomInset
        topEdgeLeftInset:       0
        topEdgeCenterInset:     Math.max(mapScale.topEdgeCenterInset, bottomActionButtonsContainer.visible ? bottomActionButtonsContainer.height + bottomActionButtonsContainer.anchors.topMargin : 0)
        topEdgeRightInset:      Math.max(topRightPanel.topEdgeRightInset, toolStrip.topEdgeRightInset)
        bottomEdgeLeftInset:    virtualJoystickMultiTouch.visible ? virtualJoystickMultiTouch.bottomEdgeLeftInset : parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  bottomRightRowLayout.bottomEdgeCenterInset
        bottomEdgeRightInset:   virtualJoystickMultiTouch.visible ? virtualJoystickMultiTouch.bottomEdgeRightInset : bottomRightRowLayout.bottomEdgeRightInset
    }

    

    FlyViewTopRightPanel {
        id:                     topRightPanel
        anchors.top:            parent.top
        anchors.right:          parent.right
        maximumHeight:          parent.height - (bottomRightRowLayout.height + _margins * 4)

        property real topEdgeRightInset:    height + _layoutMargin
        property real rightEdgeTopInset:    width + _layoutMargin
        property real rightEdgeCenterInset: rightEdgeTopInset
    }

    FlyViewTopRightColumnLayout {
        id:                 topRightColumnLayout
        anchors.top:        parent.top
        anchors.bottom:     bottomRightRowLayout.top
        anchors.right:      parent.right
        spacing:            _layoutSpacing
        visible:           !topRightPanel.visible

        property real topEdgeRightInset:    childrenRect.height + _layoutMargin
        property real rightEdgeTopInset:    width + _layoutMargin
        property real rightEdgeCenterInset: rightEdgeTopInset
    }

    FlyViewBottomRightRowLayout {
        id:                 bottomRightRowLayout
        anchors.bottom:     parent.bottom
        anchors.right:      parent.right
        spacing:            _layoutSpacing
        clip:               true
        width:              Math.min(implicitWidth, _root.width - leftBottomPanelWidth - _layoutMargin * 3)

        property real bottomEdgeRightInset:     height + _layoutMargin
        property real bottomEdgeCenterInset:    bottomEdgeRightInset
        property real rightEdgeBottomInset:     width + _layoutMargin
    }

    FlyViewMissionCompleteDialog {
        missionController:      _missionController
        geoFenceController:     _geoFenceController
        rallyPointController:   _rallyPointController
    }

    GuidedActionConfirm {
        anchors.top:                parent.top
        anchors.horizontalCenter:   parent.horizontalCenter
        z:                          QGroundControl.zOrderTopMost
        guidedController:           _guidedController
        guidedValueSlider:          _guidedValueSlider
        utmspSliderTrigger:         utmspActTrigger
    }

    // Removed: Pre-takeoff waypoint confirmation popup; TAKEOFF now auto-uploads (if dirty) and then takes off.

    //-- Virtual Joystick
    Loader {
        id:                         virtualJoystickMultiTouch
        z:                          QGroundControl.zOrderTopMost + 1
        anchors.right:              parent.right
        anchors.rightMargin:        anchors.leftMargin
        height:                     Math.min(parent.height * 0.25, ScreenTools.defaultFontPixelWidth * 16)
        visible:                    _virtualJoystickEnabled && !QGroundControl.videoManager.fullScreen && !(_activeVehicle ? _activeVehicle.usingHighLatencyLink : false)
        anchors.bottom:             parent.bottom
        anchors.bottomMargin:       bottomLoaderMargin
        anchors.left:               parent.left   
        anchors.leftMargin:         ((toolStrip.x < parent.width / 2) && (y <= toolStrip.y + toolStrip.height)) ? (toolStrip.width * 1.05 + toolStrip.x) : (toolStrip.width / 2)
        source:                     "qrc:/qml/QGroundControl/FlightDisplay/VirtualJoystick.qml"
        active:                     _virtualJoystickEnabled && !(_activeVehicle ? _activeVehicle.usingHighLatencyLink : false)

        property real bottomEdgeLeftInset:     parent.height-y
        property bool autoCenterThrottle:      QGroundControl.settingsManager.appSettings.virtualJoystickAutoCenterThrottle.rawValue
        property bool leftHandedMode:          QGroundControl.settingsManager.appSettings.virtualJoystickLeftHandedMode.rawValue
        property bool _virtualJoystickEnabled: QGroundControl.settingsManager.appSettings.virtualJoystick.rawValue
        property real bottomEdgeRightInset:    parent.height-y
        property var  _pipViewMargin:          _pipView.visible ? parentToolInsets.bottomEdgeLeftInset + ScreenTools.defaultFontPixelHeight * 2 : 
                                               bottomRightRowLayout.height + ScreenTools.defaultFontPixelHeight * 1.5

        property var  bottomLoaderMargin:      _pipViewMargin >= parent.height / 2 ? parent.height / 2 : _pipViewMargin

        // Width is difficult to access directly hence this hack which may not work in all circumstances
        property real leftEdgeBottomInset:  visible ? bottomEdgeLeftInset + width/18 - ScreenTools.defaultFontPixelHeight*2 : 0
        property real rightEdgeBottomInset: visible ? bottomEdgeRightInset + width/18 - ScreenTools.defaultFontPixelHeight*2 : 0
        property real rootWidth:            _root.width
        property var  itemX:                virtualJoystickMultiTouch.x   // real X on screen

        onRootWidthChanged: virtualJoystickMultiTouch.status == Loader.Ready && visible ? virtualJoystickMultiTouch.item.uiTotalWidth = rootWidth : undefined
        onItemXChanged:     virtualJoystickMultiTouch.status == Loader.Ready && visible ? virtualJoystickMultiTouch.item.uiRealX = itemX : undefined

        //Loader status logic
        onLoaded: {
            if (virtualJoystickMultiTouch.visible) {
                virtualJoystickMultiTouch.item.calibration = true 
                virtualJoystickMultiTouch.item.uiTotalWidth = rootWidth
                virtualJoystickMultiTouch.item.uiRealX = itemX
            } else {
                virtualJoystickMultiTouch.item.calibration = false
            }
        }
    }

    FlyViewToolStrip {
        id:                     toolStrip
        anchors.right:          parent.right
        anchors.top:            parent.top
        z:                      QGroundControl.zOrderWidgets
        maxHeight:              parent.height - y - parentToolInsets.bottomEdgeRightInset - _toolsMargin
        visible:                !QGroundControl.videoManager.fullScreen

        onDisplayPreFlightChecklist: {
            if (!preFlightChecklistLoader.active) {
                preFlightChecklistLoader.active = true
            }
            preFlightChecklistLoader.item.open()
        }

        property real topEdgeRightInset:    visible ? y + height : 0
        property real rightEdgeTopInset:    width + _toolsMargin
        property real rightEdgeCenterInset: rightEdgeTopInset
    }

    VehicleWarnings {
        anchors.centerIn:   parent
        z:                  QGroundControl.zOrderTopMost
    }

    // Top-center neon action buttons with outer border frame
    Rectangle {
        id:                 bottomActionButtonsContainer
        anchors.top:        parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 2
        width:              bottomActionButtons.width + ScreenTools.defaultFontPixelWidth * 2
        height:             bottomActionButtons.height + ScreenTools.defaultFontPixelHeight * 0.8
        color:              "transparent"
        border.width:       2
        border.color:       qgcPal.brandingBlue
        radius:             6
        z:                  QGroundControl.zOrderWidgets
        visible:            !QGroundControl.videoManager.fullScreen

        // Solid dark background to block map behind
        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.toolbarBackground
            radius:         parent.radius
        }

        // Outer glow effect
        Rectangle {
            anchors.fill:   parent
            anchors.margins: 1
            color:          qgcPal.brandingBlue
            opacity:        0.05
            radius:         parent.radius - 1
        }

        Row {
            id:                 bottomActionButtons
            anchors.centerIn:   parent
            spacing:            ScreenTools.defaultFontPixelWidth

            property var _guidedController: globals.guidedControllerFlyView
            property bool takeoffEnabled: _guidedController.showTakeoff
            property bool rthEnabled:     _guidedController.showRTL

        Rectangle {
            id:             takeoffButton
            width:          ScreenTools.defaultFontPixelWidth * 12
            height:         ScreenTools.defaultFontPixelHeight * 2.5
            visible:        true
            color:          qgcPal.toolbarBackground
            border.width:   0
            border.color:   bottomActionButtons.takeoffEnabled ? "#00FF88" : "#00CC77"
            radius:         4

            Rectangle {
                anchors.fill:   parent
                anchors.margins: 0
                color:          "#00FF88"
                opacity:        bottomActionButtons.takeoffEnabled ? (takeoffMouseArea.containsMouse ? 0.22 : 0.16) : 0.08
                radius:         parent.radius - 1
            }

            Row {
                anchors.centerIn: parent
                spacing: ScreenTools.defaultFontPixelWidth * 0.5

                Image {
                    source:         "/res/takeoff.svg"
                    width:          ScreenTools.defaultFontPixelHeight * 1.2
                    height:         ScreenTools.defaultFontPixelHeight * 1.2
                    anchors.verticalCenter: parent.verticalCenter
                    fillMode:       Image.PreserveAspectFit
                }

                QGCLabel {
                    text:           "TAKEOFF"
                    color:          "#00FF88"
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold:      true
                    opacity:        bottomActionButtons.takeoffEnabled ? 1.0 : 0.6
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            QGCMouseArea {
                id:             takeoffMouseArea
                anchors.fill:   parent
                enabled:        bottomActionButtons.takeoffEnabled
                hoverEnabled:   true
                cursorShape:    Qt.PointingHandCursor
                onClicked: {
                    _guidedController.closeAll()
                    _guidedController.confirmAction(_guidedController.actionTakeoff)
                }
            }
        }

        // Vertical divider between buttons
        Item {
            width:          ScreenTools.defaultFontPixelWidth * 1.2
            height:         bottomActionButtons.height
            Rectangle {
                anchors.centerIn: parent
                width: 1
                height: parent.height * 0.65
                radius: 1
                color:  qgcPal.brandingBlue
                opacity: 0.6
            }
        }

        Rectangle {
            id:             returnButton
            width:          ScreenTools.defaultFontPixelWidth * 10
            height:         ScreenTools.defaultFontPixelHeight * 2.5
            visible:        true
            color:          qgcPal.toolbarBackground
            border.width:   0
            border.color:   bottomActionButtons.rthEnabled ? qgcPal.brandingBlue : "#0088A8"
            radius:         4

            Rectangle {
                anchors.fill:   parent
                anchors.margins: 0
                color:          qgcPal.brandingBlue
                opacity:        bottomActionButtons.rthEnabled ? (returnMouseArea.containsMouse ? 0.14 : 0.10) : 0.06
                radius:         parent.radius - 1
            }

            Row {
                anchors.centerIn: parent
                spacing: ScreenTools.defaultFontPixelWidth * 0.5

                Image {
                    source:         "/res/rtl.svg"
                    width:          ScreenTools.defaultFontPixelHeight * 1.2
                    height:         ScreenTools.defaultFontPixelHeight * 1.2
                    anchors.verticalCenter: parent.verticalCenter
                    fillMode:       Image.PreserveAspectFit
                }

                QGCLabel {
                    text:           "RTH"
                    color:          qgcPal.brandingBlue
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold:      true
                    opacity:        bottomActionButtons.rthEnabled ? 0.9 : 0.5
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            QGCMouseArea {
                id:             returnMouseArea
                anchors.fill:   parent
                enabled:        bottomActionButtons.rthEnabled
                hoverEnabled:   true
                cursorShape:    Qt.PointingHandCursor
                onClicked: {
                    _guidedController.closeAll()
                    _guidedController.confirmAction(_guidedController.actionRTL)
                }
            }
        }
        }
    }

    

    MapScale {
        id:                 mapScale
        anchors.right:      toolStrip.left
        anchors.top:        parent.top
        mapControl:         _mapControl
        buttonsOnLeft:      false
        zoomButtonsVisible: false
        autoHide:           true
        visible:            !ScreenTools.isTinyScreen && QGroundControl.corePlugin.options.flyView.showMapScale && !isViewer3DOpen && mapControl.pipState.state === mapControl.pipState.fullState

        property real topEdgeCenterInset: visible ? y + height : 0
    }

    Loader {
        id: preFlightChecklistLoader
        sourceComponent: preFlightChecklistPopup
        active: false
    }

    Component {
        id: preFlightChecklistPopup
        FlyViewPreFlightChecklistPopup {
        }
    }
}
