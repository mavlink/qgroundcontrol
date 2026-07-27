/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

Rectangle {
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
            width:      Math.max(400, parent.width - (_margins * 2))

            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     aboutColumn.height + (_margins * 2)
                color:              qgcPal.windowShade
                radius:             ScreenTools.defaultFontPixelWidth

                ColumnLayout {
                    id:                 aboutColumn
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    anchors.margins:    _margins
                    spacing:            ScreenTools.defaultFontPixelHeight / 2

                    QGCLabel {
                        text:           "Volador Ground Control"
                        font.pointSize: ScreenTools.largeFontPointSize
                        font.bold:      true
                        color:          qgcPal.colorBlue
                    }

                    QGCLabel {
                        text:           qsTr("Developed by Volador Aerospace")
                        font.pointSize: ScreenTools.smallFontPointSize
                        color:          qgcPal.text
                    }

                    QGCLabel {
                        text:           qsTr("Professional UAV Mission Control Platform developed by Volador Aerospace.")
                        font.pointSize: ScreenTools.defaultFontPointSize
                        wrapMode:       Text.WordWrap
                        Layout.fillWidth: true
                    }

                    QGCLabel {
                        text:           qsTr("Version %1").arg(QGroundControl.qgcVersion)
                        font.pointSize: ScreenTools.smallFontPointSize
                        color:          qgcPal.text
                    }
                }
            }

            QGCLabel {
                text:           qsTr("Resources & Documentation")
                font.pointSize: ScreenTools.mediumFontPointSize
                font.bold:      true
            }

            GridLayout {
                id:         grid
                columns:    2
                Layout.fillWidth: true

                QGCLabel { text: qsTr("Official Website") }
                QGCLabel {
                    linkColor:          qgcPal.colorBlue
                    text:               "<a href=\"https://volador.in\">https://volador.in</a>"
                    onLinkActivated:    (link) => Qt.openUrlExternally(link)
                }

                QGCLabel { text: qsTr("Technical Support") }
                QGCLabel {
                    linkColor:          qgcPal.colorBlue
                    text:               "<a href=\"mailto:tech@volador.in\">tech@volador.in</a>"
                    onLinkActivated:    (link) => Qt.openUrlExternally(link)
                }

                QGCLabel { text: qsTr("QGroundControl User Guide") }
                QGCLabel {
                    linkColor:          qgcPal.text
                    text:               "<a href=\"https://docs.qgroundcontrol.com\">https://docs.qgroundcontrol.com</a>"
                    onLinkActivated:    (link) => Qt.openUrlExternally(link)
                }
            }
        }
    }
}
