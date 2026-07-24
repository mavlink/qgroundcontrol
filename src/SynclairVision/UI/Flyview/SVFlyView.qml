import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QtQuick.Shapes 2.15
import "../Camera/SVCameraLayouts.js" as SVCameraLayouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: root

    property var parentToolInsets
    property real leftToolStripBottom: 0
    property bool previewMode: false

    property int _widgetMargin: 0
    property int _toolBarHeight: 0
    property real pipViewWidth: 0

    readonly property var digiview: QGroundControl.digiviewManager
    readonly property var cameraLayouts: SVCameraLayouts.getCameraLayouts()
    readonly property var activeLayout: {
        for (let i = 0; i < cameraLayouts.length; i++) {
            if (cameraLayouts[i].id === SVState.layout) {
                return cameraLayouts[i]
            }
        }

        return cameraLayouts.length > 0 ? cameraLayouts[0] : null
    }
    readonly property string resolvedActiveLayoutId: activeLayout ? activeLayout.id : ""
    readonly property var visibleCameraSlots: activeLayout ? activeLayout.panes.map((pane) => pane.slot) : []

    QGCPalette { id: qgcPalette}

    function triggerPhotoBorder() {
        photoBorder.trigger()
    }

    function takePhoto() {
        if (digiview) {
            digiview.sendCaptureParameters(digiview.streamName, 1, 0, 0, 0)
        }

        triggerPhotoBorder()
    }

    function autoconnectDigiview() {
        if (!SVSettings.networkAutoconnectOnStart || !digiview || digiview.connected) {
            return
        }

        if (!root._applySelectedNetworkProfile()) {
            return
        }

        digiview.connectToHost()
    }

    function _applySelectedNetworkProfile() {
        if (!digiview) {
            return false
        }

        return SVSettings.applySelectedNetworkProfile(digiview)
    }

    Component.onCompleted: {
        root._applySelectedNetworkProfile()
        Qt.callLater(root.autoconnectDigiview)
    }

    Component.onDestruction: {
        SVState.cancelCursorTrackingSelection()
    }

    Connections {
        target: SVState

        function onTakePhotoRequested() {
            root.takePhoto()
        }

        function onLayoutChanged() {
            SVState.cancelCursorTrackingSelection()
        }
    }

    Connections {
        target: SVSettings

        function onNetworkProfilesChanged() {
            root._applySelectedNetworkProfile()
        }

        function onNetworkSelectedProfileIndexChanged() {
            root._applySelectedNetworkProfile()
        }
    }

    Connections {
        target: digiview

        function onConnectedChanged() {
            if (!digiview.connected) {
                SVState.clearCamera()
            }
        }
    }


    Repeater {
        model: root.activeLayout ? root.activeLayout.panes : []

        delegate: SVCameraLayer {
            required property var modelData

            width: modelData.w * root.width
            height: modelData.h * root.height
            x: modelData.x * root.width
            y: modelData.y * root.height
            cameraSlot: modelData.slot
            previewMode: root.previewMode

            _widgetMargin: root._widgetMargin

            onCursorTargetSelected: (cameraSlot, normalizedX, normalizedY) => {
                SVState.recordCursorTarget(cameraSlot, normalizedX, normalizedY)
            }
        }
    }

    Item {
        id: separatorLayer
        anchors.fill: parent
        z: 1

        Repeater {
            model: root.activeLayout ? root.activeLayout.separators : []

            delegate: Rectangle {
                required property var modelData
                readonly property bool isVertical: modelData.orientation === 'vertical'
                readonly property bool isHorizontal: modelData.orientation === 'horizontal'

                width:  isVertical   ? SVUnits.lineWidth : modelData.length * root.width
                height: isHorizontal ? SVUnits.lineWidth : modelData.length * root.height 
                x: modelData.x * root.width - (isVertical ? width / 2 : 0)
                y: modelData.y * root.height - (isHorizontal ? height / 2: 0)


                color: qgcPalette.windowShade
            }
        }
    }

    SVBorder {
        id: recordBorder
        anchors.fill: parent
        borderWidth: SVUnits.thickLineWidth + SVUnits.lineWidth * 2
        borderColor: qgcPalette.colorRed
        borderVisible: !root.previewMode && SVState.record && !SVState.cursorTrackingSessionActive
        pulse: true
        z: 2
    }

    SVBorder {
        id: photoBorder
        anchors.fill: parent
        visible: !root.previewMode && !SVState.cursorTrackingSessionActive
        borderWidth: SVUnits.thickLineWidth * 200
        flashDuration: 400
        flashStartOpacity: 0.6
        flashEndOpacity: 0.0
        borderColor: "white"
    }

    SVFlyViewWidgetLayer {
        id: widgetLayer
        z: 2
        anchors.fill: parent
        anchors.margins: _widgetMargin
        anchors.topMargin: _widgetMargin + _toolBarHeight
        leftToolStripBottom: root.leftToolStripBottom
        activeLayoutId: root.resolvedActiveLayoutId
        pipViewWidth: root.pipViewWidth
        visible: !root.previewMode && !SVState.cursorTrackingSessionActive
        onLayoutSelected: (layoutId) => SVState.layout = layoutId
        visibleCameraSlots: root.visibleCameraSlots
        cursorTargetingAvailable: root.visible && !root.previewMode && root.width > 0 && root.height > 0
    }

    onPreviewModeChanged: {
        if (previewMode) {
            SVState.cancelCursorTrackingSelection()
        }
    }

    onVisibleChanged: {
        if (!visible) {
            SVState.cancelCursorTrackingSelection()
        }
    }

    Item {
        id: pipViewDecoration
        anchors.fill: parent
        visible:            root.previewMode

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 1
            border.color: qgcPalette.windowShade
            radius: SVUnits.radius
        }

        SVBackground {
            id: labelBackground
            width: label.width + SVUnits.margin * 6
            height: label.height + SVUnits.margin * 2

            radius: SVUnits.radius
            borderColor: qgcPalette.windowShade
            borderWidth: 0

            anchors.bottom: parent.bottom
            anchors.margins: SVUnits.margin
            anchors.right: parent.right

            QGCLabel {
                id:                 label
                text:               qsTr("SynclairQGC")
                color:              "white"
                font.pointSize:     SVUnits.smallFont
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

}
