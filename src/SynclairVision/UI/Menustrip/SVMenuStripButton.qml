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
    property bool expanded: false

    property real size: ScreenTools.defaultFontPixelWidth * 7
    property real borderRadius: ScreenTools.defaultBorderRadius
    property real iconScale: root.text === "" ? 0.6 : 0.45
    property real spacing: ScreenTools.defaultFontPixelHeight * 0.2

    readonly property color foregroundColor: root.checked ? qgcPalette.buttonHighlightText : qgcPalette.statusFailedText
    readonly property color backgroundColor: (mouseArea.pressed || root.checked)
                                             ? qgcPalette.buttonHighlight
                                             : (mouseArea.containsMouse ? qgcPalette.toolStripHoverColor : "transparent")
    readonly property real frameInset: ScreenTools.defaultFontPixelWidth * 0.25

    signal clicked

    width: size
    height: size

    QGCPalette { id: qgcPalette; colorGroupEnabled: root.enabled }

    Rectangle {
        id: background
        anchors.fill: parent
        radius: root.borderRadius
        color: root.backgroundColor
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: root.frameInset / 2
        color: "transparent"
        border.width: 1
        border.color: qgcPalette.windowShade
        visible: root.checked
        radius: root.borderRadius
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        onClicked: root.clicked()
    }

    SVArrow {
        id: arrow
        visible: root.extendHeader
        width: ScreenTools.defaultFontPixelWidth * 0.6
        height: width * 1.3
        anchors.top: parent.top
        anchors.topMargin: root.height / 2 - root.spacing * 2.5
        anchors.right: parent.right
        anchors.rightMargin: root.spacing
        arrowFilled: true
        outerBorderColor: root.foregroundColor

        rotation: root.expanded ? 90 : 270

        Behavior on rotation {
            NumberAnimation { duration: 60 }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: root.spacing / 2

        QGCColoredImage {
            id: iconImage
            width: root.size * root.iconScale
            height: width
            source: root.iconSource
            color: root.foregroundColor
            anchors.horizontalCenter: parent.horizontalCenter
            visible: source !== ""
        }

        QGCLabel {
            text: root.text
            font.pointSize: ScreenTools.smallFontPointSize
            color: root.foregroundColor
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
