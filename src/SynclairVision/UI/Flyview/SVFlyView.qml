import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQml.Models
import QtQuick.Shapes 2.15
import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: root
    clip: true

    property alias videoContentAreaItem: videoContentArea
    property var detectionPosition: SVSettings.aiDetectionOverlayPosition

    property bool isMaximized: mainWindow.visibility === Window.Maximized

    property var parentToolInsets
    property real leftToolStripBottom: 0
    property bool previewMode: false

    property int _widgetMargin: 0
    property int _toolBarHeight: 0
    property real pipViewWidth: 0

    property double _ar: QGroundControl.videoManager.gstreamerEnabled
        ? QGroundControl.videoManager.videoSize.width / QGroundControl.videoManager.videoSize.height
        : QGroundControl.videoManager.aspectRatio
    property int _fitMode: QGroundControl.settingsManager.videoSettings.videoFit.rawValue
    property bool _isMode_FIT_WIDTH: _fitMode === 0
    property bool _isMode_FIT_HEIGHT: _fitMode === 1
    property bool _isMode_FILL: _fitMode === 2
    property bool _isMode_NO_CROP: _fitMode === 3
    readonly property var digiview: QGroundControl.digiviewManager
    readonly property bool digiviewOutputGeometryAvailable: !!digiview
        && digiview.connected
        && digiview.hasVideoOutputParameters
        && digiview.videoOutputStreamName === digiview.streamName
        && digiview.videoOutputWidth > 0
        && digiview.videoOutputHeight > 0
    readonly property var digiviewCameraViews: {
        if (!digiviewOutputGeometryAvailable || !digiview.videoOutputViews) {
            return []
        }

        const requestedViewCount = Number(digiview.videoOutputNumUserViews)
        if (!isFinite(requestedViewCount)) {
            return []
        }

        const outputWidth = digiview.videoOutputWidth
        const outputHeight = digiview.videoOutputHeight
        const viewCount = Math.max(0, Math.min(Math.floor(requestedViewCount), digiview.videoOutputViews.length))
        const views = []

        for (let i = 0; i < viewCount; ++i) {
            const view = digiview.videoOutputViews[i]
            const viewX = Number(view.x)
            const viewY = Number(view.y)
            const viewWidth = Number(view.width)
            const viewHeight = Number(view.height)

            if (!isFinite(viewX) || !isFinite(viewY) || !isFinite(viewWidth) || !isFinite(viewHeight)) {
                continue
            }

            const left = Math.max(0, Math.min(viewX, outputWidth))
            const top = Math.max(0, Math.min(viewY, outputHeight))
            const right = Math.max(left, Math.min(viewX + viewWidth, outputWidth))
            const bottom = Math.max(top, Math.min(viewY + viewHeight, outputHeight))

            if (right <= left || bottom <= top) {
                continue
            }

            // VIDEO_OUTPUT_PARAMETERS has no camera id; view order remains the established slot mapping.
            views.push({ slot: i, x: left, y: top, width: right - left, height: bottom - top })
        }

        return views
    }
    readonly property bool usingDigiviewLayout: digiviewCameraViews.length > 0
    readonly property real digiviewScaleX: digiviewOutputGeometryAvailable ? videoContentArea.width / digiview.videoOutputWidth : 0
    readonly property real digiviewScaleY: digiviewOutputGeometryAvailable ? videoContentArea.height / digiview.videoOutputHeight : 0

    readonly property var visibleCameraSlots: digiviewCameraViews.map((view) => view.slot)




    QGCPalette { id: qgcPalette}

    function beginPointTrackingSelection(trackingId) {
        if (!root.visible || root.previewMode || root.width <= 0 || root.height <= 0
                || !SVState.beginPointTrackingSelection(
                    trackingId, SVState.cameraSelected, root.visibleCameraSlots)) {
            return
        }
    }

    function submitImmediatePointTracking(trackingId) {
        let rejectionMessage = ""
        if (!root.visible) {
            rejectionMessage = qsTr("Immediate tracking is unavailable while the fly view is hidden.")
        } else if (root.previewMode) {
            rejectionMessage = qsTr("Immediate tracking is unavailable in preview mode.")
        } else if (!QGroundControl.videoManager.decoding || !videoContentArea.visible) {
            rejectionMessage = qsTr("Immediate tracking is unavailable because video content is not visible or decoding.")
        } else if (!root.digiviewOutputGeometryAvailable) {
            rejectionMessage = qsTr("Immediate tracking is unavailable because video geometry is missing.")
        } else if (!Number.isFinite(root.digiviewScaleX) || !Number.isFinite(root.digiviewScaleY)
                   || root.digiviewScaleX <= 0 || root.digiviewScaleY <= 0) {
            rejectionMessage = qsTr("Immediate tracking is unavailable because the video scale is invalid.")
        }

        let point
        if (!rejectionMessage) {
            const window = root.Window.window
            if (!window) {
                rejectionMessage = qsTr("Immediate tracking is unavailable because the fly view window is missing.")
            } else if (!window.contentItem) {
                rejectionMessage = qsTr("Immediate tracking is unavailable because the window content is missing.")
            } else {
                const contentPoint = window.contentItem.mapFromItem(
                    null, ScreenTools.mouseX() - window.x, ScreenTools.mouseY() - window.y)
                point = root.mapFromItem(window.contentItem, contentPoint.x, contentPoint.y)
                if (!Number.isFinite(point.x) || !Number.isFinite(point.y)
                        || point.x < videoContentArea.x || point.x >= videoContentArea.x + videoContentArea.width
                        || point.y < videoContentArea.y || point.y >= videoContentArea.y + videoContentArea.height) {
                    rejectionMessage = qsTr("The selected point is outside the video content.")
                }
            }
        }

        if (!rejectionMessage) {
            const outputX = (point.x - videoContentArea.x) / root.digiviewScaleX
            const outputY = (point.y - videoContentArea.y) / root.digiviewScaleY
            for (let index = 0; index < root.digiviewCameraViews.length; ++index) {
                const view = root.digiviewCameraViews[index]
                if (outputX >= view.x && outputX < view.x + view.width
                        && outputY >= view.y && outputY < view.y + view.height) {
                    const normalizedX = (outputX - view.x) / view.width * 2.0 - 1.0
                    const normalizedY = 1.0 - (outputY - view.y) / view.height * 2.0
                    const submitted = SVState.submitImmediatePointTracking(
                        trackingId, view.slot, normalizedX, normalizedY)
                    if (submitted) {
                        return true
                    }

                    rejectionMessage = qsTr("Immediate tracking was rejected because no camera is selected, controls are locked, or the camera state changed.")
                    break
                }
            }
            if (!rejectionMessage) {
                rejectionMessage = qsTr("The selected point does not match a DigiView camera view.")
            }
        }

        SVNotificationManager.add(
            qsTr("Immediate Tracking Rejected"), rejectionMessage, "warning", "network_error")
        return false
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

        function onPointTrackingSelectionRequested(trackingId) {
            root.beginPointTrackingSelection(trackingId)
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


    Item {
        id: videoContentArea

        visible: QGroundControl.videoManager.decoding

        width: {
            if (!isFinite(root._ar) || root._ar <= 0.0) {
                return root.width
            }

            if (SVState.synclairOverlay) {
                return Math.min(root.width, root.height * root._ar)
            }

            if (root._isMode_FIT_HEIGHT
                    || (root._isMode_FILL && (root.width / root.height < root._ar))
                    || (root._isMode_NO_CROP && (root.width / root.height > root._ar))) {
                return root.height * root._ar
            }
            return root.width
        }
        height: {
            if (!isFinite(root._ar) || root._ar <= 0.0) {
                return root.height
            }

            if (SVState.synclairOverlay) {
                return Math.min(root.height, root.width * (1 / root._ar))
            }

            if (root._isMode_FIT_WIDTH
                    || (root._isMode_FILL && (root.width / root.height > root._ar))
                    || (root._isMode_NO_CROP && (root.width / root.height < root._ar))) {
                return root.width * (1 / root._ar)
            }
            return root.height
        }
        anchors.centerIn: parent

        
        Repeater {
            model: root.digiviewCameraViews

            delegate: SVCameraLayer {
                required property var modelData

                width: modelData.width * root.digiviewScaleX
                height: modelData.height * root.digiviewScaleY
                x: modelData.x * root.digiviewScaleX
                y: modelData.y * root.digiviewScaleY
                cameraSlot: modelData.slot
                previewMode: root.previewMode
                z: !root.previewMode
                    && SVState.cameraSelectionEnabled
                    && SVState.cameraSelected === cameraSlot
                    && !SVState.cursorTrackingSessionActive
                    && SVState.hud ? 3 : 0

                _widgetMargin: root._widgetMargin

                onCursorTargetSelected: (cameraSlot, normalizedX, normalizedY) => {
                    SVState.recordCursorTarget(cameraSlot, normalizedX, normalizedY)
                }
            }
        }

        SVFlyViewDetectionOverlay {
            id: detectionOverlay
            x: root.digiview.videoOutputDetectionOverlayRect.x * root.digiviewScaleX
            y: root.digiview.videoOutputDetectionOverlayRect.y * root.digiviewScaleY
            width: root.digiview.videoOutputDetectionOverlayRect.width * root.digiviewScaleX 
            height: root.digiview.videoOutputDetectionOverlayRect.height * root.digiviewScaleY
            visible: root.digiviewOutputGeometryAvailable
                && root.digiview.videoOutputDetectionOverlayRect.width > 0
                && root.digiview.videoOutputDetectionOverlayRect.height > 0
            immediateSttHandler: () => root.submitImmediatePointTracking('singleTarget')
        }

        

        
    }
    SVBorder {
        id: cameraBorder
        anchors.fill: videoContentArea
        borderWidth: SVUnits.lineWidth * 1
        borderColor: qgcPalette.windowShadeLight
        borderVisible: QGroundControl.videoManager.decoding
        z: 2
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
        //(root.isMaximized || QGroundControl.videoManager.fullScreen)
        property bool adjustHud: SVSettings.alignHud && QGroundControl.videoManager.decoding
        //property bool adjustHud:  && QGroundControl.videoManager.decoding + SVState.aiOverlay
        readonly property real toolbarInset: SVState.toolbar ? _toolBarHeight : 0

        property real heightOffset: (root.height - videoContentArea.height) / 2
        property real widthOffset: (root.width - videoContentArea.width) / 2

        id: widgetLayer
        z: 2
        anchors.left: adjustHud ? videoContentArea.left : parent.left
        anchors.right: adjustHud ? videoContentArea.right : parent.right
        anchors.top: adjustHud ? videoContentArea.top : parent.top
        anchors.bottom: adjustHud ? videoContentArea.bottom : parent.bottom
        
        anchors.leftMargin: _widgetMargin + ((adjustHud && detectionPosition === "ColumnLeft") ? detectionOverlay.width : 0)
        anchors.rightMargin: _widgetMargin + ((adjustHud && (detectionPosition === "ColumnRight" || detectionPosition === "Single")) ? detectionOverlay.width : 0)
        anchors.bottomMargin: _widgetMargin + ((adjustHud && detectionPosition === "RowBottom") ? detectionOverlay.height : 0)
        //anchors.topMargin: _widgetMargin + (adjustHud ? (Math.max(Math.max(toolbarInset, heightOffset), adjustHud && detectionPosition === "RowTop" ? detectionOverlay.height : 0)
        anchors.topMargin: _widgetMargin + (adjustHud ? (Math.max(Math.max(0, toolbarInset - heightOffset), adjustHud && detectionPosition === "RowTop" ? detectionOverlay.height : 0)) : toolbarInset)
        offsetX: adjustHud ? (anchors.rightMargin - _widgetMargin + Math.floor(widthOffset)) : 0 
        //offsetY: SVUnits.objectWidth + toolbarInset + (adjustHud ? Math.max(Math.max(toolbarInset, heightOffset), detectionPosition === "RowTop" ? detectionOverlay.height : 0)
        offsetY: SVUnits.objectWidth + (adjustHud ? (Math.max(Math.max(toolbarInset, heightOffset) - toolbarInset, detectionPosition === "RowTop")) : 0)       
        leftToolStripBottom: root.leftToolStripBottom
        pipViewWidth: root.pipViewWidth
        visible: !root.previewMode && !SVState.cursorTrackingSessionActive
        visibleCameraSlots: root.visibleCameraSlots
        cursorTargetingAvailable: root.visible && !root.previewMode && root.width > 0 && root.height > 0
    }

    onPreviewModeChanged: {
        if (previewMode) {
            SVState.cancelCursorTrackingSelection()
        }
    }

    onVisibleCameraSlotsChanged: {
        if (SVState.cameraSelected < 0
                || visibleCameraSlots.indexOf(SVState.cameraSelected) !== -1) {
            return
        }

        SVState.clearCamera()
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
                font.pointSize:     SVUnits.smallText
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

}
