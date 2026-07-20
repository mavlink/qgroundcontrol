pragma Singleton

import QtQuick

import QGroundControl
import QGCStyle as QGCStyle

/*!
 ScreenTools provides QGC-specific platform and display information. Its legacy font and control metrics remain as
 compatibility aliases. New visual components should use QGCStyle.StyleTypography and QGCStyle.StyleMetrics directly.

 Usage:

        import QtQuick
        import QGCStyle

        Rectangle {
            anchors.fill: parent
            anchors.margins: StyleMetrics.contentMargin
            ...
        }
*/
QtObject {
    id: root

    readonly property real defaultFontPointSize: QGCStyle.StyleTypography.bodyPointSize
    readonly property real platformFontPointSize: {
        const applicationPointSize = Qt.application.font.pointSize > 0 ? Qt.application.font.pointSize : 10
        if (!root.isMobile) {
            return applicationPointSize
        }
        if (root.isiOS) {
            return root.screenWidth < 570 ? 12 : 14
        }
        return (root.screenWidth / root.realPixelDensity) < QGCStyle.StyleMetrics.tinyScreenMaximumWidthMillimeters ? 11 : 14
    }

    readonly property real smallFontPointRatio:      QGCStyle.StyleTypography.captionScale
    readonly property real mediumFontPointRatio:     QGCStyle.StyleTypography.headingScale
    readonly property real largeFontPointRatio:      QGCStyle.StyleTypography.titleScale

    /// You can use these properties to position ui elements in a screen resolution independent manner. Using fixed positioning values should not
    /// be done. All positioning should be done using anchors or a ratio of the defaultFontPixelHeight and defaultFontPixelWidth values. This way
    /// your ui elements will reposition themselves appropriately on varying screen sizes and resolutions.
    readonly property real defaultFontPixelHeight: QGCStyle.StyleTypography.bodyPixelHeight
    readonly property real largeFontPixelHeight:     QGCStyle.StyleTypography.titlePixelHeight

    /// You can use these properties to position ui elements in a screen resolution independent manner. Using fixed positioning values should not
    /// be done. All positioning should be done using anchors or a ratio of the defaultFontPixelHeight and defaultFontPixelWidth values. This way
    /// your ui elements will reposition themselves appropriately on varying screen sizes and resolutions.
    readonly property real defaultFontPixelWidth: QGCStyle.StyleTypography.bodyPixelWidth
    readonly property real largeFontPixelWidth:      QGCStyle.StyleTypography.titlePixelWidth

    /// QFontMetrics::descent for default font at default point size
    readonly property real defaultFontDescent: QGCStyle.StyleTypography.bodyDescent

    /// The default amount of space in between controls in a dialog
    readonly property real defaultDialogControlSpacing: QGCStyle.StyleMetrics.defaultDialogControlSpacing

    readonly property real smallFontPointSize:       QGCStyle.StyleTypography.captionPointSize
    readonly property real mediumFontPointSize:      QGCStyle.StyleTypography.headingPointSize
    readonly property real largeFontPointSize:       QGCStyle.StyleTypography.titlePointSize

    readonly property real toolbarHeight: QGCStyle.StyleMetrics.toolbarHeight

    property var _windowScreen: null
    readonly property var _screen: root._windowScreen ?? (Qt.application.screens.length > 0 ? Qt.application.screens[0] : null)
    readonly property real _screenPixelDensity: {
        const density = root._screen ? root._screen.pixelDensity : 0
        return Number.isFinite(density) && density > 0 ? density : 1
    }
    readonly property real realPixelDensity: {
        //-- If a plugin defines a valid override, use what it tells us
        const configuredPixelDensity = QGroundControl.corePlugin.options.devicePixelDensity
        if (Number.isFinite(configuredPixelDensity) && configuredPixelDensity > 0) {
            return configuredPixelDensity
        }
        //-- Android is rather unreliable
        if (root.isAndroid) {
            // Lets assume it's unlikely you have a tablet over 300mm wide
            if ((root.screenWidth / root._screenPixelDensity) > 300) {
                return root._screenPixelDensity * 2
            }
        }
        //-- Let's use what the system tells us
        return root._screenPixelDensity
    }

    // These properties allow us to create simulated mobile sizing for a desktop build.
    // This makes testing the UI for smaller mobile sizing much easier.
    // The 731x411 size is the size of the Herelink screen which is our target lower bound
    readonly property real screenWidth:  root.isFakeMobile ? 731 : (root._screen ? root._screen.width : 0)
    readonly property real screenHeight: root.isFakeMobile ? 411 : (root._screen ? root._screen.height : 0)

    readonly property QGCStyle.LayoutProfile _layoutProfile: QGCStyle.LayoutProfile {
        availableHeight: root.screenHeight
        availableWidth: root.screenWidth
        mobileLayout: root.isMobile
        pixelDensity: root.realPixelDensity
    }

    readonly property string _platformOs:           Qt.platform.os
    readonly property bool isAndroid:               root._platformOs === "android"
    readonly property bool isiOS:                   root._platformOs === "ios"
    readonly property bool isFakeMobile:            !root.isAndroid && !root.isiOS
                                                    && (Qt.application.arguments.includes("--fake-mobile")
                                                        || Qt.application.arguments.includes("-fake-mobile"))
    readonly property bool isMobile:                root.isAndroid || root.isiOS || root.isFakeMobile
    readonly property bool isTinyScreen:            root._layoutProfile.isTiny
    readonly property bool isShortScreen:           root._layoutProfile.isShort

    readonly property real minTouchMillimeters:     QGCStyle.StyleMetrics.minimumTouchMillimeters
    readonly property real minTouchPixels:          root._layoutProfile.minimumTouchTarget

    // The implicit heights/widths for our custom control set
    readonly property real implicitButtonWidth:              QGCStyle.StyleMetrics.implicitButtonWidth
    readonly property real implicitButtonHeight:             QGCStyle.StyleMetrics.implicitButtonHeight
    readonly property real implicitCheckBoxHeight:           QGCStyle.StyleMetrics.implicitCheckBoxHeight
    readonly property real implicitTextFieldWidth:           QGCStyle.StyleMetrics.implicitTextFieldWidth
    readonly property real implicitTextFieldHeight:          QGCStyle.StyleMetrics.implicitTextFieldHeight
    readonly property real implicitComboBoxHeight:           QGCStyle.StyleMetrics.implicitComboBoxHeight
    readonly property real implicitComboBoxWidth:            QGCStyle.StyleMetrics.implicitComboBoxWidth
    readonly property real comboBoxPadding:                  QGCStyle.StyleMetrics.comboBoxPadding
    readonly property real implicitSliderHeight:             QGCStyle.StyleMetrics.implicitSliderHeight
    readonly property real defaultBorderRadius:              QGCStyle.StyleMetrics.defaultBorderRadius

    readonly property real radioButtonIndicatorSize:         QGCStyle.StyleMetrics.radioButtonIndicatorSize

    readonly property string normalFontFamily:      QGCStyle.StyleTypography.normalFontFamily
    readonly property string fixedFontFamily:       QGCStyle.StyleTypography.fixedFontFamily
}
