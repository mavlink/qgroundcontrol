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
import QGroundControl.FlyView
import QGroundControl.FlightMap
import QGroundControl.GeoMap
import QGroundControl.Viewer3D

// This is the ui overlay layer for the widgets/tools for Fly View
Item {
    id: _root

    property rect   pipViewRect:            Qt.rect(0, height, 0, 0)
    property var    occluders:              _occluders
    property var    mapControl
    property var    viewer3DCameraController

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _planMasterController:  globals.planMasterControllerFlyView
    property var    _missionController:     _planMasterController.missionController
    property var    _geoFenceController:    _planMasterController.geoFenceController
    property var    _rallyPointController:  _planMasterController.rallyPointController
    property var    _guidedController:      globals.guidedControllerFlyView
    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75
    property real   _rightPanelWidth:       ScreenTools.defaultFontPixelWidth * 30
    property bool   _layoutSpacing:         ScreenTools.defaultFontPixelWidth
    property bool   _showSingleVehicleUI:   true

    FlyViewOccluders {
        id:         _occluders
        pipView:    pipViewRect
        toolStrip:  toolStrip.visible ? Qt.rect(toolStrip.x, toolStrip.y, toolStrip.width, toolStrip.height) : Qt.rect(0, 0, 0, 0)
        mapScale:   mapScaleRow.coversMap ? Qt.rect(mapScaleRow.x, mapScaleRow.y, mapScaleRow.width, mapScaleRow.height) : Qt.rect(mapScaleRow.x, mapScaleRow.y, 0, 0)
        topRight: {
            if (topRightPanel.visible) {
                return Qt.rect(topRightPanel.x, topRightPanel.y, topRightPanel.width, topRightPanel.height)
            }
            const column = topRightColumnLayout
            return column.visible ? Qt.rect(column.x, column.y, column.width, column.childrenRect.height) : Qt.rect(_root.width, 0, 0, 0)
        }
        bottomRight: {
            const row = bottomRightRowLayout
            return row.visible ? Qt.rect(row.x, row.y, row.width, row.height) : Qt.rect(_root.width, _root.height, 0, 0)
        }
        // The loader spans the full width but only the stick pads at each end cover the map
        virtualJoystickLeft: {
            const vj = virtualJoystickMultiTouch
            return vj.visible ? Qt.rect(vj.x, vj.y, vj.height, vj.height) : Qt.rect(0, _root.height, 0, 0)
        }
        virtualJoystickRight: {
            const vj = virtualJoystickMultiTouch
            return vj.visible ? Qt.rect(vj.x + vj.width - vj.height, vj.y, vj.height, vj.height) : Qt.rect(_root.width, _root.height, 0, 0)
        }
    }

    FlyViewTopRightPanel {
        id:                     topRightPanel
        anchors.top:            parent.top
        anchors.right:          parent.right
        maximumHeight:          parent.height - (bottomRightRowLayout.height + _margins * 4)
    }

    FlyViewTopRightColumnLayout {
        id:                 topRightColumnLayout
        anchors.top:        parent.top
        anchors.right:      parent.right
        spacing:            _layoutSpacing
        visible:           !topRightPanel.visible
    }

    FlyViewBottomRightRowLayout {
        id:                 bottomRightRowLayout
        objectName:         "flyViewBottomRightRowLayout"
        anchors.bottom:     parent.bottom
        anchors.right:      parent.right
        spacing:            _layoutSpacing
    }

    FlyViewMissionCompleteDialog {
        missionController:      _missionController
        geoFenceController:     _geoFenceController
        rallyPointController:   _rallyPointController
    }

    // Prevent the map's PinchHandler from stealing touch grabs from the joystick pads (issue #13450)
    Binding {
        target:   mapControl
        property: "pinchZoomDisabledByVirtualJoysticks"
        value:    virtualJoystickMultiTouch.visible && virtualJoystickMultiTouch.item && virtualJoystickMultiTouch.item.stickActive
    }

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
        anchors.leftMargin:         ( y > toolStrip.y + toolStrip.height ? toolStrip.width / 2 : toolStrip.width * 1.05 + toolStrip.x)
        source:                     "qrc:/qml/QGroundControl/FlyView/VirtualJoystick.qml"
        active:                     _virtualJoystickEnabled && !(_activeVehicle ? _activeVehicle.usingHighLatencyLink : false)

        property bool autoCenterThrottle:      QGroundControl.settingsManager.appSettings.virtualJoystickAutoCenterThrottle.rawValue
        property bool leftHandedMode:          QGroundControl.settingsManager.appSettings.virtualJoystickLeftHandedMode.rawValue
        property bool _virtualJoystickEnabled: QGroundControl.settingsManager.appSettings.virtualJoystick.rawValue
        property var  _pipViewMargin:          pipViewRect.height > 0 ? _root.height - pipViewRect.y + ScreenTools.defaultFontPixelHeight * 2 :
                                               bottomRightRowLayout.height + ScreenTools.defaultFontPixelHeight * 1.5

        property var  bottomLoaderMargin:      _pipViewMargin >= parent.height / 2 ? parent.height / 2 : _pipViewMargin

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
        anchors.left:           parent.left
        anchors.top:            parent.top
        z:                      QGroundControl.zOrderWidgets
        maxHeight:              pipViewRect.y - y - _toolsMargin
        visible:                !QGroundControl.videoManager.fullScreen

        onDisplayPreFlightChecklist: {
            if (!preFlightChecklistLoader.active) {
                preFlightChecklistLoader.active = true
            }
            preFlightChecklistLoader.item.open()
        }
    }

    VehicleWarnings {
        anchors.centerIn:   parent
        z:                  QGroundControl.zOrderTopMost
    }

    Row {
        id:                 mapScaleRow
        anchors.left:       toolStrip.right
        anchors.leftMargin: _toolsMargin
        anchors.top:        parent.top
        spacing:            _toolsMargin

        // MapScale auto-hides by fading opacity, not visibility
        property bool coversMap: geoMapControls.visible || (mapScale.visible && mapScale.opacity > 0)

        // Google Earth-style camera controls, GeoMap engine only
        FlyViewGeoMapControls {
            id:      geoMapControls
            geoMap:  mapControl && mapControl.geoMap ? mapControl.geoMap : null
            visible: !!geoMap && !ScreenTools.isTinyScreen && QGCViewer3DManager.displayMode !== QGCViewer3DManager.View3D && mapControl.pipState.state === mapControl.pipState.fullState
        }

        MapScale {
            id:                     mapScale
            anchors.verticalCenter: parent.verticalCenter
            mapControl:             _mapControl
            autoHide:               true
            // geoMap check: scale is ill-defined under the GeoMap engine's 3D
            // tilt; isTopDown keeps it hidden until the 3D->2D tilt animation lands
            visible:                !ScreenTools.isTinyScreen && QGroundControl.corePlugin.options.flyView.showMapScale && QGCViewer3DManager.displayMode !== QGCViewer3DManager.View3D && mapControl && mapControl.pipState.state === mapControl.pipState.fullState &&
                                    (!mapControl.geoMap || (mapControl.geoMap.camera.mode === GeoMapCamera.Mode2D && mapControl.geoMap.camera.isTopDown))
        }
    }

    Viewer3DScaleBar {
        objectName:         "viewer3DScaleBar"
        anchors.left:       toolStrip.right
        anchors.leftMargin: _toolsMargin
        anchors.top:        parent.top
        controller:         _root.viewer3DCameraController
        autoHide:           true
        visible:            !ScreenTools.isTinyScreen && QGroundControl.corePlugin.options.flyView.showMapScale && QGCViewer3DManager.displayMode === QGCViewer3DManager.View3D && !!_root.viewer3DCameraController
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
