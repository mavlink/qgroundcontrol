/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Master Application Shell Layout
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import VoladorTheme 1.0
import VoladorComponents 1.0

VoladorWindowFrame {
    id: mainRoot

    // Staggered Fade Reveals
    property bool startupCompleted: (typeof voladorStartup !== "undefined" && voladorStartup) ? voladorStartup.isCompleted : false

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Custom Enterprise Title Bar
        VoladorTitleBar {
            id: customTitleBar
            Layout.fillWidth: true
            z: 10

            // Fade in as logo morph transition completes
            opacity: startupOverlay.visible ? 0.0 : 1.0
            Behavior on opacity {
                NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
            }
        }

        // Sub-Header Action Toolbar
        VoladorToolbar {
            id: actionToolbar
            Layout.fillWidth: true
            z: 5

            opacity: startupOverlay.visible ? 0.0 : 1.0
            Behavior on opacity {
                NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
            }
        }

        // Main Workspace Layout (Sidebar Rail + Display View Area)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Left Industrial Navigation Rail
            Sidebar {
                id: sidebarNav
                Layout.fillHeight: true
                visible: true

                opacity: startupOverlay.visible ? 0.0 : 1.0
                Behavior on opacity {
                    NumberAnimation { duration: 350; easing.type: Easing.InOutQuad }
                }

                onPageSelected: function(index, pageUrl) {
                    if (pageUrl === "showFlyView") {
                        if (typeof mainWindow !== "undefined" && mainWindow.showFlyView) mainWindow.showFlyView()
                        contentLoader.visible = false
                    } else if (pageUrl === "showPlanView") {
                        if (typeof mainWindow !== "undefined" && mainWindow.showPlanView) mainWindow.showPlanView()
                        contentLoader.visible = false
                    } else {
                        if (typeof mainWindow !== "undefined") {
                            if (mainWindow.flyView) mainWindow.flyView.visible = false
                            if (mainWindow.planView) mainWindow.planView.visible = false
                        }
                        contentLoader.visible = true
                        contentLoader.source = pageUrl
                    }
                }
            }

            // Main View Display Area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: ThemeController.background

                opacity: startupOverlay.visible ? 0.0 : 1.0
                Behavior on opacity {
                    NumberAnimation { duration: 400; easing.type: Easing.InOutQuad }
                }

                Loader {
                    id: contentLoader
                    anchors.fill: parent
                    source: "qrc:/qml/VoladorDashboardView.qml"

                    Behavior on opacity {
                        NumberAnimation { duration: 150 }
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // CINEMATIC STARTUP OVERLAY
    // -------------------------------------------------------------------------
    VoladorStartupOverlay {
        id: startupOverlay
        anchors.fill: parent
    }
}
