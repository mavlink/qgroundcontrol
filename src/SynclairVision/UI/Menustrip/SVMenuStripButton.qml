import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property string text: ""
    property string description: ""
    property url iconSource
    property bool tintIcon: true
    property bool checked: false
    property bool enabled: true
    property bool extendHeader: false
    property bool expanded: false

    property bool tooltipVisible: false
    property bool isVertical: true
    property bool isLeft: true
    property bool isTop: true

    property real size: ScreenTools.defaultFontPixelWidth * 7
    property real borderRadius: ScreenTools.defaultBorderRadius
    property real iconScale: root.text === "" ? 0.6 : 0.45
    property real spacing: ScreenTools.defaultFontPixelHeight * 0.2
    property real contentSize: size - SVUnits.bigMargin * 2

    readonly property color foregroundColor: root.checked ? qgcPalette.buttonHighlightText : qgcPalette.statusFailedText

    signal clicked

    width: size
    height: size

    QGCPalette { id: qgcPalette; colorGroupEnabled: root.enabled }

    function stopTimer() {
        hoverTimer.stop()
        tooltipVisible = false
    }

    SVBackground {
        anchors.fill: parent
        anchors.margins: root.enabled ? 0 : SVUnits.lineWidth * 2
        
        transparentBackground: true
        enabled: root.enabled
        hoverEnabled: true
        hovered: mouseArea.containsMouse && root.enabled
        checkable: true
        checked: root.checked// && root.enabled
        pressed: mouseArea.pressed && root.enabled
        hoverPosition: Qt.point(mouseArea.mouseX, mouseArea.mouseY)
        radius: root.borderRadius
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true

        onPressed: {
            stopTimer()
        }

        onClicked: {
            stopTimer()
            root.clicked()
        }
    }

    HoverHandler {
        onHoveredChanged: {
            if (hovered) {
                hoverTimer.start()
            } else {
                stopTimer()
            }
        }
    }

    Timer {
        id: hoverTimer
        interval: 300
        repeat: false
        onTriggered: {
            tooltipVisible = true
        }
    }

    SVArrow {
        id: arrow
        visible: root.extendHeader
        width: ScreenTools.defaultFontPixelWidth * 0.6
        height: width * 1.3
        anchors.top: parent.top
        anchors.topMargin: root.height / 2 - root.spacing * 2.5
        anchors.right: parent.right
        anchors.rightMargin: root.spacing * 2
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

        Item {
            width: root.contentSize * root.iconScale
            height: width
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.iconSource !== ""

            QGCColoredImage {
                anchors.fill: parent
                source: root.iconSource
                color: root.foregroundColor
                visible: root.tintIcon
            }

            Image {
                anchors.fill: parent
                source: root.iconSource
                smooth: true
                mipmap: true
                antialiasing: true
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                sourceSize.height: height
                visible: !root.tintIcon
            }
        }

        QGCLabel {
            text: root.text
            font.pointSize: ScreenTools.smallFontPointSize
            color: root.foregroundColor
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    SVTooltip {
        isVertical: root.isVertical
        isLeft: root.isLeft
        isTop: root.isTop
        text: root.description
        enabled: true
    }
}
