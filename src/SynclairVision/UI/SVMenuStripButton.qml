import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property string text: ""
    property url iconSource
    property bool checked: false
    property bool enabled: true
    property bool extendHeader: false
    property bool expanded: false        // ← add this line

    property real size: ScreenTools.defaultFontPixelWidth * 7
    property real borderRadius: ScreenTools.defaultBorderRadius
    property real iconScale: (text === "") ? 0.6 : 0.45
    property real spacing: ScreenTools.defaultFontPixelHeight * 0.20

    signal clicked

    width: size
    height: size

    QGCPalette { id: qgcPalette; colorGroupEnabled: root.enabled }

    Rectangle {
        id: background
        anchors.fill: parent
        radius: root.borderRadius
        color: {
            if(mouseArea.pressed) {
                return qgcPalette.buttonHighlight
            } else if(root.checked) {
                return qgcPalette.buttonHighlight
            } else if(mouseArea.containsMouse) {
                return qgcPalette.toolStripHoverColor
            } else {
                return "transparent"
            }
        }
    }

    Rectangle {
        width: parent.width - ScreenTools.defaultFontPixelWidth * 0.25
        height: parent.height - ScreenTools.defaultFontPixelWidth * 0.25
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        color: "transparent"
        border.width: 1
        border.color: qgcPalette.windowShade
        visible: root.checked
        radius: ScreenTools.defaultBorderRadius
    }

    

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        onClicked: root.clicked()
    }

    Text {
        id: chevron
        visible: root.extendHeader
        text: "\u25BE"   // ▾
        font.pointSize: ScreenTools.smallFontPointSize
        color: root.checked ? qgcPalette.buttonHighlightText : qgcPalette.statusFailedText
        anchors.right: parent.right
        anchors.rightMargin: root.spacing
        anchors.top: parent.top
        anchors.topMargin: root.size * root.iconScale / 2
        rotation: root.expanded ? 180 : 0

        Behavior on rotation {
            NumberAnimation { duration: 60 }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: root.spacing / 2
        anchors.verticalCenter: parent.verticalCenter

        QGCColoredImage {
            id: iconImage
            width: root.size * root.iconScale
            height: width
            source: root.iconSource
            color: root.checked ? qgcPalette.buttonHighlightText : qgcPalette.statusFailedText
            anchors.horizontalCenter: parent.horizontalCenter
            visible: source !== ""
        }

        QGCLabel {
            text: root.text
            font.pointSize: ScreenTools.smallFontPointSize
            color: root.checked ? qgcPalette.buttonHighlightText : qgcPalette.statusFailedText
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}