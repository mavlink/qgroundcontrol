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
import QGroundControl.Toolbar
import QGroundControl.Viewer3D

import QGroundControl.SynclairVisionUI

Item {
    id: _root

    readonly property bool _is3DMode:       QGCViewer3DManager.displayMode === QGCViewer3DManager.View3D
    readonly property bool _keepSceneAlive: QGroundControl.settingsManager.viewer3DSettings.keepSceneAlive.rawValue

    property bool adjustHud: SVSettings.alignHud && QGroundControl.videoManager.decoding
    property var detectionPosition: SVSettings.aiDetectionOverlayPosition
    readonly property var digiview: QGroundControl.digiviewManager
    readonly property bool digiviewOutputGeometryAvailable: !!digiview
        && digiview.connected
        && digiview.hasVideoOutputParameters
        && digiview.videoOutputStreamName === digiview.streamName
        && digiview.videoOutputWidth > 0
        && digiview.videoOutputHeight > 0
    property real detectionWidth: digiview.videoOutputDetectionOverlayRect.width * digiviewScaleX    
    property real detectionHeight: digiview.videoOutputDetectionOverlayRect.height * digiviewScaleY
    readonly property real digiviewScaleX: digiviewOutputGeometryAvailable ? videoContentArea.width / digiview.videoOutputWidth : 0
    readonly property real digiviewScaleY: digiviewOutputGeometryAvailable ? videoContentArea.height / digiview.videoOutputHeight : 0

    // These should only be used by MainRootWindow
    property var planController:    _planController
    property var guidedController:  _guidedController

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
    property real   _widgetMargin:          ScreenTools.defaultFontPixelWidth * 0.75

    property real   _fullItemZorder:    0
    property real   _pipItemZorder:     QGroundControl.zOrderWidgets
    readonly property bool _showVideoView: QGroundControl.videoManager.hasVideo || SVState.synclairOverlay

    function _calcCenterViewPort() {
        var newToolInset = Qt.rect(0, 0, width, height)
        toolstrip.adjustToolInset(newToolInset)
    }

    function dropMainStatusIndicatorTool() {
        toolbar.dropMainStatusIndicatorTool();
    }

    

    QGCToolInsets {
        id:                     _toolInsets
        topEdgeLeftInset:       toolbar.height
        topEdgeCenterInset:     topEdgeLeftInset
        topEdgeRightInset:      topEdgeLeftInset
        leftEdgeBottomInset:    _pipView.leftEdgeBottomInset
        bottomEdgeLeftInset:    _pipView.bottomEdgeLeftInset
    }

    Item {
        id:                 mapHolder
        anchors.fill:       parent

        FlyViewMap {
            id:                     mapControl
            planMasterController:   _planController
            rightPanelWidth:        ScreenTools.defaultFontPixelHeight * 9
            pipView:                _pipView
            pipMode:                !_mainWindowIsMap
            toolInsets:             customOverlay.totalToolInsets
            mapName:                "FlightDisplayView"
            enabled:                !_is3DMode
            visible:                !_is3DMode
        }

        FlyViewVideo {
            id:         videoControl
            pipView:    _pipView

            SVFlyView {
                id:                 synclairVisionLayer
                anchors.fill:       parent
                _widgetMargin:      _root._widgetMargin
                _toolBarHeight:     SVState.toolbar ? toolbar.height : 0
                pipViewWidth:       (_pipView._isExpanded) ? _pipView.width : ScreenTools.defaultFontPixelHeight * 2
                leftToolStripBottom: widgetLayer.leftToolStripBottom
                previewMode:        videoControl.pipState.state === videoControl.pipState.pipState
                z:                  1

                //parentToolInsets:   _toolInsets
                visible:            SVState.synclairOverlay
                                         && videoControl.pipState.state !== videoControl.pipState.windowState
            }
        }

        PipView {
            id:                     _pipView
            anchors.left:           adjustHud ? videoContentArea.left : parent.left
            anchors.bottom:         adjustHud ? videoContentArea.bottom : parent.bottom
            anchors.leftMargin:     _widgetMargin + ((adjustHud && detectionPosition === "ColumnLeft") ? detectionWidth : 0)
            anchors.bottomMargin:   _widgetMargin + ((adjustHud && detectionPosition === "RowBottom") ? detectionHeight : 0)
            item1IsFullSettingsKey: "MainFlyWindowIsMap"
            item1:                  mapControl
            item2:                  _showVideoView ? videoControl : null
            show:                   _showVideoView && !QGroundControl.videoManager.fullScreen &&
                                        (videoControl.pipState.state === videoControl.pipState.pipState || mapControl.pipState.state === mapControl.pipState.pipState)
            z:                      QGroundControl.zOrderWidgets

            property real leftEdgeBottomInset: visible ? width + anchors.margins : 0
            property real bottomEdgeLeftInset: visible ? height + anchors.margins : 0

            visible: SVState.hud && !SVState.cursorTrackingSessionActive
        }

        Item {
            id: videoContentArea

            property var _ar: QGroundControl.videoManager.gstreamerEnabled
                ? QGroundControl.videoManager.videoSize.width / QGroundControl.videoManager.videoSize.height
                : QGroundControl.videoManager.aspectRatio

            visible: QGroundControl.videoManager.decoding

            width: {
                if (SVState.synclairOverlay) {
                    return Math.min(_root.width, _root.height * _ar)
                }

                return _root.width
            }
            height: {
                if (SVState.synclairOverlay) {
                    return Math.min(_root.height, _root.width * (1 / _ar))
                }

                return _root.height
            }
            anchors.centerIn: parent
        }

        FlyViewWidgetLayer {
            id: widgetLayer

            
            //property bool adjustHud:  && QGroundControl.videoManager.decoding + SVState.aiOverlay
            readonly property real toolbarInset: SVState.toolbar ? toolbar.height : 0

            property real heightOffset: (_root.height - videoContentArea.height) / 2
            property real widthOffset: (_root.width - videoContentArea.width) / 2

            /*
            
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            anchors.left:           videoContentAreaProxy.left
            anchors.right:          guidedValueSlider.visible ? guidedValueSlider.left : videoContentAreaProxy.right
            anchors.margins:        _widgetMargin
            anchors.topMargin:      (SVState.toolbar) ? toolbar.height + _widgetMargin : _widgetMargin

            
            */
            
            anchors.left: adjustHud ? videoContentArea.left : parent.left
            anchors.right: adjustHud ? videoContentArea.right : parent.right
            anchors.top: adjustHud ? videoContentArea.top : parent.top
            anchors.bottom: adjustHud ? videoContentArea.bottom : parent.bottom
            
            anchors.leftMargin: _widgetMargin + ((adjustHud && detectionPosition === "ColumnLeft") ? detectionWidth : 0)
            anchors.rightMargin: _widgetMargin + ((adjustHud && (detectionPosition === "ColumnRight" || detectionPosition === "Single")) ? detectionWidth : 0)
            anchors.bottomMargin: _widgetMargin + ((adjustHud && detectionPosition === "RowBottom") ? detectionHeight : 0)
            anchors.topMargin: _widgetMargin + (adjustHud ? (Math.max(Math.max(0, toolbarInset - heightOffset), adjustHud && detectionPosition === "RowTop" ? detectionHeight : 0)) : toolbarInset)


            z:                      _fullItemZorder + 2
            parentToolInsets:       _toolInsets
            mapControl:             _mapControl
            visible:                SVState.hud && !SVState.cursorTrackingSessionActive
        }


        FlyViewCustomLayer {
            id:                 customOverlay
            anchors.fill:       widgetLayer
            z:                  _fullItemZorder + 2
            parentToolInsets:   widgetLayer.totalToolInsets
            mapControl:         _mapControl
            visible:            false
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
            anchors.topMargin:  toolbar.height
            z:                  QGroundControl.zOrderTopMost
            visible:            false
        }

        Loader {
            id:           viewer3DLoader
            z:            1
            anchors.fill: parent
            visible:      _is3DMode
        }

        Connections {
            target: QGCViewer3DManager
            function onDisplayModeChanged() {
                if (QGCViewer3DManager.displayMode === QGCViewer3DManager.View3D) {
                    if (!viewer3DLoader.item) {
                        viewer3DLoader.setSource(
                            "qrc:/qml/QGroundControl/Viewer3D/Models3D/Viewer3DModel.qml",
                            { missionController: Qt.binding(() => _missionController) }
                        )
                    }
                } else if (!_keepSceneAlive) {
                    viewer3DLoader.source = ""
                }
            }
        }
    }

    function showVideoFullScreen() {
        if (_pipView && _pipView.item2) {
            _pipView.showItemFull(_pipView.item2)
        }
    }

    FlyViewToolBar {
        id:                 toolbar
        flyView:            _root
        guidedValueSlider:  _guidedValueSlider
        visible:            !QGroundControl.videoManager.fullScreen && SVState.toolbar && !SVState.cursorTrackingSessionActive
    }

    SVShortcutHandler {
        anchors.fill: parent
        toolbarVisible: toolbar.visible
        z: 999
    }
}
