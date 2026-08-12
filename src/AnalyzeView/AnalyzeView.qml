/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL & VOLADOR AEROSPACE PROJECT
 *
 * Volador Ground Control Station - Dark Engineering Console (Analyze View)
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.Controllers
import QGroundControl.ScreenTools
import VoladorTheme 1.0
import VoladorComponents 1.0

Rectangle {
    id: _root
    color: ThemeController.isDark ? "#111111" : "#F5F7FA"
    z: QGroundControl.zOrderTopMost

    signal popout()

    readonly property real  _defaultTextHeight: ScreenTools.defaultFontPixelHeight
    readonly property real  _defaultTextWidth:  ScreenTools.defaultFontPixelWidth

    GeoTagController {
        id: geoController
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // LEFT ENGINEERING NAV BAR (Width 240)
        Rectangle {
            Layout.fillHeight: true
            implicitWidth: 240
            color: ThemeController.sidebar
            border.color: ThemeController.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Text {
                    text: "ANALYZE & DIAGNOSTICS"
                    font.family: "Inter"
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    color: ThemeController.accent
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: ThemeController.border }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 6

                        Repeater {
                            id: buttonRepeater
                            model: QGroundControl.corePlugin ? QGroundControl.corePlugin.analyzePages : []

                            Component.onCompleted: {
                                if (count > 0) itemAt(0).checked = true
                            }

                            SubMenuButton {
                                id: subMenu
                                Layout.fillWidth: true
                                imageResource: modelData.icon
                                autoExclusive: true
                                text: modelData.title
                                onClicked: {
                                    panelLoader.source = modelData.url
                                    panelTitleText.text = modelData.title
                                    checked = true
                                }
                            }
                        }
                    }
                }
            }
        }

        // RIGHT ENGINEERING PANEL
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: ThemeController.background

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                // Header Bar for Active Tool
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        id: panelTitleText
                        text: "MAVLink Inspector"
                        font.family: "Inter"
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        color: ThemeController.textPrimary
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: "MAVLink v2.0 Protocol Active"
                        font.family: "JetBrains Mono"
                        font.pixelSize: 12
                        color: ThemeController.success
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: ThemeController.border }

                // Content Panel Container
                Card {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    glass: true

                    Loader {
                        id: panelLoader
                        anchors.fill: parent
                        anchors.margins: 8
                        source: (QGroundControl.corePlugin && QGroundControl.corePlugin.analyzePages.length > 0) ? QGroundControl.corePlugin.analyzePages[0].url : ""
                    }
                }
            }
        }
    }
}
