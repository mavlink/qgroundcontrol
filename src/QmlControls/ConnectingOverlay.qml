/****************************************************************************
 *
 * (c) 2025 IGCS
 *
 * Connecting overlay shown while establishing a link and downloading
 * initial parameters from the vehicle.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root
    anchors.fill: parent
    z: QGroundControl.zOrderTopMost
    visible: _showOverlay

    property var  _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool _showOverlay:    _activeVehicle && !_activeVehicle.initialConnectComplete
    property real _progress01:     _activeVehicle ? _activeVehicle.loadProgress : 0
    property int  _progressPct:    Math.round(_progress01 * 100)
    // Fixed colors to match reference neon theme
    property color _cyan:          "#00F0FF"
    property color _progressFill:  "#00F4D2"
    property color _progressFillHighlight: "#0CFDFD"
    property color _cardBg:        "#0B1014"
    property color _track:         "#06171C"
    property color _textDim:       "#9CB1B6"

    QGCPalette { id: qgcPal }

    // Dim the background
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.85)
    }

    // Centered card
    Rectangle {
        id: card
        width: Math.min(parent.width * 0.5, 480)
        height: contentCol.implicitHeight + ScreenTools.defaultFontPixelWidth * 6
        radius: ScreenTools.defaultFontPixelWidth
        color: _cardBg
        border.width: 1
        border.color: _cyan
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

        // Corner brackets (neon style)
        Rectangle { // top-left
            width: ScreenTools.defaultFontPixelWidth * 3
            height: 1
            color: _cyan
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }
        Rectangle { // left-top
            width: 1
            height: ScreenTools.defaultFontPixelWidth * 3
            color: _cyan
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }
        Rectangle { // top-right
            width: ScreenTools.defaultFontPixelWidth * 3
            height: 1
            color: _cyan
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }
        Rectangle { // right-top
            width: 1
            height: ScreenTools.defaultFontPixelWidth * 3
            color: _cyan
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }
        Rectangle { // bottom-left
            width: ScreenTools.defaultFontPixelWidth * 3
            height: 1
            color: _cyan
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }
        Rectangle { // left-bottom
            width: 1
            height: ScreenTools.defaultFontPixelWidth * 3
            color: _cyan
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }
        Rectangle { // bottom-right
            width: ScreenTools.defaultFontPixelWidth * 3
            height: 1
            color: _cyan
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }
        Rectangle { // right-bottom
            width: 1
            height: ScreenTools.defaultFontPixelWidth * 3
            color: _cyan
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: ScreenTools.defaultFontPixelWidth
        }

        ColumnLayout {
            id: contentCol
            anchors.fill: parent
            anchors.margins: ScreenTools.defaultFontPixelWidth * 3
            spacing: ScreenTools.defaultFontPixelHeight

            // Title
            QGCLabel {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("ESTABLISHING CONNECTION")
                color: _cyan
                font.bold: true
                font.pointSize: ScreenTools.largeFontPointSize
                font.capitalization: Font.AllUppercase
                opacity: 0.95
            }

            QGCLabel {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("VEHICLE LINK")
                color: _textDim
                opacity: 0.9
            }

            // Custom neon progress bar
            Item {
                id: customProgress
                Layout.fillWidth: true
                height: ScreenTools.defaultFontPixelHeight * 0.4

                Rectangle { // track
                    id: track
                    anchors.fill: parent
                    radius: height / 2
                    color: _track
                    border.width: 1
                    border.color: Qt.rgba(0, 240/255, 1, 0.18)
                }
                Rectangle { // fill
                    id: fill
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height
                    width: Math.max(parent.width * (_progressPct / 100), height)
                    radius: height / 2
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: _progressFillHighlight }
                        GradientStop { position: 0.6; color: _progressFill }
                        GradientStop { position: 1.0; color: _progressFill }
                    }
                }
            }

            // Footer row
            RowLayout {
                Layout.fillWidth: true

                QGCLabel {
                    text: qsTr("PROGRESS")
                    color: _textDim
                    Layout.alignment: Qt.AlignLeft
                }

                Item { Layout.fillWidth: true }

                QGCLabel {
                    text: _progressPct + "%"
                    color: _cyan
                    font.bold: true
                    Layout.alignment: Qt.AlignRight
                }
            }

            // Connecting status
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: ScreenTools.defaultFontPixelWidth

                Rectangle {
                    width: ScreenTools.defaultFontPixelHeight * 0.6
                    height: width
                    radius: width / 2
                    color: _progressFillHighlight
                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { from: 0.3; to: 1.0; duration: 600 }
                        NumberAnimation { from: 1.0; to: 0.3; duration: 600 }
                    }
                }

                QGCLabel {
                    text: qsTr("CONNECTING...")
                    color: _progressFillHighlight
                    font.bold: true
                }
            }
        }
    }
}


