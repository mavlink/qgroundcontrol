pragma Singleton

import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.ScreenToolsController 1.0

Item {
    signal repaintRequested

    readonly property real defaultFontPixelSize:    _textMeasure.contentHeight * ScreenToolsController.defaultFontPixelSizeRatio
    readonly property real defaultFontPixelHeight:  defaultFontPixelSize
    readonly property real defaultFontPixelWidth:   _textMeasure.contentWidth
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
        id: _textMeasure
        text: "X"
    }

    Connections {
        target: ScreenToolsController
        onRepaintRequested: repaintRequested()
    }
}
