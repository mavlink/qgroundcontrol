pragma Singleton

import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Window 2.2

import QGroundControl.ScreenToolsController 1.0

Item {
    signal repaintRequested

    property real availableHeight:          0

    property real defaultFontPixelSize:     1
    property real defaultFontPixelHeight:   1
    property real defaultFontPixelWidth:    1
    property real smallFontPixelSize:       1
    property real smallFontPixelHeight:     1
    property real smallFontPixelWidth:      1
    property real mediumFontPixelSize:      1
    property real largeFontPixelSize:       1

    // To proportionally scale fonts

    readonly property real  _defaultFontHeight: 16
    readonly property real  fontHRatio:         isTinyScreen ? (_textMeasure.contentHeight / _defaultFontHeight) * 0.75 : (_textMeasure.contentHeight / _defaultFontHeight)
    readonly property real  realFontHeight:     _textMeasure.contentHeight
    readonly property real  realFontWidth :     _textMeasure.contentWidth

    // On OSX ElCapitan with Qt 5.4.0 any font pixel size above 19 shows garbage test. No idea why at this point.
    // Will remove Math.min when problem is figure out.
    // readonly property real mediumFontPixelSize:     Math.min(defaultFontPixelSize * ScreenToolsController.mediumFontPixelSizeRatio, ScreenToolsController.isMobile ? 10000 : 19)
    // readonly property real largeFontPixelSize:      Math.min(defaultFontPixelSize * ScreenToolsController.largeFontPixelSizeRatio, ScreenToolsController.isMobile ? 10000 : 19)

    property bool isAndroid:        ScreenToolsController.isAndroid
    property bool isiOS:            ScreenToolsController.isiOS
    property bool isMobile:         ScreenToolsController.isMobile
    property bool isDebug:          ScreenToolsController.isDebug
    property bool isTinyScreen:     (Screen.width  / Screen.pixelDensity) < 120 // 120mm
    property bool isShortScreen:    ScreenToolsController.isMobile && ((Screen.height / Screen.width) < 0.6) // Nexus 7 for example

    readonly property string normalFontFamily:      "opensans"
    readonly property string demiboldFontFamily:    "opensans-demibold"

    function mouseX() {
        return ScreenToolsController.mouseX()
    }

    function mouseY() {
        return ScreenToolsController.mouseY()
    }

    Text {
        id:     _textMeasure
        text:   "X"
        font.family:    normalFontFamily
        property real   fontWidth:    contentWidth  * (ScreenToolsController.testHighDPI ? 2 : 1)
        property real   fontHeight:   contentHeight * (ScreenToolsController.testHighDPI ? 2 : 1)
        Component.onCompleted: {
            var fudgeFactor = 1.0
            // Small Android Devices (Large Scaling Error)
            if(ScreenToolsController.isAndroid && (Screen.width  / Screen.pixelDensity) < 120) {
                fudgeFactor = 0.6
            // Nexus 7 type
            } else if(ScreenToolsController.isAndroid && (Screen.height / Screen.width) < 0.6) {
                fudgeFactor = 0.75
            // iPhones
            } else if (ScreenToolsController.isiOS && (Screen.width  / Screen.pixelDensity) < 120) {
                fudgeFactor = 0.75
            }
            defaultFontPixelSize    = _textMeasure.fontHeight   * ScreenToolsController.defaultFontPixelSizeRatio   * fudgeFactor
            defaultFontPixelHeight  =  defaultFontPixelSize     * fudgeFactor
            defaultFontPixelWidth   =  _textMeasure.fontWidth   * ScreenToolsController.defaultFontPixelSizeRatio   * fudgeFactor
            smallFontPixelSize      = defaultFontPixelSize      * ScreenToolsController.smallFontPixelSizeRatio     * fudgeFactor
            smallFontPixelHeight    = smallFontPixelSize        * fudgeFactor
            smallFontPixelWidth     = defaultFontPixelWidth     * ScreenToolsController.smallFontPixelSizeRatio     * fudgeFactor
            mediumFontPixelSize     = defaultFontPixelSize      * ScreenToolsController.mediumFontPixelSizeRatio    * fudgeFactor
            largeFontPixelSize      = defaultFontPixelSize      * ScreenToolsController.largeFontPixelSizeRatio     * fudgeFactor
        }
    }
}
