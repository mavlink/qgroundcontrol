import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Item {
    id: control

    objectName: "flyViewToolBarIndicators"

    implicitWidth: mainLayout.implicitWidth + (_margin * 2)

    property real _margin:  ScreenTools.defaultFontPixelHeight * 0.55
    property real _dotSize: ScreenTools.defaultFontPixelHeight * 0.45

    QGCPalette {
        id: qgcPal
    }

    // ---------------------------------------------------------------------
    // Menu comum aos quatro indicadores
    // ---------------------------------------------------------------------

    Menu {
        id: statusMenu

        property string statusTitle: ""
        property string statusText:  ""
        property string extraText:   ""

        MenuItem {
            text: statusMenu.statusTitle
            font.bold: true
            enabled: false
        }

        MenuSeparator {}

        MenuItem {
            text: statusMenu.statusText
            enabled: false
        }

        MenuItem {
            text: statusMenu.extraText
            enabled: false
            visible: statusMenu.extraText.length > 0
        }
    }


    Row {
        id: mainLayout

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: _margin

        spacing: ScreenTools.defaultFontPixelWidth * 2


        // ================================================================
        // JETSON
        //
        // Estado REAL da ligação TCP AtlasJetsonClient.
        // ================================================================

        Item {
            width: jetsonRow.implicitWidth
            height: parent.height

            Row {
                id: jetsonRow

                anchors.centerIn: parent

                spacing: ScreenTools.defaultFontPixelWidth * 0.6

                Rectangle {
                    width: _dotSize
                    height: _dotSize
                    radius: width / 2

                    anchors.verticalCenter: parent.verticalCenter

                    color:
                        atlasJetsonClient.connected
                        ? "#39D353"
                        : "#808080"
                }

                QGCLabel {
                    text: "JETSON"
                    font.bold: true

                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                anchors.fill: parent

                cursorShape: Qt.PointingHandCursor

                onClicked: {
                    statusMenu.statusTitle = "JETSON"

                    statusMenu.statusText =
                        atlasJetsonClient.connected
                        ? "Status: Connected"
                        : "Status: Not connected"

                    statusMenu.extraText =
                        atlasJetsonClient.connected
                        ? "ATLAS service reachable via TCP"
                        : "AI / video processing / SIYI control"

                    statusMenu.popup()
                }
            }
        }


        // ================================================================
        // HM30
        //
        // Ainda MOCK.
        // ================================================================

        Item {
            width: hm30Row.implicitWidth
            height: parent.height

            Row {
                id: hm30Row

                anchors.centerIn: parent

                spacing: ScreenTools.defaultFontPixelWidth * 0.6

                Rectangle {
                    width: _dotSize
                    height: _dotSize
                    radius: width / 2

                    color: "#808080"

                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCLabel {
                    text: "HM30"
                    font.bold: true

                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                anchors.fill: parent

                cursorShape: Qt.PointingHandCursor

                onClicked: {
                    statusMenu.statusTitle = "HM30"
                    statusMenu.statusText = "Status: Not connected"
                    statusMenu.extraText = "Air-ground data link"

                    statusMenu.popup()
                }
            }
        }


        // ================================================================
        // A8 MINI
        //
        // Ainda MOCK.
        // ================================================================

        Item {
            width: a8Row.implicitWidth
            height: parent.height

            Row {
                id: a8Row

                anchors.centerIn: parent

                spacing: ScreenTools.defaultFontPixelWidth * 0.6

                Rectangle {
                    width: _dotSize
                    height: _dotSize
                    radius: width / 2

                    color: "#808080"

                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCLabel {
                    text: "A8"
                    font.bold: true

                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                anchors.fill: parent

                cursorShape: Qt.PointingHandCursor

                onClicked: {
                    statusMenu.statusTitle = "SIYI A8 MINI"
                    statusMenu.statusText = "Status: Not connected"
                    statusMenu.extraText =
                        "Gimbal / zoom / recording control"

                    statusMenu.popup()
                }
            }
        }


        // ================================================================
        // VIDEO
        //
        // Ainda MOCK.
        // ================================================================

        Item {
            width: videoRow.implicitWidth
            height: parent.height

            Row {
                id: videoRow

                anchors.centerIn: parent

                spacing: ScreenTools.defaultFontPixelWidth * 0.6

                Rectangle {
                    width: _dotSize
                    height: _dotSize
                    radius: width / 2

                    color: "#808080"

                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCLabel {
                    text: "VIDEO"
                    font.bold: true

                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                anchors.fill: parent

                cursorShape: Qt.PointingHandCursor

                onClicked: {
                    statusMenu.statusTitle = "VIDEO"
                    statusMenu.statusText =
                        "Status: Waiting for stream"

                    statusMenu.extraText =
                        "Processed RTSP stream from Jetson"

                    statusMenu.popup()
                }
            }
        }
    }
}
