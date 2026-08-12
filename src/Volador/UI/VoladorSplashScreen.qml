// Volador Branding
// Phase 1 & Phase 2 Official Identity Integration

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Official Splash Screen
 *
 * Displays master Volador branding logo centered on dark background with
 * progress indicator and status updates.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Volador.Theme 1.0

Rectangle {
    id: splashRoot
    width: 640
    height: 420
    color: VoladorTheme.primaryBackground

    property alias progressText: progressLabel.text
    property alias statusText: statusLabel.text
    property alias progressValue: progressBar.value

    // Subtle Brand Border
    border.color: VoladorTheme.border
    border.width: 1

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 28
        width: parent.width * 0.85

        // Centered Official Volador Logo Container
        Item {
            id: logoContainer
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 260
            implicitHeight: 140

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 12

                Image {
                    id: logoImage
                    source: "qrc:/Volador/Assets/Logos/volador_primary.png"
                    Layout.alignment: Qt.AlignHCenter
                    implicitWidth: 320
                    implicitHeight: 100
                    fillMode: Image.PreserveAspectFit
                    antialiasing: true
                    mipmap: true
                }

                Text {
                    text: VoladorTheme.shortName
                    font.family: VoladorTheme.fontPrimary
                    font.pixelSize: 18
                    font.bold: true
                    color: VoladorTheme.primaryText
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: VoladorTheme.productDescription
                    font.family: VoladorTheme.fontPrimary
                    font.pixelSize: 12
                    color: VoladorTheme.secondaryText
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // Progress Indicator
        ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            from: 0
            to: 100
            value: 45

            background: Rectangle {
                implicitHeight: 6
                color: VoladorTheme.surface
                radius: 3
            }

            contentItem: Item {
                implicitHeight: 6

                Rectangle {
                    width: progressBar.visualPosition * parent.width
                    height: parent.height
                    radius: 3
                    color: VoladorTheme.primaryAccent
                }
            }
        }

        // Status & Progress Information
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 6

            Text {
                id: statusLabel
                text: "Initializing VGCS Core Engine..."
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeBody
                color: VoladorTheme.primaryText
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                id: progressLabel
                text: "Loading MAVLink Stack & Vehicle Plugins (45%)"
                font.family: VoladorTheme.fontPrimary
                font.pixelSize: VoladorTheme.fontSizeSmall
                color: VoladorTheme.secondaryText
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }

    // Version Footer
    Text {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 16
        text: "v" + VoladorTheme.version
        font.family: VoladorTheme.fontMono
        font.pixelSize: VoladorTheme.fontSizeCaption
        color: VoladorTheme.secondaryText
    }
}
