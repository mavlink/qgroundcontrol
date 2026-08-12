/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Base Industrial Card Container
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Effects
import "qrc:/qml/VoladorTheme"

Rectangle {
    id: cardRoot

    property bool hoverable: false
    property bool isHovered: hoverArea.containsMouse
    property color cardColor: ThemeController.cards
    property color borderColor: isHovered ? ThemeController.accent : ThemeController.border
    property real cornerRadius: 12
    property bool glass: true

    default property alias content: container.children

    color: cardColor
    border.color: borderColor
    border.width: isHovered ? 1.5 : 1
    radius: cornerRadius

    // Subtle Glass / Soft Depth Overlay
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: ThemeController.isDark ? "#10FFFFFF" : "#05000000"
        visible: cardRoot.glass
    }

    Item {
        id: container
        anchors.fill: parent
        anchors.margins: 12
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        enabled: cardRoot.hoverable
        hoverEnabled: cardRoot.hoverable
        onEntered: cardRoot.scale = 1.01
        onExited: cardRoot.scale = 1.0
    }

    Behavior on color { ColorAnimation { duration: 150 } }
    Behavior on border.color { ColorAnimation { duration: 150 } }
    Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.InOutQuad } }
}
