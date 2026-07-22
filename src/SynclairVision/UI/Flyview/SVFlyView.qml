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
    property var _pipViewWidth
    property string _savedRtspUrl: ""
    property bool _rtspOverrideApplied: false

    readonly property var digiview: QGroundControl.digiviewManager
    readonly property var rtspUrlFact: QGroundControl.settingsManager.videoSettings.rtspUrl
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

    QGCPalette { id: qgcPalette}

    function triggerPhotoBorder() {
        photoBorder.trigger()
    }

    function takePhoto() {
        if (digiview) {
            digiview.sendCaptureParameters("stream", 1, 0, 0, 0)
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

    function _rtspRawValueText() {
        if (!rtspUrlFact || rtspUrlFact.rawValue === undefined || rtspUrlFact.rawValue === null) {
            return ""
        }

        return rtspUrlFact.rawValue.toString().trim()
    }

    function _rtspPathForProfile(profile) {
        const streamName = SVSettings.networkProfileStreamName(profile && profile.streamName)

        return streamName.charAt(0) === "/" ? streamName : "/" + streamName
    }

    function _overrideRtspUrl() {
        const profile = SVSettings.selectedNetworkProfile()

        if (!profile) {
            return ""
        }

        const host = SVSettings.networkProfileText(profile.host)
        const videoPort = SVSettings.networkProfilePort(profile.videoPort, -1)

        if (host === "" || videoPort <= 0) {
            return ""
        }

        return "rtsp://" + host + ":" + videoPort + root._rtspPathForProfile(profile)
    }

    function _updateRtspUrlOverride() {
        if (!rtspUrlFact) {
            return
        }

        const shouldOverride = SVState.synclairOverlay && SVState.digiviewActive
        const overrideRtspUrl = shouldOverride ? root._overrideRtspUrl() : ""

        if (overrideRtspUrl !== "") {
            if (!root._rtspOverrideApplied) {
                root._savedRtspUrl = root._rtspRawValueText()
                root._rtspOverrideApplied = true
            }

            if (root._rtspRawValueText() !== overrideRtspUrl) {
                rtspUrlFact.rawValue = overrideRtspUrl
            }

            return
        }

        if (!root._rtspOverrideApplied) {
            return
        }

        if (root._rtspRawValueText() !== root._savedRtspUrl) {
            rtspUrlFact.rawValue = root._savedRtspUrl
        }

        root._rtspOverrideApplied = false
        root._savedRtspUrl = ""
    }

    Component.onCompleted: {
        root._applySelectedNetworkProfile()
        Qt.callLater(root.autoconnectDigiview)
        Qt.callLater(root._updateRtspUrlOverride)
    }

    Component.onDestruction: {
        if (root._rtspOverrideApplied && rtspUrlFact) {
            rtspUrlFact.rawValue = root._savedRtspUrl
        }
    }

    Connections {
        target: SVState

        function onTakePhotoRequested() {
            root.takePhoto()
        }

        function onSynclairOverlayChanged() {
            root._updateRtspUrlOverride()
        }
    }

    Connections {
        target: SVSettings

        function onNetworkProfilesChanged() {
            root._applySelectedNetworkProfile()
            root._updateRtspUrlOverride()
        }

        function onNetworkSelectedProfileIndexChanged() {
            root._applySelectedNetworkProfile()
            root._updateRtspUrlOverride()
        }
    }

    Connections {
        target: digiview

        function onConnectedChanged() {
            if (!SVState.digiviewActive) {
                SVState.clearCamera()
            }

            root._updateRtspUrlOverride()
        }
    }

    Connections {
        target: QGroundControl.videoManager

        function onStreamingChanged() {
            if (!SVState.digiviewActive) {
                SVState.clearCamera()
            }

            root._updateRtspUrlOverride()
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
        borderWidth: SVUnits.thickLineWidth + SVUnits.lineWidth * 3
        borderColor: qgcPalette.colorRed
        borderVisible: !root.previewMode && SVState.record
        pulse: true
    }

    SVBorder {
        id: photoBorder
        anchors.fill: parent
        visible: !root.previewMode
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
        visible: !root.previewMode
        onLayoutSelected: (layoutId) => SVState.layout = layoutId
    }
}
