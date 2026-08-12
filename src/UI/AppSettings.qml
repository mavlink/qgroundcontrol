/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL & VOLADOR AEROSPACE PROJECT
 *
 * Volador Ground Control Station - Modern Application Settings
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools
import VoladorTheme 1.0
import VoladorComponents 1.0

Rectangle {
    id: settingsView
    color: ThemeController.background
    z: QGroundControl.zOrderTopMost

    readonly property real _defaultTextHeight: ScreenTools.defaultFontPixelHeight
    readonly property real _defaultTextWidth:  ScreenTools.defaultFontPixelWidth

    property bool _first: true
    property bool _commingFromRIDSettings: false

    function showSettingsPage(settingsPage) {
        for (var i = 0; i < buttonRepeater.count; i++) {
            var button = buttonRepeater.itemAt(i)
            if (button.text === settingsPage) {
                button.clicked()
                break
            }
        }
    }

    QGCPalette { id: qgcPal }

    Component.onCompleted: {
        if (globals.commingFromRIDIndicator) {
            rightPanel.source = "qrc:/qml/RemoteIDSettings.qml"
            globals.commingFromRIDIndicator = false
        } else {
            rightPanel.source = "qrc:/qml/GeneralSettings.qml"
        }
    }

    SettingsPagesModel { id: settingsPagesModel }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // LEFT CATEGORIES RAIL (Width 250)
        Rectangle {
            Layout.fillHeight: true
            implicitWidth: 250
            color: ThemeController.sidebar
            border.color: ThemeController.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Text {
                    text: "SETTINGS CATEGORIES"
                    font.family: "Inter"
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    color: ThemeController.accent
                }

                SearchField {
                    Layout.fillWidth: true
                    placeholderText: "Search preferences..."
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: ThemeController.border }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 4

                        Repeater {
                            id: buttonRepeater
                            model: settingsPagesModel

                            SettingsButton {
                                Layout.fillWidth: true
                                text: name
                                icon.source: iconUrl
                                visible: pageVisible()

                                onClicked: {
                                    if (mainWindow.allowViewSwitch()) {
                                        if (rightPanel.source !== url) {
                                            rightPanel.source = url
                                        }
                                        checked = true
                                    }
                                }

                                Component.onCompleted: {
                                    if (globals.commingFromRIDIndicator) {
                                        _commingFromRIDSettings = true
                                    }
                                    if (_commingFromRIDSettings && text === "Remote ID") {
                                        checked = true
                                        _commingFromRIDSettings = false
                                    } else if (_first && text === "General") {
                                        checked = true
                                        _first = false
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // RIGHT PREFERENCES CARD CONTAINER
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: ThemeController.background

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Card {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    glass: true

                    Loader {
                        id: rightPanel
                        anchors.fill: parent
                        anchors.margins: 16
                    }
                }
            }
        }
    }
}
