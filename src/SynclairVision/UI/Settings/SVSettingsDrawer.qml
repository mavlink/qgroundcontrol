import QtQuick
import QtQuick.Shapes 2.15


import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property string activeSettingsId: ""
    property var settingsModel: []

    property real offsetX: 0
    property real offsetY: 0

    signal settingsSelected(string settingsId)
    signal dismissed()

    function openDrawer() {
        mainWindow.showIndicatorDrawer(settingsDrawer, settingsAnchor)
    }

    QGCPalette { id: qgcPalette }

    Item {
        id: settingsAnchor
        width: 1
        height: 1
        anchors.top: parent.top
        anchors.right: parent.right
    }

    Component {
        id: settingsDrawer

        Item {
            id: settingsDrawerRoot

            property var drawer
            property bool indicatorDrawerUseRightEdgeAlignment: true
            property real indicatorDrawerRightEdgeMargin: -(SVUnits.height / 8) - (SVUnits.lineWidth * 2)
            readonly property real drawerSpacing: SVUnits.bigMargin * 2
            readonly property real settingsCategoryStripTopOffset: root.offsetY //SVUnits.objectWidth - SVUnits.margin  -  2.5 * SVUnits.lineWidth
            readonly property real minimumSettingsPanelWidth: SVUnits.width * 75
            readonly property real maximumSettingsPanelWidth: Math.max(minimumSettingsPanelWidth,
                                                                        drawerViewportWidth - settingsCategoryStripContainer.width - drawerSpacing - (SVUnits.width * 12))
            readonly property real minimumSettingsPanelHeight: SVUnits.height * 12
            readonly property real maximumSettingsPanelHeight: Math.max(minimumSettingsPanelHeight,
                                                                         drawerViewportHeight - (SVUnits.height * 5))
            property real settingsPanelWidth: minimumSettingsPanelWidth
            property real settingsPanelHeight: SVUnits.height * 50
            readonly property real drawerViewportWidth: {
                if (!drawer || !drawer.parent || (drawer.parent.width <= 0)) {
                    return root.width
                }

                return drawer.parent.width - indicatorDrawerRightEdgeMargin - (drawer.padding * 2)
            }
            readonly property real drawerViewportHeight: {
                if (!drawer || !drawer.parent || (drawer.parent.height <= 0)) {
                    return root.height
                }

                return drawer.parent.height - drawer.y - (drawer.padding * 2) - drawer._margins
            }
            function clamp(value, minimum, maximum) {
                return Math.max(minimum, Math.min(value, maximum))
            }

            width: settingsPanelContainer.width + settingsCategoryStripContainer.width + drawerSpacing
            height: Math.max(settingsPanelContainer.height, settingsCategoryStripContainer.height)

            Component.onDestruction: root.dismissed()

            Component.onCompleted: {
                settingsPanelWidth = clamp(settingsPanelWidth, minimumSettingsPanelWidth, maximumSettingsPanelWidth)
                settingsPanelHeight = clamp(settingsPanelHeight, minimumSettingsPanelHeight, maximumSettingsPanelHeight)
            }

            onMaximumSettingsPanelWidthChanged: {
                if (settingsPanelWidth > maximumSettingsPanelWidth) {
                    settingsPanelWidth = clamp(settingsPanelWidth, minimumSettingsPanelWidth, maximumSettingsPanelWidth)
                }
            }

            onMaximumSettingsPanelHeightChanged: {
                if (settingsPanelHeight > maximumSettingsPanelHeight) {
                    settingsPanelHeight = clamp(settingsPanelHeight, minimumSettingsPanelHeight, maximumSettingsPanelHeight)
                }
            }

            Row {
                spacing: parent.drawerSpacing


                Item {
                    id: settingsPanelContainer
                    width: settingsDrawerRoot.clamp(settingsDrawerRoot.settingsPanelWidth,
                                                    settingsDrawerRoot.minimumSettingsPanelWidth,
                                                    settingsDrawerRoot.maximumSettingsPanelWidth)
                    height: settingsDrawerRoot.clamp(settingsDrawerRoot.settingsPanelHeight,
                                                     settingsDrawerRoot.minimumSettingsPanelHeight,
                                                     settingsDrawerRoot.maximumSettingsPanelHeight)

                    SVSettingsMenu {
                        id: settingsPanel
                        anchors.fill: parent
                        activeSettingsId: root.activeSettingsId
                    }

                    Item {
                        id: settingsResizeHandle
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        width: SVUnits.objectWidth / 1.5
                        height: SVUnits.objectWidth / 1.5
                        readonly property color handleColor: qgcPalette.windowShadeLight

                        Shape {
                            anchors.fill: parent
                            antialiasing: true

                            ShapePath {
                                strokeWidth: 0
                                fillColor: Qt.alpha(settingsResizeHandle.handleColor, 0.35)
                                startX: 0
                                startY: 0
                                PathLine { x: settingsResizeHandle.width; y: settingsResizeHandle.height }
                                PathLine { x: 0; y: settingsResizeHandle.height }
                                PathLine { x: 0; y: 0 }
                            }

                            ShapePath {
                                strokeWidth: 1
                                strokeColor: qgcPalette.windowShadeLight
                                fillColor: "transparent"
                                startX: 0
                                startY: 0
                                PathLine { x: settingsResizeHandle.width; y: settingsResizeHandle.height }
                            }
                        }

                        HoverHandler {
                            cursorShape: Qt.SizeFDiagCursor
                        }

                        DragHandler {
                            acceptedButtons: Qt.LeftButton
                            target: null
                            cursorShape: Qt.SizeFDiagCursor

                            property real initialWidth: 0
                            property real initialHeight: 0

                            onActiveChanged: {
                                if (!active) {
                                    return
                                }

                                initialWidth = settingsPanel.width
                                initialHeight = settingsPanel.height
                            }

                            onActiveTranslationChanged: {
                                if (!active) {
                                    return
                                }

                                var nextWidth = initialWidth - activeTranslation.x
                                var nextHeight = initialHeight + activeTranslation.y

                                settingsDrawerRoot.settingsPanelWidth = settingsDrawerRoot.clamp(nextWidth,
                                                                                                  settingsDrawerRoot.minimumSettingsPanelWidth,
                                                                                                  settingsDrawerRoot.maximumSettingsPanelWidth)
                                settingsDrawerRoot.settingsPanelHeight = settingsDrawerRoot.clamp(nextHeight,
                                                                                                   settingsDrawerRoot.minimumSettingsPanelHeight,
                                                                                                   settingsDrawerRoot.maximumSettingsPanelHeight)
                            }
                        }
                    }
                }

                Item {
                    id: settingsCategoryStripContainer
                    width: settingsCategoryStrip.width + root.offsetX
                    height: settingsDrawerRoot.settingsCategoryStripTopOffset + settingsCategoryStrip.height

                    SVMenuStrip {
                        id: settingsCategoryStrip
                        anchors.top: parent.top
                        anchors.topMargin: settingsDrawerRoot.settingsCategoryStripTopOffset
                        anchors.left: parent.left
                        anchors.leftMargin: 0

                        headerless: true
                        autoUpdateActiveId: false
                        direction: vertical
                        activeId: root.activeSettingsId
                        model: root.settingsModel

                        onItemSelected: (id) => {
                            if (root.activeSettingsId === id) {
                                mainWindow.closeIndicatorDrawer()
                                return
                            }

                            root.settingsSelected(id)
                        }

                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            border.width: SVUnits.lineWidth
                            border.color: qgcPalette.windowShadeLight
                            radius: SVUnits.radius
                        }
                    }
                }
            }
        }
    }
}
