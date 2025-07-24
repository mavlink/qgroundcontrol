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

import QGroundControl.ScreenTools
import QGroundControl.UTMSP

import QGroundControl.Viewer3D

//my added import
import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: _root

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property string errorMessages: ""
    property real msgScaleFactor: 1.0
    property real safeScaleFactor: msgScaleFactor < 1.0 ? 1.0 : (msgScaleFactor > 3.0 ? 3.0 : msgScaleFactor)
    property var msgScaleFactObj: QGroundControl.settingsManager.appSettings.appImportantMsgScaleFactor

    //FOR WIDTH
    property real msgScaleFactor_WIDTH: 1.0
    property real safeScaleFactor_WIDTH: msgScaleFactor_WIDTH < 1.0 ? 1.0 : (msgScaleFactor_WIDTH > 3.0 ? 3.0 : msgScaleFactor_WIDTH)
    property var msgScaleFactObj_WIDTH: QGroundControl.settingsManager.appSettings.appImportantMsgScaleFactor_WIDTH

    //FOR FONT
    property real msgScaleFactor_FONT: 1.0
    property real safeScaleFactor_FONT: msgScaleFactor_FONT < 1.0 ? 1.0 : (msgScaleFactor_FONT > 3.0 ? 3.0 : msgScaleFactor_FONT)
    property var msgScaleFactObj_FONT: QGroundControl.settingsManager.appSettings.appImportantMsgScaleFactor_FONT
    //end my add

    //property real msgScaleFactor: QGroundControl.settingsManager.appSettings.appImportantMsgScaleFactor ? QGroundControl.settingsManager.appSettings.appImportantMsgScaleFactor.value : 1.0

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
    //property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
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

    property real   _fullItemZorder:    0
    property real   _pipItemZorder:     QGroundControl.zOrderWidgets

    function _calcCenterViewPort() {
        var newToolInset = Qt.rect(0, 0, width, height)
        toolstrip.adjustToolInset(newToolInset)
    }

    function dropMainStatusIndicatorTool() {
        toolbar.dropMainStatusIndicatorTool();
    }

    //my add for changing the HEIGHT of the message box
    //增加缩放函数
    function increaseScale() {
        msgScaleFactor = safeScaleFactor + 0.1
        if (msgScaleFactor > 3.0)
            msgScaleFactor = 3.0
    }
    // 减少缩放函数
    function decreaseScale() {
        msgScaleFactor = safeScaleFactor - 0.1
        if (msgScaleFactor < 1.0)
            msgScaleFactor = 1.0
    }
    //my add
    function initializeMsgScaleFactor() {
        if (msgScaleFactObj) {
            if (msgScaleFactObj.value === 0 || msgScaleFactObj.value === undefined) {
                msgScaleFactObj.value = 1.0;
                msgScaleFactor = 1.0;
            }
            else
                msgScaleFactor = msgScaleFactObj.value;
        }
        else
            msgScaleFactor = 1.0;
    }


    //my add FOR WIDTH:
    //增加缩放函数
    function increaseScale_WIDTH() {
        msgScaleFactor_WIDTH = safeScaleFactor_WIDTH + 0.1
        if (msgScaleFactor_WIDTH > 3.0)
            msgScaleFactor_WIDTH = 3.0
    }
    //减少缩放函数
    function decreaseScale_WIDTH() {
        msgScaleFactor_WIDTH = safeScaleFactor_WIDTH - 0.1
        if (msgScaleFactor_WIDTH < 1.0)
            msgScaleFactor_WIDTH = 1.0
    }
    //my add
    function initializeMsgScaleFactor_WIDTH() {
        if (msgScaleFactObj_WIDTH) {
            if (msgScaleFactObj_WIDTH.value === 0 || msgScaleFactObj_WIDTH.value === undefined) {
                msgScaleFactObj_WIDTH.value = 1.0;
                msgScaleFactor_WIDTH = 1.0;
            }
            else
                msgScaleFactor_WIDTH = msgScaleFactObj_WIDTH.value;
        }
        else
            msgScaleFactor_WIDTH = 1.0;
    }


    //my add for Font
    //增加缩放函数
    function increaseScale_FONT() {
        msgScaleFactor_FONT = safeScaleFactor_FONT + 0.1
        if (msgScaleFactor_FONT > 3.0)
            msgScaleFactor_FONT = 3.0
    }
    //减少缩放函数
    function decreaseScale_FONT() {
        msgScaleFactor_FONT = safeScaleFactor_FONT - 0.1
        if (msgScaleFactor_FONT < 1.0)
            msgScaleFactor_FONT = 1.0
    }
    //my add
    function initializeMsgScaleFactor_FONT() {
        if (msgScaleFactObj_FONT) {
            if (msgScaleFactObj_FONT.value === 0 || msgScaleFactObj_FONT.value === undefined) {
                msgScaleFactObj_FONT.value = 1.0;
                msgScaleFactor_FONT = 1.0;
            }
            else
                msgScaleFactor_FONT = msgScaleFactObj_FONT.value;
        }
        else
            msgScaleFactor_FONT = 1.0;
    }


    QGCToolInsets {
        id:                     _toolInsets
        leftEdgeBottomInset:    _pipView.leftEdgeBottomInset
        bottomEdgeLeftInset:    _pipView.bottomEdgeLeftInset
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
            planMasterController:   _planController
            rightPanelWidth:        ScreenTools.defaultFontPixelHeight * 9
            pipView:                _pipView
            pipMode:                !_mainWindowIsMap
            toolInsets:             customOverlay.totalToolInsets
            mapName:                "FlightDisplayView"
            enabled:                !viewer3DWindow.isOpen
        }

        FlyViewVideo {
            id:         videoControl
            pipView:    _pipView
        }

        PipView {
            id:                     _pipView
            anchors.left:           parent.left
            anchors.bottom:         parent.bottom
            anchors.margins:        _toolsMargin
            item1IsFullSettingsKey: "MainFlyWindowIsMap"
            item1:                  mapControl
            item2:                  QGroundControl.videoManager.hasVideo ? videoControl : null
            show:                   QGroundControl.videoManager.hasVideo && !QGroundControl.videoManager.fullScreen &&
                                        (videoControl.pipState.state === videoControl.pipState.pipState || mapControl.pipState.state === mapControl.pipState.pipState)
            z:                      QGroundControl.zOrderWidgets

            property real leftEdgeBottomInset: visible ? width + anchors.margins : 0
            property real bottomEdgeLeftInset: visible ? height + anchors.margins : 0
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

        //my add, connect messages:
        Connections {
            target: QGroundControl.multiVehicleManager
            onActiveVehicleChanged: {
                _activeVehicle = QGroundControl.multiVehicleManager.activeVehicle
                messageConnectionLoader.active = (_activeVehicle !== null)
                console.log("Active vehicle changed:", _activeVehicle)
            }
        }

        //my add, connect HEIGHT's SIZE:
        Connections {
            target: msgScaleFactObj
            onValueChanged: {
                if (msgScaleFactObj.value === 0 || msgScaleFactObj.value === undefined)
                    msgScaleFactObj.value = 1.0;
                else
                    msgScaleFactor = msgScaleFactObj.value
            }
        }

        //my add, connect WIDTH's SIZE
        Connections {
            target: msgScaleFactObj_WIDTH
            onValueChanged: {
                if (msgScaleFactObj_WIDTH.value === 0 || msgScaleFactObj_WIDTH.value === undefined)
                    msgScaleFactObj_WIDTH.value = 1.0;
                else
                    msgScaleFactor_WIDTH = msgScaleFactObj_WIDTH.value
            }
        }

        //my add, connect FONT's SIZE
        Connections {
            target: msgScaleFactObj_FONT
            onValueChanged: {
                if (msgScaleFactObj_FONT.value === 0 || msgScaleFactObj_FONT.value === undefined)
                    msgScaleFactObj_FONT.value = 1.0;
                else
                    msgScaleFactor_FONT = msgScaleFactObj_FONT.value
            }
        }

        QtObject {
            id: vehicleMessagesBridge
            property string allErrors: ""

            function handleNewMessage(msg) {

                // Step 1: 解码 HTML 实体
                msg = msg.replace(/&lt;/g, "<").replace(/&gt;/g, ">");

                // Step 2: 判断是否为红色消息
                const isRed = msg.includes("<#E>") ||
                              msg.includes("<#W>") ||
                              msg.includes("<#C>") ||
                              msg.includes("<#I>");

                // Step 3: 去除多余的标签和字体样式
                let cleanMsg = msg
                    .replace(/<font[^>]*>/gi, "")    // 去除 <font ...>
                    .replace(/<\/font>/gi, "")       // 去除 </font>
                    .replace(/<br\s*\/?>/gi, "")     // 去除 <br> 或 <br/>
                    .replace(/<#[A-Z]>/g, "")        // 去除 QGC 标签如 <#E>
                    .replace(/^["'\s>]+/, "")        // 去除开头的 > 或引号等冗余符号
                    .trim();

                // Step 4: 包裹颜色
                const finalMsg = `<font color="${isRed ? "red" : "black"}">${cleanMsg}</font><br>`;

                // Step 5: 更新文本框内容
                allErrors += finalMsg;
                messageText.text = allErrors;
            }
        }

        Loader {
            id: messageConnectionLoader
            active: _activeVehicle !== null
            sourceComponent: _activeVehicle === null ? null : messageConnectionsComponent
        }

        Component {
            id: messageConnectionsComponent
            Connections {
                target: _activeVehicle
                onNewFormattedMessage: msg => vehicleMessagesBridge.handleNewMessage(msg)
            }
        }
    }

    //my add important messages
    Rectangle {
        id: importantMessageBox
        property real safeScaleFactor: msgScaleFactor < 0.0 ? 0.0 : (msgScaleFactor > 3.0 ? 3.0 : msgScaleFactor)
        property real safeScaleFactor_WIDTH: msgScaleFactor_WIDTH < 0.0 ? 0.0 : (msgScaleFactor_WIDTH > 3.0 ? 3.0 : msgScaleFactor_WIDTH)
        property real safeScaleFactor_FONT: msgScaleFactor_FONT < 0.0 ? 0.0 : (msgScaleFactor_FONT > 3.0 ? 3.0 : msgScaleFactor_FONT)

        //property real safeScaleFactor: msgScaleFactor > 0.1 ? msgScaleFactor : 0.1  // 设最小缩放0.1，防止过小
        width: safeScaleFactor_WIDTH * 500//(msgScaleFactor > 0.01 ? msgScaleFactor : 1.0) * 500
        height: safeScaleFactor * 80//(msgScaleFactor > 0.01 ? msgScaleFactor : 1.0) * 80
        anchors.top: parent.top
        anchors.right: parent.right
        color: "#E6FFFFFF"
        border.color: "white"
        border.width: 1
        z: 9999

        Flickable {
            id: flickable
            anchors.fill: parent
            contentWidth: parent.width
            contentHeight: messageText.height
            clip: true
            interactive: true

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
                active: true
            }

            Text {
                id: messageText
                width: flickable.width - 16
                textFormat: Text.RichText//able to use <font>
                text: _root.errorMessages
                //text: "This is text"//vehicleMessagesBridge.allErrors
                wrapMode: Text.Wrap
                color: "black"
                //font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.6 * safeScaleFactor
                font.pixelSize: ScreenTools.defaultFontPixelHeight * 0.6 * safeScaleFactor_FONT
                anchors.margins: 8
            }
        }

        Component.onCompleted: {
            initializeMsgScaleFactor()
            initializeMsgScaleFactor_WIDTH()
            initializeMsgScaleFactor_FONT()
            flickable.contentY = flickable.contentHeight - flickable.height
        }
    }
}
