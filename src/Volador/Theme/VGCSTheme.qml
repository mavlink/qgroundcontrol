// Volador Branding
// Phase 1

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Centralized VGCS Theme Engine
 *
 ****************************************************************************/

import QtQuick
import QGroundControl
import QGroundControl.Palette

pragma Singleton

QtObject {
    id: vgcsTheme

    property bool isDarkMode: ThemeController.isDarkMode

    // Light Theme Palette
    readonly property QtObject light: QtObject {
        readonly property color window:          "#F5F7FA"
        readonly property color panels:          "#ECEFF1"
        readonly property color cards:           "#FFFFFF"
        readonly property color border:          "#C7CDD4"

        readonly property color textPrimary:     "#111111"
        readonly property color textSecondary:   "#6B7280"
        readonly property color textDisabled:    "#9CA3AF"

        readonly property color accent:          VoladorTheme.primaryAccent
        readonly property color hoverAccent:     VoladorTheme.accentHover
        readonly property color success:         VoladorTheme.success
        readonly property color warning:         VoladorTheme.warning
        readonly property color danger:          VoladorTheme.error
        readonly property color glassBg:         "#EEFFFFFF"
        readonly property color glassBorder:     "#E5C7CDD4"
    }

    // Dark Theme Palette (Phase 1 Volador Ground Control Station Palette)
    readonly property QtObject dark: QtObject {
        readonly property color window:          VoladorTheme.primaryBackground   // #0A0F14
        readonly property color panels:          VoladorTheme.secondaryBackground // #151C24
        readonly property color cards:           VoladorTheme.surface             // #1D2733
        readonly property color border:          VoladorTheme.border              // #2C3847

        readonly property color textPrimary:     VoladorTheme.primaryText         // #F5F7FA
        readonly property color textSecondary:   VoladorTheme.secondaryText       // #9BA8B5
        readonly property color textDisabled:    "#6B7280"

        readonly property color accent:          VoladorTheme.primaryAccent       // #FF6A00
        readonly property color hoverAccent:     VoladorTheme.accentHover         // #FF8533
        readonly property color success:         VoladorTheme.success             // #00C853
        readonly property color warning:         VoladorTheme.warning             // #FFC107
        readonly property color danger:          VoladorTheme.error               // #F44336
        readonly property color glassBg:         "#CC151C24"
        readonly property color glassBorder:     "#402C3847"
    }

    // Dynamic Accessors
    readonly property color window:         isDarkMode ? dark.window : light.window
    readonly property color panels:         isDarkMode ? dark.panels : light.panels
    readonly property color cards:          isDarkMode ? dark.cards : light.cards
    readonly property color border:         isDarkMode ? dark.border : light.border

    readonly property color textPrimary:    isDarkMode ? dark.textPrimary : light.textPrimary
    readonly property color textSecondary:  isDarkMode ? dark.textSecondary : light.textSecondary
    readonly property color textDisabled:   isDarkMode ? dark.textDisabled : light.textDisabled

    readonly property color accent:         isDarkMode ? dark.accent : light.accent
    readonly property color hoverAccent:    isDarkMode ? dark.hoverAccent : light.hoverAccent
    readonly property color success:        isDarkMode ? dark.success : light.success
    readonly property color warning:        isDarkMode ? dark.warning : light.warning
    readonly property color danger:         isDarkMode ? dark.danger : light.danger

    readonly property color glassBg:        isDarkMode ? dark.glassBg : light.glassBg
    readonly property color glassBorder:    isDarkMode ? dark.glassBorder : light.glassBorder

    // Typography Tokens
    readonly property string fontPrimary:   VoladorTheme.fontPrimary
    readonly property string fontFallback:  VoladorTheme.fontFallback
    readonly property string fontMono:      VoladorTheme.fontMono

    readonly property int fontSizeHeading: VoladorTheme.fontSizeHeading
    readonly property int fontSizeSection: VoladorTheme.fontSizeSubheading
    readonly property int fontSizeBody:    VoladorTheme.fontSizeBody
    readonly property int fontSizeButton:  VoladorTheme.fontSizeBody

    function toggle() {
        ThemeController.toggleTheme()
    }
}
