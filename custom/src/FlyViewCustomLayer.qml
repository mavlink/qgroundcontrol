import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.SynclairVisionUI

Item {
    id: root

    property var parentToolInsets
    property var totalToolInsets: toolInsets
    property var mapControl

    readonly property var flyView: parent ? parent.parent : null
    readonly property var videoControl: flyView ? flyView.customLayerVideoControl : null
    readonly property var pipView: flyView ? flyView.customLayerPipView : null
    readonly property var widgetLayer: flyView ? flyView.customLayerWidgetLayer : null
    readonly property var toolbar: flyView ? flyView.customLayerToolbar : null
    readonly property bool welcomePromptAcknowledged: {
        const shownIds = QGroundControl.settingsManager.appSettings.firstRunPromptIdsShown.rawValue
        return shownIds.includes(QGroundControl.corePlugin.svInitialWelcomePromptId)
    }

    function showVideoFullScreen() {
        if (!videoControl || !mapControl || !pipView) {
            return
        }

        videoControl.pipState.state = videoControl.pipState.fullState
        mapControl.pipState.state = mapControl.pipState.pipState
        QGroundControl.saveBoolGlobalSetting("MainFlyWindowIsMap", false)
    }

    QGCToolInsets {
        id: toolInsets

        leftEdgeTopInset: parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset: parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset: parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset: parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset: parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset: parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset: parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset: parentToolInsets.topEdgeCenterInset
        topEdgeRightInset: parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset: parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset: parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset: parentToolInsets.bottomEdgeRightInset
    }

    Binding {
        target: toolbar
        property: "visible"
        value: SVState.toolbar && !QGroundControl.videoManager.fullScreen && !SVState.cursorTrackingSessionActive
    }

    Binding {
        target: widgetLayer ? widgetLayer.customLayerTopRightColumn : null
        property: "visible"
        value: widgetLayer && !widgetLayer.customLayerTopRightPanel.visible && !SVState.synclairOverlay
    }

    Binding {
        target: widgetLayer ? widgetLayer.customLayerBottomRightRow : null
        property: "visible"
        value: !SVState.synclairOverlay || (flyView && flyView.customLayerMainWindowIsMap)
    }

    SVFlyView {
        id: synclairFlyView

        parent: videoControl ? videoControl : root
        anchors.fill: parent
        _widgetMargin: flyView ? flyView.customLayerWidgetMargin : 0
        _toolBarHeight: SVState.toolbar && toolbar ? toolbar.height : 0
        pipViewWidth: pipView && pipView._isExpanded ? pipView.width : ScreenTools.defaultFontPixelHeight * 2
        leftToolStripBottom: widgetLayer ? widgetLayer.customLayerToolStrip.topEdgeLeftInset : 0
        previewMode: videoControl && videoControl.pipState.state === videoControl.pipState.pipState
        visible: SVState.synclairOverlay
            && videoControl
            && videoControl.pipState.state !== videoControl.pipState.windowState
        z: 1
    }

    SVShortcutHandler {
        anchors.fill: parent
        flyView: synclairFlyView
        toolbarVisible: toolbar ? toolbar.visible : false
    }

    Item {
        anchors.top: parent.top
        anchors.topMargin: toolbar && toolbar.visible ? toolbar.height + ScreenTools.defaultFontPixelHeight * 0.5 : 0
        anchors.right: parent.right
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth
        width: overlayToggle.width
        height: overlayToggle.height
        z: QGroundControl.zOrderTopMost

        MultiEffect {
            id: overlayGlow

            anchors.fill: overlayToggle
            source: overlayToggle
            visible: !SVState.synclairOverlay && !root.welcomePromptAcknowledged
            shadowEnabled: true
            shadowColor: QGroundControl.globalPalette.colorYellow
            shadowBlur: 1

            SequentialAnimation on shadowOpacity {
                running: overlayGlow.visible
                loops: Animation.Infinite
                NumberAnimation { from: 0; to: 1; duration: 1000; easing.type: Easing.InOutSine }
                NumberAnimation { from: 1; to: 0; duration: 1000; easing.type: Easing.InOutSine }
            }
        }

        QGCCheckBoxSlider {
            id: overlayToggle

            text: qsTr("Synclair Vision: QGroundControl")
            checked: SVState.synclairOverlay

            onToggled: {
                SVState.synclairOverlay = checked
                if (checked && !root.welcomePromptAcknowledged) {
                    root.showVideoFullScreen()
                    welcomePromptLoader.active = true
                }
            }
        }

        Loader {
            id: welcomePromptLoader

            active: false
            source: "qrc:/qml/QGroundControl/SynclairVisionUI/Flyview/SVWelcomePrompt.qml"
            onLoaded: item.open()
        }
    }
}
