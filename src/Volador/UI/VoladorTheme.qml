/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - UI Design Tokens & Theme Manager
 *
 ****************************************************************************/

import QtQuick
import QGroundControl.ScreenTools

pragma Singleton

QtObject {
    id: theme

    // Color Palette Tokens - Industrial Aerospace Theme
    readonly property color bgDark: "#111111"
    readonly property color bgCard: "#1B1F24"
    readonly property color bgCardHover: "#2B2F33"
    readonly property color primaryOrange: "#40464D"
    readonly property color primaryOrangeGlow: "#40464D"
    readonly property color accentCyan: "#40464D"
    readonly property color textPrimary: "#FFFFFF"
    readonly property color textSecondary: "#D1D5DB"
    readonly property color statusGreen: "#2E7D32"
    readonly property color statusRed: "#E53935"
    readonly property color statusYellow: "#6B7280"
    readonly property color borderDark: "#40464D"

    // Responsive Typography
    readonly property int fontTitle: ScreenTools.largeFontPointSize * 1.4
    readonly property int fontHeader: ScreenTools.mediumFontPointSize * 1.2
    readonly property int fontBody: ScreenTools.defaultFontPointSize
    readonly property int fontSmall: ScreenTools.smallFontPointSize

    // Touch Target Standards (Android / Rugged Tablet compliant)
    readonly property real minTouchHeight: 48
    readonly property real defaultPadding: 16
    readonly property real cornerRadius: 6
}
