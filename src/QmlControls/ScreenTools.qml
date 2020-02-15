pragma Singleton

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2

import QGroundControl                       1.0
import QGroundControl.ScreenToolsController 1.0

/*!
 The ScreenTools Singleton provides information on QGC's standard font metrics. It also provides information on screen
 size which can be used to adjust user interface for varying available screen real estate.

 QGC has four standard font sizes: default, small, medium and large. The QGC controls use the default font for display and you should use this font
 for most text within the system that is drawn using something other than a standard QGC control. The small font is smaller than the default font.
 The medium and large fonts are larger than the default font.

 Usage:

        import QGroundControl.ScreenTools 1.0

        Rectangle {
            anchors.fill:       parent
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            ...
        }
*/
Item {
    id: _screenTools

    //-- The point and pixel font size values are computed at runtime

    property real defaultFontPointSize:     10
    property real platformFontPointSize:    10

    /// You can use this property to position ui elements in a screen resolution independent manner. Using fixed positioning values should not
    /// be done. All positioning should be done using anchors or a ratio of the defaultFontPixelHeight and defaultFontPixelWidth values. This way
    /// your ui elements will reposition themselves appropriately on varying screen sizes and resolutions.
    property real defaultFontPixelHeight:   10

    /// You can use this property to position ui elements in a screen resolution independent manner. Using fixed positioning values should not
    /// be done. All positioning should be done using anchors or a ratio of the defaultFontPixelHeight and defaultFontPixelWidth values. This way
    /// your ui elements will reposition themselves appropriately on varying screen sizes and resolutions.
    property real defaultFontPixelWidth:    10

    property real smallFontPointSize:       10
    property real mediumFontPointSize:      10
    property real largeFontPointSize:       10

    property real toolbarHeight:            0

    readonly property real smallFontPointRatio:      0.75
    readonly property real mediumFontPointRatio:     1.25
    readonly property real largeFontPointRatio:      1.5

    property real realPixelDensity: {
        //-- If a plugin defines it, just use what it tells us
        if(QGroundControl.corePlugin.options.devicePixelDensity != 0) {
            return QGroundControl.corePlugin.options.devicePixelDensity
        }
        //-- Android is rather unreliable
        if(isAndroid) {
            // Lets assume it's unlikely you have a tablet over 300mm wide
            if((Screen.width / Screen.pixelDensity) > 300) {
                return Screen.pixelDensity * 2
            }
        }
        //-- Let's use what the system tells us
        return Screen.pixelDensity
    }

    property bool isAndroid:                        ScreenToolsController.isAndroid
    property bool isiOS:                            ScreenToolsController.isiOS
    property bool isMobile:                         ScreenToolsController.isMobile
    property bool isWindows:                        ScreenToolsController.isWindows
    property bool isDebug:                          ScreenToolsController.isDebug
    property bool isMac:                            ScreenToolsController.isMacOS
    property bool isTinyScreen:                     (Screen.width / realPixelDensity) < 120 // 120mm
    property bool isShortScreen:                    ((Screen.height / realPixelDensity) < 120) || (ScreenToolsController.isMobile && ((Screen.height / Screen.width) < 0.6))
    property bool isHugeScreen:                     (Screen.width / realPixelDensity) >= (23.5 * 25.4) // 27" monitor
    property bool isSerialAvailable:                ScreenToolsController.isSerialAvailable

    readonly property real minTouchMillimeters:     10      ///< Minimum touch size in millimeters
    property real minTouchPixels:                   0       ///< Minimum touch size in pixels

    // The implicit heights/widths for our custom control set
    property real implicitButtonWidth:              Math.round(defaultFontPixelWidth *  (isMobile ? 7.0 : 5.0))
    property real implicitButtonHeight:             Math.round(defaultFontPixelHeight * (isMobile ? 2.0 : 1.6))
    property real implicitCheckBoxHeight:           Math.round(defaultFontPixelHeight * (isMobile ? 2.0 : 1.0))
    property real implicitRadioButtonHeight:        implicitCheckBoxHeight
    property real implicitTextFieldHeight:          Math.round(defaultFontPixelHeight * (isMobile ? 2.0 : 1.6))
    property real implicitComboBoxHeight:           Math.round(defaultFontPixelHeight * (isMobile ? 2.0 : 1.6))
    property real implicitComboBoxWidth:            Math.round(defaultFontPixelWidth *  (isMobile ? 7.0 : 5.0))
    property real comboBoxPadding:                  defaultFontPixelWidth
    property real implicitSliderHeight:             isMobile ? Math.max(defaultFontPixelHeight, minTouchPixels) : defaultFontPixelHeight
    // It's not possible to centralize an even number of pixels, checkBoxIndicatorSize should be an odd number to allow centralization
    property real checkBoxIndicatorSize:            2 * Math.floor(defaultFontPixelHeight * (isMobile ? 1.5 : 1.0) / 2) + 1
    property real radioButtonIndicatorSize:         checkBoxIndicatorSize

    readonly property string normalFontFamily:      ScreenToolsController.normalFontFamily
    readonly property string demiboldFontFamily:    ScreenToolsController.boldFontFamily
    readonly property string fixedFontFamily:       ScreenToolsController.fixedFontFamily
    /* This mostly works but for some reason, reflowWidths() in SetupView doesn't change size.
       I've disabled (in release builds) until I figure out why. Changes require a restart for now.
    */
    Connections {
        target: QGroundControl.settingsManager.appSettings.appFontPointSize
        onValueChanged: {
            _setBasePointSize(QGroundControl.settingsManager.appSettings.appFontPointSize.value)
        }
    }

    onRealPixelDensityChanged: {
        _setBasePointSize(defaultFontPointSize)
    }

    function printScreenStats() {
        console.log('ScreenTools: Screen.width: ' + Screen.width + ' Screen.height: ' + Screen.height + ' Screen.pixelDensity: ' + Screen.pixelDensity)
    }

    /// Returns the current x position of the mouse in global screen coordinates.
    function mouseX() {
        return ScreenToolsController.mouseX()
    }

    /// Returns the current y position of the mouse in global screen coordinates.
    function mouseY() {
        return ScreenToolsController.mouseY()
    }

    /// \private
    function _setBasePointSize(pointSize) {
        _textMeasure.font.pointSize = pointSize
        defaultFontPointSize    = pointSize
        defaultFontPixelHeight  = Math.round(_textMeasure.fontHeight/2.0)*2
        defaultFontPixelWidth   = Math.round(_textMeasure.fontWidth/2.0)*2
        smallFontPointSize      = defaultFontPointSize  * _screenTools.smallFontPointRatio
        mediumFontPointSize     = defaultFontPointSize  * _screenTools.mediumFontPointRatio
        largeFontPointSize      = defaultFontPointSize  * _screenTools.largeFontPointRatio
        minTouchPixels          = Math.round(minTouchMillimeters * realPixelDensity)
        if (minTouchPixels / Screen.height > 0.15) {
            // If using physical sizing takes up too much of the vertical real estate fall back to font based sizing
            minTouchPixels      = defaultFontPixelHeight * 3
        }
        toolbarHeight           = isMobile ? minTouchPixels : defaultFontPixelHeight * 3
        toolbarHeight           = toolbarHeight * QGroundControl.corePlugin.options.toolbarHeightMultiplier
    }

    Text {
        id:     _defaultFont
        text:   "X"
    }

    Text {
        id:     _textMeasure
        text:   "X"
        font.family:    normalFontFamily
        property real   fontWidth:    contentWidth
        property real   fontHeight:   contentHeight
        Component.onCompleted: {
            //-- First, compute platform, default size
            if(ScreenToolsController.isMobile) {
                //-- Check iOS really tiny screens (iPhone 4s/5/5s)
                if(ScreenToolsController.isiOS) {
                    if(ScreenToolsController.isiOS && Screen.width < 570) {
                        // For iPhone 4s size we don't fit with additional tweaks to fit screen,
                        // we will just drop point size to make things fit. Correct size not yet determined.
                        platformFontPointSize = 12;  // This will be lowered in a future pull
                    } else {
                        platformFontPointSize = 14;
                    }
                } else if((Screen.width / realPixelDensity) < 120) {
                    platformFontPointSize = 11;
                // Other Android
                } else {
                    platformFontPointSize = 14;
                }
            } else {
                platformFontPointSize = _defaultFont.font.pointSize;
            }
            //-- See if we are using a custom size
            var _appFontPointSizeFact = QGroundControl.settingsManager.appSettings.appFontPointSize
            var baseSize = _appFontPointSizeFact.value
            //-- Sanity check
            if(baseSize < _appFontPointSizeFact.min || baseSize > _appFontPointSizeFact.max) {
                baseSize = platformFontPointSize;
                _appFontPointSizeFact.value = baseSize
            }
            //-- Set size saved in settings
            _screenTools._setBasePointSize(baseSize);
        }
    }
}
