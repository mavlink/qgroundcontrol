// Volador Branding
// Phase 1

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Branded About Dialog
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Volador.Theme 1.0
import QGroundControl 1.0

Rectangle {
    id: aboutDialogRoot
    implicitWidth: 640
    implicitHeight: 480
    color: VoladorTheme.primaryBackground
    border.color: VoladorTheme.border
    border.width: 1
    radius: 8

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        // Header Section
        RowLayout {
            spacing: 16
            Layout.fillWidth: true

            Image {
                source: "qrc:/Volador/Assets/Logos/volador_primary.png"
                implicitWidth: 160
                implicitHeight: 56
                fillMode: Image.PreserveAspectFit
                antialiasing: true
                mipmap: true
            }

            ColumnLayout {
                spacing: 2

                Text {
                    text: VoladorTheme.shortName
                    font.family: VoladorTheme.fontPrimary
                    font.pixelSize: VoladorTheme.fontSizeHeading
                    font.bold: true
                    color: VoladorTheme.primaryText
                }

                Text {
                    text: VoladorTheme.productDescription
                    font.family: VoladorTheme.fontPrimary
                    font.pixelSize: VoladorTheme.fontSizeBody
                    color: VoladorTheme.secondaryText
                }

                Text {
                    text: VoladorTheme.company + " • " + VoladorTheme.website
                    font.family: VoladorTheme.fontPrimary
                    font.pixelSize: VoladorTheme.fontSizeSmall
                    color: VoladorTheme.primaryAccent
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: VoladorTheme.border
        }

        // Version & Build Technical Information
        GridLayout {
            columns: 2
            rowSpacing: 8
            columnSpacing: 24
            Layout.fillWidth: true

            Text {
                text: "Product Name:"
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.secondaryText
            }
            Text {
                text: VoladorTheme.brandName
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                font.bold: true
                color: VoladorTheme.primaryText
            }

            Text {
                text: "Short Name:"
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.secondaryText
            }
            Text {
                text: VoladorTheme.shortName
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                font.bold: true
                color: VoladorTheme.primaryText
            }

            Text {
                text: "Company:"
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.secondaryText
            }
            Text {
                text: VoladorTheme.company
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                font.bold: true
                color: VoladorTheme.primaryText
            }

            Text {
                text: "Application Version:"
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.secondaryText
            }
            Text {
                text: VoladorTheme.version
                font.family: VoladorTheme.fontMono
                font.pixelSize: VoladorTheme.fontSizeBody
                font.bold: true
                color: VoladorTheme.primaryText
            }

            Text {
                text: "Qt Version:"
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.secondaryText
            }
            Text {
                text: (typeof voladorCore !== "undefined" && voladorCore) ? voladorCore.qtVersion : "6.8.3"
                font.family: VoladorTheme.fontMono
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.primaryText
            }

            Text {
                text: "MAVLink Protocol:"
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.secondaryText
            }
            Text {
                text: "v2.0 (Common / ArduPilot / PX4 Dialects)"
                font.family: VoladorTheme.fontMono
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.primaryText
            }

            Text {
                text: "Build Number:"
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.secondaryText
            }
            Text {
                text: "2026.08.06-ALPHA-1"
                font.family: VoladorTheme.fontMono
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.primaryText
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: VoladorTheme.border
        }

        // Open Source Licenses & Acknowledgements Section
        Text {
            text: "Open Source Acknowledgements & License Information"
            font.family: VoladorTheme.fontPrimary
            font.pixelSize: VoladorTheme.fontSizeSubheading
            font.bold: true
            color: VoladorTheme.primaryText
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: licenseText.height
            clip: true

            Text {
                id: licenseText
                width: parent.width
                wrapMode: Text.WordWrap
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeSmall
                color: VoladorTheme.secondaryText
                text: "Volador Ground Control Station is built upon open-source technology standards.\n\n" +
                      "Copyright © 2026 Volador Aerospace. All Rights Reserved.\n" +
                      "Portions Copyright © 2009-2024 QGroundControl Project (GPLv3 / Apache 2.0).\n" +
                      "Portions Copyright © MAVLink Micro Air Vehicle Communication Protocol.\n" +
                      "Qt Framework © The Qt Company Ltd (LGPLv3 / Commercial).\n\n" +
                      "This software is provided 'as is' without warranty of any kind, express or implied."
            }
        }

        // Footer Copyright
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: VoladorTheme.copyright
            font.family: VoladorTheme.fontPrimary
            font.pixelSize: VoladorTheme.fontSizeCaption
            color: VoladorTheme.secondaryText
        }
    }
}
