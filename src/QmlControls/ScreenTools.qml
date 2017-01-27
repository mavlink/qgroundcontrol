pragma Singleton

import QtQuick 2.4
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

    property real availableHeight:          0
    property real toolbarHeight:            defaultFontPixelHeight * 3

    readonly property real smallFontPointRatio:      0.75
    readonly property real mediumFontPointRatio:     1.25
    readonly property real largeFontPointRatio:      1.5

    property bool isAndroid:        ScreenToolsController.isAndroid
    property bool isiOS:            ScreenToolsController.isiOS
    property bool isMobile:         ScreenToolsController.isMobile
    property bool isDebug:          ScreenToolsController.isDebug
    property bool isTinyScreen:     (Screen.width / Screen.pixelDensity) < 120 // 120mm
    property bool isShortScreen:    ScreenToolsController.isMobile && ((Screen.height / Screen.width) < 0.6) // Nexus 7 for example

    readonly property string normalFontFamily:      "opensans"
    readonly property string demiboldFontFamily:    "opensans-demibold"

    /* This mostly works but for some reason, reflowWidths() in SetupView doesn't change size.
       I've disabled (in release builds) until I figure out why. Changes require a restart for now.
    */
    Connections {
        target: QGroundControl
        onBaseFontPointSizeChanged: {
            if(ScreenToolsController.isDebug)
                _setBasePointSize(QGroundControl.baseFontPointSize)
        }
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
            var baseSize = QGroundControl.baseFontPointSize;
            //-- If this is the first time (not saved in settings)
            if(baseSize < 6 || baseSize > 48) {
                //-- Init base size base on the platform
                if(ScreenToolsController.isMobile) {
                    //-- Check iOS really tiny screens (iPhone 4s/5/5s)
                    if(ScreenToolsController.isiOS) {
                        if(ScreenToolsController.isiOS && Screen.width < 570) {
                            // For iPhone 4s size we don't fit with additional tweaks to fit screen,
                            // we will just drop point size to make things fit. Correct size not yet determined.
                            baseSize = 12;  // This will be lowered in a future pull
                        } else {
                            baseSize = 12;
                        }
                    } else if((Screen.width / Screen.pixelDensity) < 120) {
                        baseSize = 11;
                    // Other Android
                    } else {
                        baseSize = 14;
                    }
                } else {
                    //-- Mac OS
                    if(ScreenToolsController.isMacOS)
                        baseSize = _defaultFont.font.pointSize;
                    //-- Linux
                    else if(ScreenToolsController.isLinux)
                        baseSize = _defaultFont.font.pointSize - 3.25;
                    //-- Windows
                    else
                        baseSize = _defaultFont.font.pointSize;
                }
                QGroundControl.baseFontPointSize = baseSize
                //-- Release build doesn't get signal
                if(!ScreenToolsController.isDebug)
                    _screenTools._setBasePointSize(baseSize);
            } else {
                //-- Set size saved in settings
                _screenTools._setBasePointSize(baseSize);
            }
        }
    }
}
