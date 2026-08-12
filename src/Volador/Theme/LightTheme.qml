/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Tech Stark White Theme (Day Mode)
 *
 ****************************************************************************/

import QtQuick

QtObject {
    id: lightTheme

    readonly property bool isDark: false

    readonly property color background:    "#F5F7FA"
    readonly property color surface:       "#FFFFFF"
    readonly property color cards:         "#FFFFFF"
    readonly property color panel:         "#ECEFF1"
    readonly property color sidebar:       "#FFFFFF"
    readonly property color topbar:        "#FFFFFF"
    readonly property color border:        "#C7CDD4"

    readonly property color textPrimary:   "#111111"
    readonly property color textSecondary: "#6B7280"

    readonly property color accent:        "#40464D"
    readonly property color secondary:     "#2B2F33"
    readonly property color success:       "#2E7D32"
    readonly property color warning:       "#6B7280"
    readonly property color danger:        "#E53935"
}
