// Volador Branding
// Phase 1

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Color Palette Base Tokens
 *
 ****************************************************************************/

import QtQuick

pragma Singleton

QtObject {
    id: colors

    // Phase 1 Core Tokens
    readonly property color primaryBackground:   VoladorTheme.primaryBackground
    readonly property color secondaryBackground: VoladorTheme.secondaryBackground
    readonly property color surface:             VoladorTheme.surface
    readonly property color primaryAccent:       VoladorTheme.primaryAccent
    readonly property color accentHover:         VoladorTheme.accentHover
    readonly property color success:             VoladorTheme.success
    readonly property color warning:             VoladorTheme.warning
    readonly property color error:               VoladorTheme.error
    readonly property color primaryText:         VoladorTheme.primaryText
    readonly property color secondaryText:       VoladorTheme.secondaryText
    readonly property color border:              VoladorTheme.border

    // Tech Stark White (Day Mode) Tokens
    readonly property color dayBackground:    "#F7F8FA"
    readonly property color daySurface:       "#FFFFFF"
    readonly property color dayCard:          "#FFFFFF"
    readonly property color dayPanel:         "#F3F5F7"
    readonly property color dayBorder:        "#D9DEE5"
    readonly property color dayTextPrimary:   "#1C1F24"
    readonly property color dayTextSecondary: "#4B5563"

    // Industrial Dark (Night Mode) Tokens
    readonly property color nightBackground:    VoladorTheme.primaryBackground
    readonly property color nightPanels:        VoladorTheme.secondaryBackground
    readonly property color nightCards:         VoladorTheme.surface
    readonly property color nightSidebar:       VoladorTheme.secondaryBackground
    readonly property color nightTopbar:        VoladorTheme.secondaryBackground
    readonly property color nightBorder:        VoladorTheme.border
    readonly property color nightTextPrimary:   VoladorTheme.primaryText
    readonly property color nightTextSecondary: VoladorTheme.secondaryText

    // Common Aerospace Accent Tokens
    readonly property color accentOrange: VoladorTheme.primaryAccent
    readonly property color accentBlue:   VoladorTheme.primaryAccent
    readonly property color dayBlue:      VoladorTheme.primaryAccent
    readonly property color statusGreen:  VoladorTheme.success
    readonly property color dayGreen:     VoladorTheme.success
    readonly property color statusYellow: VoladorTheme.warning
    readonly property color statusRed:    VoladorTheme.error
}
