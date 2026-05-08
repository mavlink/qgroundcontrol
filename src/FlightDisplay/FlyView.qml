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
import QGroundControl.Controllers
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Vehicle

// 3D Viewer modules
import Viewer3D

Item {
    id: _root

    // These should only be used by MainRootWindow
    property var planController:    _planController
    property var guidedController:  _guidedController

    // Properties of UTM adapter
    property bool utmspSendActTrigger: false

    PlanMasterController {
        id:                     _planController
        flyView:                true
        Component.onCompleted:  start()
    }

    property bool   _mainWindowIsMap:       mapControl.pipState.state === mapControl.pipState.fullState
    property bool   _isFullWindowItemDark:  _mainWindowIsMap ? mapControl.isSatelliteMap : true
    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _missionController:     _planController.missionController
    property var    _geoFenceController:    _planController.geoFenceController
    property var    _rallyPointController:  _planController.rallyPointController
    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property var    _guidedController:      guidedActionsController
    property var    _guidedValueSlider:     guidedValueSlider
    property var    _widgetLayer:           widgetLayer
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75
    property rect   _centerViewport:        Qt.rect(0, 0, width, height)
    property real   _rightPanelWidth:       ScreenTools.defaultFontPixelWidth * 30
    property var    _mapControl:            mapControl
    property bool   _ceVideoInsetVisible:   true

    property real   _fullItemZorder:    0
    property real   _pipItemZorder:     QGroundControl.zOrderWidgets

    function _calcCenterViewPort() {
        var newToolInset = Qt.rect(0, 0, width, height)
        toolstrip.adjustToolInset(newToolInset)
    }

    function dropMainStatusIndicatorTool() {
        toolbar.dropMainStatusIndicatorTool();
    }

    QGCToolInsets {
        id:                     _toolInsets
        leftEdgeBottomInset:    0
        bottomEdgeLeftInset:    0
    }

    FlyViewToolBar {
        id:         toolbar
        visible:    !QGroundControl.videoManager.fullScreen
    }

    Item {
        id:                 mapHolder
        anchors.top:        toolbar.bottom
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.right:      parent.right

        FlyViewMap {
            id:                     mapControl
            anchors.fill:           parent
            planMasterController:   _planController
            rightPanelWidth:        ScreenTools.defaultFontPixelHeight * 9
            pipView:                _pipView
            pipMode:                false
            toolInsets:             customOverlay.totalToolInsets
            mapName:                "FlightDisplayView"
            enabled:                !viewer3DWindow.isOpen
            Component.onCompleted:  pipState.state = pipState.fullState
        }

        Item {
            id:         _pipView
            visible:    false
            property real leftEdgeBottomInset: 0
            property real bottomEdgeLeftInset: 0
        }

        FlyViewWidgetLayer {
            id:                     widgetLayer
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            anchors.left:           parent.left
            anchors.right:          guidedValueSlider.visible ? guidedValueSlider.left : parent.right
            z:                      _fullItemZorder + 2 // we need to add one extra layer for map 3d viewer (normally was 1)
            parentToolInsets:       _toolInsets
            mapControl:             _mapControl
            visible:                !QGroundControl.videoManager.fullScreen
            utmspActTrigger:        utmspSendActTrigger
            isViewer3DOpen:         viewer3DWindow.isOpen
        }

        FlyViewCustomLayer {
            id:                 customOverlay
            anchors.fill:       widgetLayer
            z:                  _fullItemZorder + 2
            parentToolInsets:   widgetLayer.totalToolInsets
            mapControl:         _mapControl
            visible:            !QGroundControl.videoManager.fullScreen
        }

        // Development tool for visualizing the insets for a paticular layer, show if needed
        FlyViewInsetViewer {
            id:                     widgetLayerInsetViewer
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            anchors.left:           parent.left
            anchors.right:          guidedValueSlider.visible ? guidedValueSlider.left : parent.right
            z:                      widgetLayer.z + 1
            insetsToView:           widgetLayer.totalToolInsets
            visible:                false
        }

        GuidedActionsController {
            id:                 guidedActionsController
            missionController:  _missionController
            guidedValueSlider:     _guidedValueSlider
        }

        //-- Guided value slider (e.g. altitude)
        GuidedValueSlider {
            id:                 guidedValueSlider
            anchors.right:      parent.right
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            z:                  QGroundControl.zOrderTopMost
            visible:            false
        }

        Viewer3D{
            id:                     viewer3DWindow
            anchors.fill:           parent
        }

        Rectangle {
            id:                     ceVideoInset
            z:                      QGroundControl.zOrderTopMost - 2
            visible:                QGroundControl.videoManager.hasVideo && _ceVideoInsetVisible && !viewer3DWindow.isOpen
            anchors.left:           parent.left
            anchors.bottom:         parent.bottom
            anchors.leftMargin:     customOverlay.totalToolInsets.leftEdgeBottomInset + _toolsMargin
            anchors.bottomMargin:   customOverlay.totalToolInsets.bottomEdgeLeftInset + _toolsMargin
            width:                  Math.max(ScreenTools.defaultFontPixelWidth * 28, parent.width * 0.26)
            height:                 width * 9 / 16
            color:                  "black"
            border.color:           Qt.rgba(1, 1, 1, 0.35)
            border.width:           1
            radius:                 3
            clip:                   true

            FlightDisplayViewVideo {
                anchors.fill:   parent
                useSmallFont:   true
                visible:        QGroundControl.videoManager.isStreamSource
            }

        }

        Rectangle {
            z:                  QGroundControl.zOrderTopMost - 1
            visible:            QGroundControl.videoManager.hasVideo && !_ceVideoInsetVisible && !viewer3DWindow.isOpen
            anchors.left:       parent.left
            anchors.bottom:     parent.bottom
            anchors.leftMargin: customOverlay.totalToolInsets.leftEdgeBottomInset + _toolsMargin
            anchors.bottomMargin: customOverlay.totalToolInsets.bottomEdgeLeftInset + _toolsMargin
            width:              ScreenTools.defaultFontPixelHeight * 2.4
            height:             width
            radius:             3
            color:              Qt.rgba(0, 0, 0, 0.7)

            QGCLabel {
                anchors.centerIn: parent
                text:             qsTr("VID")
                color:            "white"
                font.pointSize:   ScreenTools.smallFontPointSize
                font.bold:        true
            }

            MouseArea {
                anchors.fill: parent
                onClicked:    _ceVideoInsetVisible = true
            }
        }
    }
}
