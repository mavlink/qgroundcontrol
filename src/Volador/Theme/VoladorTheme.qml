// Volador Branding
// Phase 1

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Reusable Theme Module
 *
 * Centralized design system color tokens, typography and brand definitions.
 *
 ****************************************************************************/

import QtQuick

pragma Singleton

QtObject {
    id: voladorTheme

    // Application Branding Metadata
    readonly property string brandName:          "Volador Ground Control Station"
    readonly property string shortName:          "VGCS"
    readonly property string company:            "Volador Aerospace"
    readonly property string productDescription: "Enterprise Drone Mission Control Platform"
    readonly property string website:            "https://volador.in"
    readonly property string copyright:          "© 2026 Volador Aerospace. All Rights Reserved."
    readonly property string version:            "2.0.0-alpha.1"

    // Phase 1 Centralized Theme Colors
    readonly property color primaryBackground:   "#0A0F14"
    readonly property color secondaryBackground: "#151C24"
    readonly property color surface:             "#1D2733"
    readonly property color primaryAccent:       "#FF6A00"
    readonly property color accentHover:         "#FF8533"
    readonly property color success:             "#00C853"
    readonly property color warning:             "#FFC107"
    readonly property color error:               "#F44336"
    readonly property color primaryText:         "#F5F7FA"
    readonly property color secondaryText:       "#9BA8B5"
    readonly property color border:              "#2C3847"

    // Typography Architecture
    readonly property string fontPrimary:        "Inter"
    readonly property string fontMono:           "JetBrains Mono"
    readonly property string fontFallback:       "Segoe UI"
    readonly property string fontMonoFallback:   "Consolas"

    readonly property int fontSizeHeading:       28
    readonly property int fontSizeSubheading:    20
    readonly property int fontSizeBody:          14
    readonly property int fontSizeSmall:         12
    readonly property int fontSizeCaption:       10

    // Convenience Getters
    readonly property string fontFamily:        fontPrimary + ", " + fontFallback + ", sans-serif"
    readonly property string fontFamilyMono:    fontMono + ", " + fontMonoFallback + ", monospace"
}
