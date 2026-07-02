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

Item {
    id: root

    property var parentToolInsets
    property real leftToolStripBottom: 0
    property string activeLayoutId: "four_square"
    property string activeSettingsId: ""
    property bool recordActive: false
    property var settingsModel: [
        { id: "general",  text: "General",  checkable: true },
        { id: "conotrols",  text: "Controls",  checkable: true },
        { id: "dev", text: "Dev", checkable: true }
    ]

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
        visible: SVState.svHUD
    }

    Item {
        id: settingsHost
        anchors.top: parent.top
        anchors.right: parent.right
        visible: SVState.svHUD
        z: 3

        width: settings.width
        height: settings.height
    }

    SVMenuStrip {
        id: settingsMenu
        anchors.top: parent.top
        anchors.right: parent.right
        visible: SVState.svHUD


        menuText: "Settings"
        source: "/qmlimages/settings_main.svg"
        direction: vertical
        open: false
        autoUpdateActiveId: false
        activeId: root.activeSettingsId

        model: [
            { id: "General",  text: "General",  checkable: true, iconSource: "/qmlimages/settings_general.svg" },
            { id: "Controls", text: "Controls", checkable: true, iconSource: "/qmlimages/settings_controls.svg" },
            { id: "Dev",      text: "Dev",      checkable: true, iconSource: "/qmlimages/settings_dev.svg" },
        ]

        onItemSelected: (id) => {
            activeSettingsId = (activeSettingsId === id) ? "" : id
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
            if (!SVState.svHUD) ids.push("hud")
            if (root.recordActive) ids.push("record")
            return ids
        }

        model: [
            { 
                id: "hud",
                text: "HUD",
                checkable: true,
                iconSource: "/qmlimages/hud_eye.svg",
                alternateIconSource: "/qmlimages/hud_eye_closed.svg",
                iconActive: !SVState.svHUD
            },
            { 
                id: "photo",  
                text: "Photo",  
                checkable: false, 
                iconSource: "/qmlimages/camera_photo.svg" 
            },
            { 
                id: "record", 
                text: "Record", 
                checkable: true, 
                iconSource: "/qmlimages/camera_record.svg" 
            }
        ]

        onItemSelected: (id) => {
            if (id === "hud") {
                SVState.svHUD = !SVState.svHUD
                return
            }

            if (id === "record") {
                root.recordActive = !root.recordActive
                return
            }
        }
    }

    SVMenuStrip {
        id: layout
        anchors.top: parent.top
        anchors.topMargin: root.leftToolStripBottom + 5
        anchors.left: parent.left
        visible: SVState.svHUD


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