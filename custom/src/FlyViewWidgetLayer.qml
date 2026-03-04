/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
import QtQuick
import Custom.Widgets
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QGroundControl
import QGroundControl.Controls
import QGroundControl.Controllers
import QGroundControl.FactSystem
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Vehicle
Item {
    id: _root
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
    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75
    property rect   _centerViewport:        Qt.rect(0, 0, width, height)
    property real   _rightPanelWidth:       ScreenTools.defaultFontPixelWidth * 30
    property alias  _gripperMenu:           gripperOptions
    property real   _layoutMargin:          ScreenTools.defaultFontPixelWidth * 0.75
    property real   _layoutSpacing:         ScreenTools.defaultFontPixelWidth
    property bool   _showSingleVehicleUI:   true
    property bool   utmspActTrigger
    
    function openPlanView() {
        console.log("MainFlyView: Opening Plan View from New Mission button...");
        
        
        if (QGroundControl.mainWindow) {
            console.log("Success: Opening Plan View via QGroundControl.mainWindow");
            QGroundControl.mainWindow.showPlanView();
            return true;
        }
        
        
        if (typeof globals !== 'undefined' && globals.mainWindow) {
            console.log("Success: Opening Plan View via globals.mainWindow");
            globals.mainWindow.showPlanView();
            return true;
        }
        
    
        if (typeof mainWindow !== 'undefined') {
            console.log("Success: Opening Plan View via mainWindow");
            mainWindow.showPlanView();
            return true;
        }
        
        console.error("Error: Cannot find mainWindow object!");
        return false;
    }
    QGCToolInsets {  
    id: _totalToolInsets  
    leftEdgeTopInset:       toolStrip.leftEdgeTopInset  
    leftEdgeCenterInset:    toolStrip.leftEdgeCenterInset  
    leftEdgeBottomInset:    virtualJoystickMultiTouch.visible ? virtualJoystickMultiTouch.leftEdgeBottomInset : parentToolInsets.leftEdgeBottomInset  
    rightEdgeTopInset:      topRightPanel.rightEdgeTopInset  
    rightEdgeCenterInset:   topRightPanel.rightEdgeCenterInset  
    rightEdgeBottomInset:   Math.max(bottomRightRowLayout.rightEdgeBottomInset, customBottomInfoPanel.rightEdgeBottomInset)  
    topEdgeLeftInset:       toolStrip.topEdgeLeftInset  
    topEdgeCenterInset:     mapScale.topEdgeCenterInset  
    topEdgeRightInset:      topRightPanel.topEdgeRightInset  
    bottomEdgeLeftInset:    virtualJoystickMultiTouch.visible ? virtualJoystickMultiTouch.bottomEdgeLeftInset : parentToolInsets.bottomEdgeLeftInset  
    bottomEdgeCenterInset:  Math.max(bottomRightRowLayout.bottomEdgeCenterInset, customBottomInfoPanel.bottomEdgeCenterInset)  
    bottomEdgeRightInset:   Math.max(  
        virtualJoystickMultiTouch.visible ? virtualJoystickMultiTouch.bottomEdgeRightInset : bottomRightRowLayout.bottomEdgeRightInset,  
        customBottomInfoPanel.bottomEdgeRightInset  
    )  
}
    FlyViewTopRightPanel {
        id:                     topRightPanel
        anchors.top:            parent.top
        anchors.right:          parent.right
        anchors.topMargin:      _layoutMargin
        anchors.rightMargin:    _layoutMargin
        maximumHeight:          parent.height - (bottomRightRowLayout.height + _margins * 5)
        property real topEdgeRightInset:    height + _layoutMargin
        property real rightEdgeTopInset:    width + _layoutMargin
        property real rightEdgeCenterInset: rightEdgeTopInset
    }
    FlyViewTopRightColumnLayout {
        id:                 topRightColumnLayout
        anchors.margins:    _layoutMargin
        anchors.top:        parent.top
        anchors.bottom:     bottomRightRowLayout.top
        anchors.right:      parent.right
        spacing:            _layoutSpacing
        visible:            !topRightPanel.visible
        property real topEdgeRightInset:    childrenRect.height + _layoutMargin
        property real rightEdgeTopInset:    width + _layoutMargin
        property real rightEdgeCenterInset: rightEdgeTopInset
    }
    // Это гавно отвечает а панель, если я не разберусь с телеметрией я сам скоро пойду напнель....
    FlyViewBottomRightRowLayout {  
    id:                 bottomRightRowLayout  
    anchors.bottom:     parent.bottom  
    anchors.bottomMargin: _layoutMargin + 20  
    anchors.right:      newMissionButton.left  
    anchors.rightMargin: 65  
    spacing:            _layoutSpacing  
    visible: false      // отключил пока панель  
  
    property real bottomEdgeRightInset:     height + _layoutMargin  
    property real bottomEdgeCenterInset:    bottomEdgeRightInset  
    property real rightEdgeBottomInset:     width + _layoutMargin  
}
    // создаем новую панель телеметрии
    CustomBottomInfoPanel {  
        id: customBottomInfoPanel  
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth
        anchors.bottomMargin: ScreenTools.defaultFontPixelWidth * 1.5
        z: QGroundControl.zOrderTopMost - 1
        
        width: ScreenTools.defaultFontPixelWidth * 80  
        height: ScreenTools.defaultFontPixelHeight * 6.5  
        
        factList: [  
            _activeVehicle.battery.remainingPercent,  
            _activeVehicle.altitudeRelative,  
            _activeVehicle.groundSpeed,  
            _activeVehicle.distanceToHome,  
            _activeVehicle.flightTime,  
            _activeVehicle.gps.hdop  
        ]  
        
        property real bottomEdgeRightInset: height + _layoutMargin  
        property real bottomEdgeCenterInset: bottomEdgeRightInset  
        property real rightEdgeBottomInset: width + _layoutMargin  
    }

    FlyViewMissionCompleteDialog {
        missionController:      _missionController
        geoFenceController:     _geoFenceController
        rallyPointController:   _rallyPointController
    }
    GuidedActionConfirm {
        anchors.margins:            _toolsMargin
        anchors.top:                parent.top
        anchors.horizontalCenter:   parent.horizontalCenter
        z:                          QGroundControl.zOrderTopMost
        guidedController:           _guidedController
        guidedValueSlider:          _guidedValueSlider
        utmspSliderTrigger:         utmspActTrigger
    }
    Loader {
        id:                             virtualJoystickMultiTouch
        z:                              QGroundControl.zOrderTopMost + 1
        anchors.right:                  parent.right
        anchors.rightMargin:            anchors.leftMargin
        height:                         Math.min(parent.height * 0.25, ScreenTools.defaultFontPixelWidth * 16)
        visible:                        _virtualJoystickEnabled && !QGroundControl.videoManager.fullScreen && !(_activeVehicle ? _activeVehicle.usingHighLatencyLink : false)
        anchors.bottom:                 parent.bottom
        anchors.bottomMargin:           bottomLoaderMargin
        anchors.left:                   parent.left
        anchors.leftMargin:             ( y > toolStrip.y + toolStrip.height ? toolStrip.width / 2 : toolStrip.width * 1.05 + toolStrip.x)
        source:                         "qrc:/qml/QGroundControl/FlightDisplay/VirtualJoystick.qml"
        active:                         _virtualJoystickEnabled && !(_activeVehicle ? _activeVehicle.usingHighLatencyLink : false)
        property real bottomEdgeLeftInset:     parent.height-y
        property bool autoCenterThrottle:      QGroundControl.settingsManager.appSettings.virtualJoystickAutoCenterThrottle.rawValue
        property bool leftHandedMode:          QGroundControl.settingsManager.appSettings.virtualJoystickLeftHandedMode.rawValue
        property bool _virtualJoystickEnabled: QGroundControl.settingsManager.appSettings.virtualJoystick.rawValue
        property real bottomEdgeRightInset:    parent.height-y
        property var  _pipViewMargin:          _pipView.visible ? parentToolInsets.bottomEdgeLeftInset + ScreenTools.defaultFontPixelHeight * 2 :
                                                       bottomRightRowLayout.height + ScreenTools.defaultFontPixelHeight * 1.5
        property var  bottomLoaderMargin:      _pipViewMargin >= parent.height / 2 ? parent.height / 2 : _pipViewMargin
        property real leftEdgeBottomInset:  visible ? bottomEdgeLeftInset + width/18 - ScreenTools.defaultFontPixelHeight*2 : 0
        property real rightEdgeBottomInset: visible ? bottomEdgeRightInset + width/18 - ScreenTools.defaultFontPixelHeight*2 : 0
        property real rootWidth:            _root.width
        property var  itemX:                virtualJoystickMultiTouch.x
        onRootWidthChanged: virtualJoystickMultiTouch.status == Loader.Ready && visible ? virtualJoystickMultiTouch.item.uiTotalWidth = rootWidth : undefined
        onItemXChanged:     virtualJoystickMultiTouch.status == Loader.Ready && visible ? virtualJoystickMultiTouch.item.uiRealX = itemX : undefined
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
        anchors.leftMargin:     _toolsMargin + parentToolInsets.leftEdgeCenterInset
        anchors.topMargin:      _toolsMargin + parentToolInsets.topEdgeLeftInset
        anchors.left:           parent.left
        anchors.top:            parent.top
        z:                      QGroundControl.zOrderWidgets
        maxHeight:              parent.height - y - parentToolInsets.bottomEdgeLeftInset - _toolsMargin
        visible:                !QGroundControl.videoManager.fullScreen
        onDisplayPreFlightChecklist: {
            if (!preFlightChecklistLoader.active) {
                preFlightChecklistLoader.active = true
            }
            preFlightChecklistLoader.item.open()
        }
        
        property bool showPlanFlight: false
        
        property real topEdgeLeftInset:     visible ? y + height : 0
        property real leftEdgeTopInset:     visible ? x + width : 0
        property real leftEdgeCenterInset:  leftEdgeTopInset
    }
    GripperMenu {
        id: gripperOptions
    }
    VehicleWarnings {
        anchors.centerIn:   parent
        z:                  QGroundControl.zOrderTopMost
    }
    MapScale {
        id:                 mapScale
        anchors.margins:    _toolsMargin
        anchors.left:       toolStrip.right
        anchors.top:        parent.top
        mapControl:         _mapControl
        buttonsOnLeft:      true
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
    QGCButton {
        id: newMissionButton
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth
        anchors.bottomMargin: ScreenTools.defaultFontPixelWidth * 3
        z: QGroundControl.zOrderTopMost - 1
        
        
        width: ScreenTools.defaultFontPixelHeight * 5.5  
        height: width
        
        
        iconSource: "qrc:/qmlimages/Plan.svg"
        text: qsTr("New mission")
        
        
        primary: true
        pointSize: ScreenTools.defaultFontPointSize * 1.2  
        heightFactor: 0.3
        
        
        contentItem: ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight * 0.25
            
            QGCColoredImage {
                Layout.alignment: Qt.AlignHCenter
                source: newMissionButton.iconSource
                height: ScreenTools.defaultFontPixelHeight * 2
                width: height
                color: newMissionButton._showHighlight ? qgcPal.buttonHighlightText :
                       (newMissionButton.primary ? qgcPal.primaryButtonText : qgcPal.buttonText)
                fillMode: Image.PreserveAspectFit
                sourceSize.height: height
                visible: newMissionButton.iconSource !== ""
            }
            
            QGCLabel {
                id: textItem
                Layout.alignment: Qt.AlignHCenter
                text: newMissionButton.text
                font.pointSize: newMissionButton.pointSize
                font.family: ScreenTools.normalFontFamily
                font.weight: newMissionButton.fontWeight
                color: newMissionButton._showHighlight ? qgcPal.buttonHighlightText :
                       (newMissionButton.primary ? qgcPal.primaryButtonText : qgcPal.buttonText)
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: newMissionButton.text !== ""
                
                
                Layout.maximumWidth: newMissionButton.width - (newMissionButton._horizontalPadding * 2)
            }
        }
        
        onClicked: {
            console.log("New Mission button clicked - opening Plan View");
            
            var success = _root.openPlanView();
            
            if (success) {
                console.log("Plan View opened successfully");
                
                if (_missionController) {
                    console.log("Clearing current mission");
                    _missionController.removeAll();
                }
            } else {
                console.error("Failed to open Plan View");
            }
        }

        QGCPalette { id: qgcPal; colorGroupEnabled: newMissionButton.enabled }
    }

    Item {
        id: newMissionButtonPlaceholder
        property real bottomEdgeRightInset: newMissionButton.height + ScreenTools.defaultFontPixelWidth
        property real rightEdgeBottomInset: newMissionButton.width + ScreenTools.defaultFontPixelWidth
    }
}