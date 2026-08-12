/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Layout & Spacing Metrics
 *
 * Touch & Industrial Ergonomics
 *
 ****************************************************************************/

import QtQuick

pragma Singleton

QtObject {
    id: metrics

    // Touch Target Standards (Android / Industrial Tablet compliant)
    readonly property real minTouchHeight: 48
    readonly property real minTouchWidth:  48

    // Spacing Grid
    readonly property real spacingXs: 4
    readonly property real spacingSm: 8
    readonly property real spacingMd: 12
    readonly property real spacingLg: 16
    readonly property real spacingXl: 24
    readonly property real spacingXxl: 32

    // Corner Radii
    readonly property real radiusSm: 4
    readonly property real radiusMd: 6
    readonly property real radiusLg: 8
    readonly property real radiusXl: 12

    // Component Dimensions
    readonly property real sidebarWidth: 240
    readonly property real sidebarRailWidth: 64
    readonly property real headerHeight: 56
    readonly property real cardPadding: 16
}
