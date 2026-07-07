import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap
//import QGroundControl.ScreenTools

Item {
    id: root

    property var parentToolInsets
    property real leftToolStripBottom: 0
    property string activeLayoutId: "four_square"
    property string activeSettingsId: ""
    property bool recordActive: false
    property var settingsModel: [
        { id: "General", text: "General", checkable: true, iconSource: "/qmlimages/settings_general.svg" },
        { id: "Controls", text: "Controls", checkable: true, iconSource: "/qmlimages/settings_controls.svg" },
        { id: "Dev", text: "Dev", checkable: true, iconSource: "/qmlimages/settings_dev.svg" }
    ]

    property bool activeDigiview: QGroundControl.videoManager.streaming

    signal layoutSelected(string layoutId)

    readonly property bool settingsPanelVisible: activeSettingsId !== ""
    readonly property var settingsOverlayParent: settingsModalOverlay.contentItem
    readonly property bool settingsInOverlay: settingsPanelVisible && !!settingsOverlayParent
    readonly property var activeSettingsEntry: {
        for (var index = 0; index < root.settingsModel.length; index++) {
            var entry = root.settingsModel[index]

            if (entry.id === activeSettingsId) {
                return entry
            }
        }

        return null
    }


    QGCToolInsets {
        id: _toolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }

    QGCPalette { id: qgcPalette }

    SVControlPanel {
        id: controlPanel
        anchors.bottom: root.bottom
        anchors.horizontalCenter: root.horizontalCenter
        visible: SVSettings.svHUD
    }

    Item {
        id: settingsHost
        anchors.top: parent.top
        anchors.right: parent.right
        visible: SVSettings.svHUD
        z: 3

        width: settings.width
        height: settings.height

        
    }

    

    SVMenuStrip {
        id: settingsMenu
        anchors.top: parent.top
        anchors.right: parent.right
        visible: SVSettings.svHUD


        menuText: "Settings"
        source: "/qmlimages/settings_main.svg"
        alternateSource: "/qmlimages/settings_main_open.svg"
        direction: vertical
        open: false
        autoUpdateActiveId: false
        activeId: root.activeSettingsId

        model: root.settingsModel

        onItemSelected: (id) => {
            activeSettingsId = (activeSettingsId === id) ? "" : id

            if(activeSettingsId !== "") {
                showIndicatorDrawer(settingsDrawer, settingsAnchor)
            }
        }
    }

    Item {
        id: settingsAnchor
        width: 1
        height: 1
        //anchors.horizontalCenter: parent.horizontalCenter
        //anchors.verticalCenter: parent.verticalCenter
        anchors.top: parent.top
        anchors.right: parent.right
        //anchors.rightMargin: 300 
    }

    Component {
        id: settingsDrawer 

        Item {
            id: settingsDrawerRoot

            property var drawer
            property bool indicatorDrawerUseRightEdgeAlignment: true
            property real indicatorDrawerRightEdgeMargin: -ScreenTools.defaultFontPixelHeight / 8 - 2
            readonly property real drawerSpacing: ScreenTools.defaultFontPixelWidth * 2
            property real settingsPanelWidth: 700
            property real settingsPanelHeight: 900
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
            readonly property real minimumSettingsPanelWidth: ScreenTools.defaultFontPixelWidth * 75
            readonly property real maximumSettingsPanelWidth: Math.max(minimumSettingsPanelWidth,
                                                                        drawerViewportWidth - settingsCategoryStrip.width - drawerSpacing - (ScreenTools.defaultFontPixelWidth * 12))
            readonly property real minimumSettingsPanelHeight: ScreenTools.defaultFontPixelHeight * 10
            readonly property real maximumSettingsPanelHeight: Math.max(minimumSettingsPanelHeight,
                                                                         drawerViewportHeight - (ScreenTools.defaultFontPixelHeight * 5))

            width: settingsPanelContainer.width + settingsCategoryStrip.width + drawerSpacing
            height: Math.max(settingsPanelContainer.height, settingsCategoryStrip.height)

            Component.onDestruction: root.activeSettingsId = ""

            Component.onCompleted: {
                settingsPanelWidth = Math.max(minimumSettingsPanelWidth, Math.min(settingsPanelWidth, maximumSettingsPanelWidth))
                settingsPanelHeight = Math.max(minimumSettingsPanelHeight, Math.min(settingsPanelHeight, maximumSettingsPanelHeight))
            }

            onMaximumSettingsPanelWidthChanged: {
                if (settingsPanelWidth > maximumSettingsPanelWidth) {
                    settingsPanelWidth = maximumSettingsPanelWidth
                }
            }

            onMaximumSettingsPanelHeightChanged: {
                if (settingsPanelHeight > maximumSettingsPanelHeight) {
                    settingsPanelHeight = maximumSettingsPanelHeight
                }
            }

            Row {
                spacing: parent.drawerSpacing

                Item {
                    id: settingsPanelContainer
                    width: Math.max(settingsDrawerRoot.minimumSettingsPanelWidth,
                                    Math.min(settingsDrawerRoot.settingsPanelWidth, settingsDrawerRoot.maximumSettingsPanelWidth))
                    height: Math.max(settingsDrawerRoot.minimumSettingsPanelHeight,
                                     Math.min(settingsDrawerRoot.settingsPanelHeight, settingsDrawerRoot.maximumSettingsPanelHeight))

                    SVSettingsMenu {
                        id: settingsPanel
                        anchors.fill: parent

                        activeSettingsId: root.activeSettingsId
                    }

                    Rectangle {
                        id: settingsResizeHandle
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        //anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
                        //anchors.bottomMargin: ScreenTools.defaultFontPixelWidth / 2
                        width: ScreenTools.defaultFontPixelHeight * 1.8
                        height: width
                        radius: ScreenTools.defaultBorderRadius
                        color: Qt.rgba(0, 0, 0, 0.18)
                        border.width: 1
                        border.color: qgcPalette.windowShadeLight
                        z: 1

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width * 0.65
                            height: 1
                            rotation: -45
                            color: qgcPalette.text
                            opacity: 0.8
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

                                settingsDrawerRoot.settingsPanelWidth = Math.max(settingsDrawerRoot.minimumSettingsPanelWidth,
                                                                                 Math.min(nextWidth, settingsDrawerRoot.maximumSettingsPanelWidth))
                                settingsDrawerRoot.settingsPanelHeight = Math.max(settingsDrawerRoot.minimumSettingsPanelHeight,
                                                                                  Math.min(nextHeight, settingsDrawerRoot.maximumSettingsPanelHeight))
                            }
                        }
                    }
                }

                SVMenuStrip {
                    id: settingsCategoryStrip
                    transform: Translate { y: ScreenTools.defaultFontPixelWidth * 7 - ScreenTools.defaultFontPixelWidth / 2 - 1 }

                    headerless: true
                    autoUpdateActiveId: false
                    direction: vertical
                    activeId: root.activeSettingsId
                    model: root.settingsModel

                    onItemSelected: (id) => {
                        if (root.activeSettingsId === id) {
                            closeIndicatorDrawer()
                            return
                        }

                        root.activeSettingsId = id
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.width: 1
                        border.color: qgcPalette.windowShadeLight
                        radius: 5
                    }
                }
            }

        }
    }



    SVMenuStrip {
        id: oneshots
        headerless: true
        exclusiveSelection: false
        autoUpdateActiveId: false
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 7 + ScreenTools.defaultFontPixelWidth * 0.5

        direction: horizontal

        activeIds: {
            var ids = []
            if (!SVSettings.svHUD) ids.push("hud")
            if (root.recordActive) ids.push("record")
            if (!SVSettings.svToolbar) ids.push("toolbar")
            return ids
        }

        model: [
            { 
                id: "hud",
                text: "HUD",
                checkable: true,
                iconSource: "/qmlimages/hud_eye.svg",
                alternateIconSource: "/qmlimages/hud_eye_closed.svg",
                iconActive: !SVSettings.svHUD,
                enabled: true
            },
            { 
                id: "toolbar",
                text: "Toolbar",
                checkable: true,
                iconSource: "/qmlimages/toolbar_open.svg",
                alternateIconSource: "/qmlimages/toolbar_closed.svg",
                iconActive: !SVSettings.svToolbar,
                enabled: true
            },
            { 
                id: "photo",  
                text: "Photo",  
                checkable: false, 
                iconSource: "/qmlimages/camera_photo.svg", 
                enabled: activeDigiview
            },
            { 
                id: "record", 
                text: "Record", 
                checkable: true, 
                iconSource: "/qmlimages/camera_record.svg", 
                enabled: activeDigiview
            }
        ]

        onItemSelected: (id) => {
            if (id === "hud") {
                SVSettings.svHUD = !SVSettings.svHUD
                return
            }

            if (id === "record") {
                root.recordActive = !root.recordActive
                return
            }

            if (id === "toolbar") {
                SVSettings.svToolbar = !SVSettings.svToolbar
                return
            }
        }
    }

    SVMenuStrip {
        id: layout
        anchors.top: parent.top
        anchors.topMargin: root.leftToolStripBottom + 5
        anchors.left: parent.left
        visible: SVSettings.svHUD


        menuText: "Layout"
        source: "/qmlimages/layout_main.svg"
        direction: vertical
        open: false
        autoUpdateActiveId: false
        activeId: root.activeLayoutId

        model: [
            { id: "single",                     checkable: true, iconSource: "/qmlimages/layout_single.svg" },
            { id: "two_stacked_square",         checkable: true, iconSource: "/qmlimages/layout_double.svg" },
            { id: "four_square",                checkable: true, iconSource: "/qmlimages/layout_quadruple.svg" },
            { id: "two_stacked_panorama",       checkable: true, iconSource: "/qmlimages/layout_double_panorama.svg" },
            { id: "two_square_one_panorama",    checkable: true, iconSource: "/qmlimages/layout_double+panorama.svg" },
            { id: "three_square_one_panorama",  checkable: true, iconSource: "/qmlimages/layout_triple+panorama.svg" },
            { id: "entire_picture",             checkable: true, iconSource: "/qmlimages/layout_single_panorama.svg" }
        ]

        onItemSelected: (id) => {
            root.layoutSelected(id)
        }
    }
}
