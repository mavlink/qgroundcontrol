pragma Singleton

import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Window 2.2

import QGroundControl.ScreenToolsController 1.0

Item {
    signal repaintRequested

    property real availableHeight: 0

    readonly property real defaultFontPixelSize:    _textMeasure.fontHeight * ScreenToolsController.defaultFontPixelSizeRatio
    readonly property real defaultFontPixelHeight:  defaultFontPixelSize
    readonly property real defaultFontPixelWidth:   _textMeasure.fontWidth
    readonly property real smallFontPixelSize:      defaultFontPixelSize * ScreenToolsController.smallFontPixelSizeRatio
    readonly property real smallFontPixelHeight:    smallFontPixelSize
    readonly property real smallFontPixelWidth:     defaultFontPixelWidth * ScreenToolsController.smallFontPixelSizeRatio

    // To proportionally scale fonts

    readonly property real  _defaultFontHeight: 16
    readonly property real  fontHRatio:         isTinyScreen ? (_textMeasure.contentHeight / _defaultFontHeight) * 0.75 : (_textMeasure.contentHeight / _defaultFontHeight)
    readonly property real  realFontHeight:     _textMeasure.contentHeight
    readonly property real  realFontWidth :     _textMeasure.contentWidth

    // On OSX ElCapitan with Qt 5.4.0 any font pixel size above 19 shows garbage test. No idea why at this point.
    // Will remove Math.min when problem is figure out.
    readonly property real mediumFontPixelSize:     Math.min(defaultFontPixelSize * ScreenToolsController.mediumFontPixelSizeRatio, ScreenToolsController.isMobile ? 10000 : 19)
    readonly property real largeFontPixelSize:      Math.min(defaultFontPixelSize * ScreenToolsController.largeFontPixelSizeRatio, ScreenToolsController.isMobile ? 10000 : 19)

    property bool isAndroid:        ScreenToolsController.isAndroid
    property bool isiOS:            ScreenToolsController.isiOS
    property bool isMobile:         ScreenToolsController.isMobile
    property bool isDebug:          ScreenToolsController.isDebug
    property bool isTinyScreen:     (Screen.width  / Screen.pixelDensity) < 120 // 120mm
    property bool isShortScreen:    ScreenToolsController.isMobile && ((Screen.height / Screen.width) < 0.6) // Nexus 7 for example

    function mouseX() {
        return ScreenToolsController.mouseX()
    }

    function mouseY() {
        return ScreenToolsController.mouseY()
    }

    Text {
        id:     _textMeasure
        text:   "X"
        property real fontWidth:    contentWidth  * (ScreenToolsController.testHighDPI ? 2 : 1)
        property real fontHeight:   contentHeight * (ScreenToolsController.testHighDPI ? 2 : 1)
    }
}
