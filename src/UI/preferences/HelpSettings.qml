// Volador Branding
// Phase 1

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Help & Preferences Information
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools
import Volador.Theme 1.0

Rectangle {
    color:          VoladorTheme.primaryBackground
    anchors.fill:   parent

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCFlickable {
        anchors.margins:    _margins
        anchors.fill:       parent
        contentWidth:       mainColumn.implicitWidth
        contentHeight:      mainColumn.height
        clip:               true

        ColumnLayout {
            id:         mainColumn
            spacing:    _margins * 1.5
            Layout.fillWidth: true

            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     aboutColumn.height + (_margins * 2)
                color:              VoladorTheme.surface
                border.color:       VoladorTheme.border
                radius:             ScreenTools.defaultFontPixelWidth

                ColumnLayout {
                    id:                 aboutColumn
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    anchors.margins:    _margins
                    spacing:            ScreenTools.defaultFontPixelHeight / 2

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth * 2

                        Image {
                            source: "qrc:/Volador/Assets/Logos/volador_primary.png"
                            sourceSize.height: ScreenTools.defaultFontPixelHeight * 2.5
                            fillMode: Image.PreserveAspectFit
                            antialiasing: true
                            mipmap: true
                        }

                        ColumnLayout {
                            QGCLabel {
                                text:           VoladorTheme.shortName
                                font.pointSize: ScreenTools.largeFontPointSize
                                font.bold:      true
                                color:          VoladorTheme.primaryAccent
                            }

                            QGCLabel {
                                text:           VoladorTheme.productDescription
                                font.pointSize: ScreenTools.smallFontPointSize
                                color:          VoladorTheme.secondaryText
                            }
                        }
                    }

                    QGCLabel {
                        text:           qsTr("Enterprise Drone Mission Control Platform.")
                        font.pointSize: ScreenTools.defaultFontPointSize
                        wrapMode:       Text.WordWrap
                        color:          VoladorTheme.primaryText
                        Layout.fillWidth: true
                    }

                    QGCLabel {
                        text:           qsTr("Version %1 (MAVLink v2.0 Protocol)").arg(VoladorTheme.version)
                        font.pointSize: ScreenTools.smallFontPointSize
                        color:          VoladorTheme.secondaryText
                    }

                    QGCLabel {
                        text:           VoladorTheme.copyright
                        font.pointSize: ScreenTools.smallFontPointSize
                        color:          VoladorTheme.secondaryText
                    }
                }
            }

            QGCLabel {
                text:           qsTr("Resources & Documentation")
                font.pointSize: ScreenTools.mediumFontPointSize
                font.bold:      true
                color:          VoladorTheme.primaryText
            }

            GridLayout {
                id:         grid
                columns:    2
                Layout.fillWidth: true

                QGCLabel { text: qsTr("Official Website"); color: VoladorTheme.secondaryText }
                QGCLabel {
                    linkColor:          VoladorTheme.primaryAccent
                    text:               "<a href=\"" + VoladorTheme.website + "\">" + VoladorTheme.website + "</a>"
                    onLinkActivated:    (link) => Qt.openUrlExternally(link)
                }

                QGCLabel { text: qsTr("Technical Support"); color: VoladorTheme.secondaryText }
                QGCLabel {
                    linkColor:          VoladorTheme.primaryAccent
                    text:               "<a href=\"mailto:tech@volador.in\">tech@volador.in</a>"
                    onLinkActivated:    (link) => Qt.openUrlExternally(link)
                }

                QGCLabel { text: qsTr("Open Source Base & Licenses"); color: VoladorTheme.secondaryText }
                QGCLabel {
                    linkColor:          VoladorTheme.primaryText
                    text:               "QGroundControl (GPLv3/Apache 2.0) • MAVLink Protocol"
                }
            }
        }
    }
}
