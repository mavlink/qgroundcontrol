pragma Singleton

import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.ScreenToolsController 1.0

Item {
    signal repaintRequested

    readonly property real defaultFontPixelSize:    _textMeasure.fontHeight * ScreenToolsController.defaultFontPixelSizeRatio
    readonly property real defaultFontPixelHeight:  defaultFontPixelSize
    readonly property real defaultFontPixelWidth:   _textMeasure.fontWidth
    readonly property real smallFontPixelSize:      defaultFontPixelSize * ScreenToolsController.smallFontPixelSizeRatio

    // On OSX ElCapitan with Qt 5.4.0 any font pixel size above 19 shows garbage test. No idea why at this point.
    // Will remove Math.min when problem is figure out.
    readonly property real mediumFontPixelSize:     Math.min(defaultFontPixelSize * ScreenToolsController.mediumFontPixelSizeRatio, ScreenToolsController.isMobile ? 10000 : 19)
    readonly property real largeFontPixelSize:      Math.min(defaultFontPixelSize * ScreenToolsController.largeFontPixelSizeRatio, ScreenToolsController.isMobile ? 10000 : 19)

    property bool isAndroid:        ScreenToolsController.isAndroid
    property bool isiOS:            ScreenToolsController.isiOS
    property bool isMobile:         ScreenToolsController.isMobile
    property bool isDebug:          ScreenToolsController.isDebug

    function mouseX() {
        return ScreenToolsController.mouseX()
    }

    function mouseY() {
        return ScreenToolsController.mouseY()
    }

    Text {
        id:     _textMeasure
        text:   "X"

        property real fontWidth:    contentWidth * (ScreenToolsController.testHighDPI ? 2 : 1)
        property real fontHeight:   contentHeight * (ScreenToolsController.testHighDPI ? 2 : 1)
    }

    Connections {
        target: ScreenToolsController
        onRepaintRequested: repaintRequested()
    }
}
