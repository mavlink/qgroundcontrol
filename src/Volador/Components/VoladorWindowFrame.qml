/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Enterprise Window Frame Shell
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VoladorTheme 1.0

Rectangle {
    id: windowFrameRoot

    anchors.fill: parent
    color: ThemeController.background
    border.color: ThemeController.border
    border.width: 1
    radius: 10
    clip: true

    default property alias contentData: containerLayout.data

    // Startup Fade-In Animation
    opacity: 0.0
    Component.onCompleted: fadeInAnim.start()

    NumberAnimation {
        id: fadeInAnim
        target: windowFrameRoot
        property: "opacity"
        from: 0.0
        to: 1.0
        duration: 350
        easing.type: Easing.OutCubic
    }

    // Outer Ambient Shadow Glow Simulation
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.color: ThemeController.isDark ? "#1A232E" : "#D0D7E0"
        border.width: 1
        z: 999
    }

    ColumnLayout {
        id: containerLayout
        anchors.fill: parent
        spacing: 0
    }
}
