/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Industrial Dark Theme (Night Mode)
 *
 ****************************************************************************/

import QtQuick

QtObject {
    id: darkTheme

    readonly property bool isDark: true

    readonly property color background:    "#111111"
    readonly property color surface:       "#1B1F24"
    readonly property color cards:         "#2B2F33"
    readonly property color panel:         "#1B1F24"
    readonly property color sidebar:       "#111111"
    readonly property color topbar:        "#1B1F24"
    readonly property color border:        "#40464D"

    readonly property color textPrimary:   "#FFFFFF"
    readonly property color textSecondary: "#D1D5DB"

    readonly property color accent:        "#40464D"
    readonly property color secondary:     "#2B2F33"
    readonly property color success:       "#2E7D32"
    readonly property color warning:       "#6B7280"
    readonly property color danger:        "#E53935"
}
