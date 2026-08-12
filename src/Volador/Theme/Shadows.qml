/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Shadows Tokens
 *
 * Subtle Flat Industrial Elevation Tokens
 *
 ****************************************************************************/

import QtQuick

pragma Singleton

QtObject {
    id: shadows

    readonly property color cardShadowColor: "#0D000000"
    readonly property color panelShadowColor: "#1F000000"
    readonly property real shadowRadius: 4
    readonly property real shadowOffsetY: 2
}
