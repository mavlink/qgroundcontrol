pragma Singleton

import QtQuick

QtObject {
    id: root

    readonly property int animationDuration: normalAnimationDuration
    readonly property real baseToolbarHeight: StyleTypography.bodyPixelHeight * 3
    readonly property int borderWidth: 1
    readonly property int busyIndicatorSize: Math.round((StylePreferences.touchMode ? 48 : 40) * StyleTypography.scaleFactor)
    readonly property real checkBoxIndicatorSize: 2 * Math.floor(StyleTypography.bodyPixelHeight / 2) + 1
    readonly property real comboBoxPadding: StyleTypography.bodyPixelWidth
    readonly property int contentMargin: Math.max(1, Math.round((StylePreferences.touchMode ? 16 : 12) * StyleTypography.scaleFactor))
    readonly property int controlContentHeight: Math.ceil(StyleTypography.bodyFontHeight)
    readonly property int controlHeight: Math.max(minimumInteractiveHeight, controlContentHeight + controlVerticalPadding * 2)
    readonly property int controlHorizontalPadding: contentMargin
    readonly property int controlVerticalPadding: Math.max(1, Math.round(6 * StyleTypography.scaleFactor))
    readonly property int controlWidth: Math.round((StylePreferences.touchMode ? 120 : 100) * StyleTypography.scaleFactor)
    readonly property real defaultBorderRadius: StyleTypography.bodyPixelWidth / 2
    readonly property real defaultDialogControlSpacing: Math.max(1, Math.round(StyleTypography.bodyLineSpacing / 2))
    readonly property int dialogPadding: contentMargin
    readonly property real fallbackTouchTargetHeight: StyleTypography.bodyPixelHeight * 3
    readonly property int focusBorderWidth: 2
    readonly property int fastAnimationDuration: StylePreferences.animationsEnabled ? 100 : 0
    readonly property real hugeScreenMinimumWidthMillimeters: 23.5 * 25.4
    readonly property int iconSize: toolbarIconSize
    readonly property real implicitButtonHeight: controlHeight
    readonly property real implicitButtonWidth: Math.round(StyleTypography.bodyPixelWidth * 5)
    readonly property real implicitCheckBoxHeight: Math.round(StyleTypography.bodyPixelHeight)
    readonly property real implicitComboBoxHeight: implicitButtonHeight
    readonly property real implicitComboBoxWidth: implicitButtonWidth
    readonly property real implicitRadioButtonHeight: implicitCheckBoxHeight
    readonly property real implicitSliderHeight: StyleTypography.bodyPixelHeight
    readonly property real implicitTextFieldHeight: implicitButtonHeight
    readonly property real implicitTextFieldWidth: StyleTypography.bodyPixelWidth * 10
    readonly property int indicatorSize: Math.round((StylePreferences.touchMode ? 32 : 28) * StyleTypography.scaleFactor)
    readonly property int inlineIconSize: Math.max(1, Math.ceil(StyleTypography.bodyCapitalHeight))
    readonly property int largeIconSize: Math.round(32 * StyleTypography.scaleFactor)
    readonly property real maximumTouchTargetHeightRatio: 0.15
    readonly property int menuWidth: Math.round(200 * StyleTypography.scaleFactor)
    readonly property int minimumInteractiveHeight: Math.round((StylePreferences.touchMode ? 48 : 40) * StyleTypography.scaleFactor)
    readonly property real minimumTouchMillimeters: 5
    readonly property int nominalTouchTarget: Math.round(48 * StyleTypography.scaleFactor)
    readonly property int normalAnimationDuration: StylePreferences.animationsEnabled ? 150 : 0
    readonly property int padding: controlVerticalPadding
    readonly property int panelRadius: Math.max(1, Math.round(6 * StyleTypography.scaleFactor))
    readonly property int progressBarHeight: Math.max(1, Math.round(6 * StyleTypography.scaleFactor))
    readonly property real radioButtonIndicatorSize: checkBoxIndicatorSize
    readonly property int radius: Math.max(1, Math.round(4 * StyleTypography.scaleFactor))
    readonly property int scrollBarExtent: Math.max(1, Math.round((StylePreferences.touchMode ? 8 : 6) * StyleTypography.scaleFactor))
    readonly property int separatorHeight: Math.max(1, Math.round(StyleTypography.scaleFactor))
    readonly property real shortScreenMaximumAspectRatio: 0.6
    readonly property real shortScreenMaximumHeightMillimeters: 120
    readonly property int smallIconSize: Math.round(16 * StyleTypography.scaleFactor)
    readonly property real smallSpacing: spacing / 2
    readonly property int slowAnimationDuration: StylePreferences.animationsEnabled ? 500 : 0
    readonly property int spacing: Math.max(1, Math.round(6 * StyleTypography.scaleFactor))
    readonly property int switchHeight: Math.round((StylePreferences.touchMode ? 32 : 28) * StyleTypography.scaleFactor)
    readonly property int switchWidth: Math.round((StylePreferences.touchMode ? 64 : 56) * StyleTypography.scaleFactor)
    readonly property real toolbarHeight: root.baseToolbarHeight * root.toolbarHeightMultiplier
    property real toolbarHeightMultiplier: 1
    readonly property int toolbarIconSize: Math.round(24 * StyleTypography.scaleFactor)
    readonly property real tinyScreenMaximumWidthMillimeters: 120
}
