import QtQml

QtObject {
    id: root

    required property real availableHeight
    required property real availableWidth
    required property real pixelDensity

    property bool mobileLayout: StylePreferences.platformTouchMode

    readonly property real _effectivePixelDensity: Math.max(root.pixelDensity, 1)
    readonly property bool isShort: (root.availableHeight / root._effectivePixelDensity) < StyleMetrics.shortScreenMaximumHeightMillimeters
                                    || (root.mobileLayout && root.availableWidth > 0
                                        && (root.availableHeight / root.availableWidth) < StyleMetrics.shortScreenMaximumAspectRatio)
    readonly property bool isTiny: (root.availableWidth / root._effectivePixelDensity) < StyleMetrics.tinyScreenMaximumWidthMillimeters
    readonly property real minimumTouchTarget: {
        const physicalTouchTarget = Math.round(StyleMetrics.minimumTouchMillimeters * root._effectivePixelDensity)
        return root.availableHeight > 0
                && physicalTouchTarget / root.availableHeight > StyleMetrics.maximumTouchTargetHeightRatio
            ? StyleMetrics.fallbackTouchTargetHeight
            : physicalTouchTarget
    }
}
