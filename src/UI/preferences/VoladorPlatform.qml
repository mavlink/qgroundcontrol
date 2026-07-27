/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control - Company Platform & Operations Hub
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

Rectangle {
    id:             root
    color:          qgcPal.window
    anchors.fill:   parent

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCFlickable {
        anchors.margins:    _margins
        anchors.fill:       parent
        contentWidth:       mainColumn.width
        contentHeight:      mainColumn.height
        clip:               true

        ColumnLayout {
            id:         mainColumn
            spacing:    _margins * 1.5
            width:      Math.max(600, parent.width - (_margins * 2))

            // Header Banner
            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     bannerColumn.height + (_margins * 2)
                color:              qgcPal.windowShade
                radius:             ScreenTools.defaultFontPixelWidth

                ColumnLayout {
                    id:                 bannerColumn
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    anchors.margins:    _margins
                    spacing:            ScreenTools.defaultFontPixelHeight / 2

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth * 2

                        Image {
                            source:         "qrc:/res/QGCLogoFull.svg"
                            sourceSize.height: ScreenTools.defaultFontPixelHeight * 2.5
                            fillMode:       Image.PreserveAspectFit
                        }

                        ColumnLayout {
                            QGCLabel {
                                text:           "VOLADOR AEROSPACE PLATFORM"
                                font.pointSize: ScreenTools.largeFontPointSize
                                font.bold:      true
                                color:          qgcPal.colorOrange
                            }
                            QGCLabel {
                                text:           qsTr("Enterprise Mission Control & Autonomous Fleet Intelligence")
                                font.pointSize: ScreenTools.smallFontPointSize
                                color:          qgcPal.text
                            }
                        }
                    }
                }
            }

            // Navigation Tabs
            RowLayout {
                spacing: _margins

                QGCButton {
                    text:               qsTr("Dashboard")
                    primary:            currentTab === 0
                    onClicked:          currentTab = 0
                }
                QGCButton {
                    text:               qsTr("About Volador")
                    primary:            currentTab === 1
                    onClicked:          currentTab = 1
                }
                QGCButton {
                    text:               qsTr("Documentation")
                    primary:            currentTab === 2
                    onClicked:          currentTab = 2
                }
                QGCButton {
                    text:               qsTr("Support & Contact")
                    primary:            currentTab === 3
                    onClicked:          currentTab = 3
                }
                QGCButton {
                    text:               qsTr("Release Notes")
                    primary:            currentTab === 4
                    onClicked:          currentTab = 4
                }
            }

            property int currentTab: 0

            // Tab Contents
            // Tab 0: Dashboard Overview
            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     dashCol.height + (_margins * 2)
                color:              qgcPal.windowShadeDark
                radius:             ScreenTools.defaultFontPixelWidth
                visible:            mainColumn.currentTab === 0

                ColumnLayout {
                    id:                 dashCol
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    anchors.margins:    _margins
                    spacing:            _margins

                    QGCLabel {
                        text:           qsTr("Operational Overview")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        font.bold:      true
                        color:          qgcPal.colorOrange
                    }

                    GridLayout {
                        columns: 3
                        Layout.fillWidth: true

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 80
                            color: qgcPal.windowShade
                            radius: 4

                            ColumnLayout {
                                anchors.centerIn: parent
                                QGCLabel { text: "Active Fleet Status"; font.bold: true }
                                QGCLabel { text: "All Systems Operational"; color: qgcPal.colorGreen }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 80
                            color: qgcPal.windowShade
                            radius: 4

                            ColumnLayout {
                                anchors.centerIn: parent
                                QGCLabel { text: "Mission Mode"; font.bold: true }
                                QGCLabel { text: "Autonomous Flight Ready"; color: qgcPal.colorOrange }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 80
                            color: qgcPal.windowShade
                            radius: 4

                            ColumnLayout {
                                anchors.centerIn: parent
                                QGCLabel { text: "Cloud Sync"; font.bold: true }
                                QGCLabel { text: "Connected (volador.in)"; color: qgcPal.text }
                            }
                        }
                    }
                }
            }

            // Tab 1: About Volador
            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     aboutCol.height + (_margins * 2)
                color:              qgcPal.windowShadeDark
                radius:             ScreenTools.defaultFontPixelWidth
                visible:            mainColumn.currentTab === 1

                ColumnLayout {
                    id:                 aboutCol
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    anchors.margins:    _margins
                    spacing:            _margins / 2

                    QGCLabel {
                        text:           qsTr("About Volador Aerospace")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        font.bold:      true
                        color:          qgcPal.colorOrange
                    }

                    QGCLabel {
                        text:           qsTr("Volador Aerospace builds enterprise-grade UAV mission control solutions, autonomous flight intelligence platforms, and drone operations software tailored for survey, inspection, agriculture, and defense applications.")
                        wrapMode:       Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            // Tab 2: Documentation
            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     docCol.height + (_margins * 2)
                color:              qgcPal.windowShadeDark
                radius:             ScreenTools.defaultFontPixelWidth
                visible:            mainColumn.currentTab === 2

                ColumnLayout {
                    id:                 docCol
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    anchors.margins:    _margins
                    spacing:            _margins / 2

                    QGCLabel {
                        text:           qsTr("Platform Documentation")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        font.bold:      true
                        color:          qgcPal.colorOrange
                    }

                    QGCLabel {
                        text:           qsTr("Access complete user manuals, payload integration guides, and flight safety instructions at https://volador.in/docs")
                        wrapMode:       Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            // Tab 3: Support & Contact
            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     supCol.height + (_margins * 2)
                color:              qgcPal.windowShadeDark
                radius:             ScreenTools.defaultFontPixelWidth
                visible:            mainColumn.currentTab === 3

                ColumnLayout {
                    id:                 supCol
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    anchors.margins:    _margins
                    spacing:            _margins / 2

                    QGCLabel {
                        text:           qsTr("Technical Support & Operations Contact")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        font.bold:      true
                        color:          qgcPal.colorOrange
                    }

                    QGCLabel { text: qsTr("Official Website: https://volador.in") }
                    QGCLabel { text: qsTr("Support Email: tech@volador.in") }
                    QGCLabel { text: qsTr("Emergency Ops Desk: +91 (Volador Ops Support)") }
                }
            }

            // Tab 4: Release Notes
            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     relCol.height + (_margins * 2)
                color:              qgcPal.windowShadeDark
                radius:             ScreenTools.defaultFontPixelWidth
                visible:            mainColumn.currentTab === 4

                ColumnLayout {
                    id:                 relCol
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    anchors.margins:    _margins
                    spacing:            _margins / 2

                    QGCLabel {
                        text:           qsTr("Release Notes - Volador Ground Control v5.0")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        font.bold:      true
                        color:          qgcPal.colorOrange
                    }

                    QGCLabel {
                        text:           qsTr("- Custom Volador Aerospace Branding & High-Contrast Matte Black / Neon Orange UI Theme\n- GStreamer 1.0 & Qt 6.8.3 native integration\n- Fleet & Mission Management Platform Framework")
                        wrapMode:       Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
