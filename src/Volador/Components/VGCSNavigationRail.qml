/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Aerospace Mission-Control Navigation Rail
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: navRailRoot

    property int activeIndex: 0
    property bool isCompact: true

    implicitWidth: 76
    width: implicitWidth
    color: "#0A0F14"
    border.color: "#2C3847"
    border.width: 1
    z: 100

    signal navigationTriggered(int index, string routeId)

    // Synchronize activeIndex with window state
    Connections {
        target: (typeof mainWindow !== "undefined") ? mainWindow : null
        ignoreUnknownSignals: true
        function onFlyViewChanged() { syncActiveIndex() }
        function onPlanViewChanged() { syncActiveIndex() }
    }

    function syncActiveIndex() {
        if (typeof mainWindow === "undefined" || !mainWindow) return
        if (mainWindow.toolDrawer && mainWindow.toolDrawer.visible) {
            var title = mainWindow.toolDrawer.toolTitle || ""
            if (title.indexOf("Vehicle") >= 0 || title.indexOf("Fleet") >= 0) activeIndex = 2
            else if (title.indexOf("Analyze") >= 0 || title.indexOf("Telemetry") >= 0) activeIndex = 4
            else if (title.indexOf("Video") >= 0) activeIndex = 5
            else if (title.indexOf("Log") >= 0) activeIndex = 6
            else if (title.indexOf("Settings") >= 0 || title.indexOf("Application") >= 0) activeIndex = 7
        } else if (mainWindow.planView && mainWindow.planView.visible) {
            activeIndex = 1
        } else if (mainWindow.flyView && mainWindow.flyView.visible) {
            activeIndex = 0
        }
    }

    function handleNavClick(index) {
        activeIndex = index
        if (typeof mainWindow === "undefined" || !mainWindow) return

        switch (index) {
        case 0: // FLIGHT
            if (mainWindow.toolDrawer) mainWindow.toolDrawer.visible = false
            mainWindow.showFlyView()
            break
        case 1: // MISSIONS
            if (mainWindow.toolDrawer) mainWindow.toolDrawer.visible = false
            mainWindow.showPlanView()
            break
        case 2: // VEHICLES
            mainWindow.showVehicleConfig()
            break
        case 3: // MAP
            if (mainWindow.toolDrawer) mainWindow.toolDrawer.visible = false
            mainWindow.showFlyView()
            break
        case 4: // TELEMETRY
            mainWindow.showAnalyzeTool()
            break
        case 5: // VIDEO
            mainWindow.showSettingsTool("Video")
            break
        case 6: // LOGS
            mainWindow.showAnalyzeTool()
            break
        case 7: // SETTINGS
            mainWindow.showSettingsTool()
            break
        }
        navRailRoot.navigationTriggered(index, getRouteName(index))
    }

    function getRouteName(index) {
        switch (index) {
        case 0: return "flight"
        case 1: return "missions"
        case 2: return "vehicles"
        case 3: return "map"
        case 4: return "telemetry"
        case 5: return "video"
        case 6: return "logs"
        case 7: return "settings"
        default: return "flight"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // TOP: VGCS Compact Volador Logo Emblem
        Rectangle {
            Layout.fillWidth: true
            height: 64
            color: "transparent"

            Rectangle {
                anchors.centerIn: parent
                width: 44
                height: 44
                radius: 10
                color: "#151C24"
                border.color: emblemMouse.containsMouse ? ThemeController.accent : "#2C3847"
                border.width: 1

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/Volador/Assets/Logos/volador_compact.png"
                    width: 28
                    height: 28
                    fillMode: Image.PreserveAspectFit
                    antialiasing: true
                    mipmap: true
                }

                MouseArea {
                    id: emblemMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: handleNavClick(0)
                }

                ToolTip {
                    visible: emblemMouse.containsMouse
                    delay: 400
                    text: "Volador Ground Control Station"
                    x: parent.width + 12
                    y: (parent.height - height) / 2
                    contentItem: Text { text: "Volador GCS"; font.family: "Inter"; font.pixelSize: 11; color: "#F5F7FA" }
                    background: Rectangle { color: "#1D2733"; border.color: "#2C3847"; radius: 4 }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#2C3847"
        }

        // CENTER: Primary Navigation Items
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Flickable {
                anchors.fill: parent
                contentHeight: navCol.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: navCol
                    width: parent.width
                    spacing: 4
                    anchors.top: parent.top
                    anchors.topMargin: 8

                    // 1. FLIGHT
                    VGCSNavigationItem {
                        itemIndex: 0
                        iconSource: "qrc:/InstrumentValueIcons/airplane.svg"
                        itemLabel: "FLIGHT"
                        tooltipText: "Flight View"
                        isActive: navRailRoot.activeIndex === 0
                        onItemClicked: (idx) => navRailRoot.handleNavClick(idx)
                    }

                    // 2. MISSIONS
                    VGCSNavigationItem {
                        itemIndex: 1
                        iconSource: "qrc:/qmlimages/Plan.svg"
                        itemLabel: "MISSIONS"
                        tooltipText: "Mission Planning"
                        isActive: navRailRoot.activeIndex === 1
                        onItemClicked: (idx) => navRailRoot.handleNavClick(idx)
                    }

                    // 3. VEHICLES
                    VGCSNavigationItem {
                        itemIndex: 2
                        iconSource: "qrc:/InstrumentValueIcons/drone.svg"
                        itemLabel: "VEHICLES"
                        tooltipText: "Vehicle Management"
                        isActive: navRailRoot.activeIndex === 2
                        onItemClicked: (idx) => navRailRoot.handleNavClick(idx)
                    }

                    // 4. MAP
                    VGCSNavigationItem {
                        itemIndex: 3
                        iconSource: "qrc:/InstrumentValueIcons/globe.svg"
                        itemLabel: "MAP"
                        tooltipText: "Map Workspace"
                        isActive: navRailRoot.activeIndex === 3
                        onItemClicked: (idx) => navRailRoot.handleNavClick(idx)
                    }

                    // 5. TELEMETRY
                    VGCSNavigationItem {
                        itemIndex: 4
                        iconSource: "qrc:/InstrumentValueIcons/radar.svg"
                        itemLabel: "TELEMETRY"
                        tooltipText: "Telemetry & Live Data"
                        isActive: navRailRoot.activeIndex === 4
                        onItemClicked: (idx) => navRailRoot.handleNavClick(idx)
                    }

                    // 6. VIDEO
                    VGCSNavigationItem {
                        itemIndex: 5
                        iconSource: "qrc:/InstrumentValueIcons/video-camera.svg"
                        itemLabel: "VIDEO"
                        tooltipText: "Video Streams"
                        isActive: navRailRoot.activeIndex === 5
                        onItemClicked: (idx) => navRailRoot.handleNavClick(idx)
                    }

                    // 7. LOGS
                    VGCSNavigationItem {
                        itemIndex: 6
                        iconSource: "qrc:/InstrumentValueIcons/document.svg"
                        itemLabel: "LOGS"
                        tooltipText: "Flight & System Logs"
                        isActive: navRailRoot.activeIndex === 6
                        onItemClicked: (idx) => navRailRoot.handleNavClick(idx)
                    }
                }
            }
        }

        // BOTTOM: Divider & SETTINGS
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#2C3847"
        }

        Item {
            Layout.fillWidth: true
            height: 64

            VGCSNavigationItem {
                anchors.centerIn: parent
                itemIndex: 7
                iconSource: "qrc:/res/gear-white.svg"
                itemLabel: "SETTINGS"
                tooltipText: "Application Settings"
                isActive: navRailRoot.activeIndex === 7
                onItemClicked: (idx) => navRailRoot.handleNavClick(idx)
            }
        }
    }
}
