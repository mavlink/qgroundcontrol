/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Centralized Theme Controller
 *
 ****************************************************************************/

import QtQuick
import QGroundControl
import QGroundControl.Palette
import "qrc:/qml/VoladorTheme"

pragma Singleton

QtObject {
    id: controller

    // Active Theme State (Default: Industrial Dark / Night Mode)
    property bool isDarkMode: true

    onIsDarkModeChanged: {
        if (typeof QGroundControl !== "undefined" && QGroundControl.globalPalette) {
            QGroundControl.globalPalette.globalTheme = isDarkMode ? QGCPalette.Dark : QGCPalette.Light
        }
    }

    // Theme Sub-objects
    readonly property LightTheme _lightTheme: LightTheme {}
    readonly property DarkTheme  _darkTheme:  DarkTheme {}

    // Active Theme Pointer
    readonly property var activeTheme: isDarkMode ? _darkTheme : _lightTheme

    // Convenience Shortcuts for Components
    readonly property bool  isDark:        activeTheme.isDark
    readonly property color background:    activeTheme.background
    readonly property color surface:       activeTheme.surface
    readonly property color cards:         activeTheme.cards
    readonly property color panel:         activeTheme.panel
    readonly property color sidebar:       activeTheme.sidebar
    readonly property color topbar:        activeTheme.topbar
    readonly property color border:        activeTheme.border

    readonly property color textPrimary:   activeTheme.textPrimary
    readonly property color textSecondary: activeTheme.textSecondary

    readonly property color accent:        activeTheme.accent
    readonly property color secondary:     activeTheme.secondary
    readonly property color success:       activeTheme.success
    readonly property color warning:       activeTheme.warning
    readonly property color danger:        activeTheme.danger

    function toggleTheme() {
        isDarkMode = !isDarkMode
    }

    function setDarkMode(enableDark) {
        isDarkMode = enableDark
    }
}
