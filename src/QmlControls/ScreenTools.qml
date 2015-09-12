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
    readonly property real mediumFontPixelSize:     defaultFontPixelSize * ScreenToolsController.mediumFontPixelSizeRatio
    readonly property real largeFontPixelSize:      defaultFontPixelSize * ScreenToolsController.largeFontPixelSizeRatio

    property bool isAndroid:        ScreenToolsController.isAndroid
    property bool isiOS:            ScreenToolsController.isiOS
    property bool isMobile:         ScreenToolsController.isMobile

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
