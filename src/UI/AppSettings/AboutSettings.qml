import QtQuick

import QGroundControl
import QGroundControl.Controls

Rectangle {
    color:          qgcPal.toolbarBackground
    anchors.fill:   parent

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Item {
        anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.75
        anchors.fill:       parent

        ColumnLayout {
            id:         column
            spacing:    ScreenTools.defaultFontPixelHeight
            width:      Math.min(parent.width, ScreenTools.defaultFontPixelWidth * 120)
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.fillWidth: true

            QGCLabel {
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.bold:          true
                font.pointSize:     ScreenTools.largeFontPointSize
                text:               qsTr("Welcome to the IG Drones Ground Control Station!")
            }

            QGCLabel {
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                text:               qsTr("Our GCS is designed to provide you with a robust, user-friendly interface to command and control our advanced UAV platforms.")
            }

            QGCLabel {
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                text:               qsTr("We are committed to empowering operators with precise control, secure communication, and seamless multi-drone coordination to ensure mission success in defense applications. For detailed instructions, updates, and support, please visit our official website or contact our technical support team.")
            }

            GridLayout {
                Layout.fillWidth:   true
                columns:            2
                columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.5

                QGCLabel { text: qsTr("Official Website:") }
                QGCLabel {
                    linkColor:      qgcPal.text
                    horizontalAlignment: Text.AlignLeft
                    text:           "<a href=\"https://igdrones.com\">https://igdrones.com</a>"
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                }

                QGCLabel { text: qsTr("Support Contact:") }
                QGCLabel {
                    linkColor:      qgcPal.text
                    horizontalAlignment: Text.AlignLeft
                    text:           "<a href=\"https://igdrones.com/contact\">https://igdrones.com/contact</a>"
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                }
            }

            QGCLabel {
                Layout.fillWidth:   true
                horizontalAlignment: Text.AlignLeft
                wrapMode:           Text.WordWrap
                text:               qsTr("Thank you for choosing IG GCS FLY — precision in every flight, security in every mission.")
            }
        }
    }
}