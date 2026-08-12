/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Typography Tokens
 *
 * Direct Sunlight Readable Font Hierarchy
 *
 ****************************************************************************/

import QtQuick
import QGroundControl.ScreenTools

pragma Singleton

QtObject {
    id: typography

    readonly property string fontFamily: "Inter, Roboto, sans-serif"

    // Font Sizes (pt)
    readonly property real bodySize:      16
    readonly property real telemetrySize: 18
    readonly property real titleSize:     24
    readonly property real headingSize:   32
    readonly property real buttonSize:    16
    readonly property real captionSize:   13

    // Font Weights
    readonly property int weightMedium:   Font.Medium
    readonly property int weightSemiBold: Font.DemiBold
    readonly property int weightBold:     Font.Bold
}
