pragma Singleton

import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Window 2.2

import QGroundControl.ScreenToolsController 1.0

Item {
    id: _screenTools

    signal repaintRequested

    property real availableHeight:          0

    //-- These are computed at runtime
    property real defaultFontPointSize:     10
    property real defaultFontPixelHeight:   10
    property real defaultFontPixelWidth:    10
    property real smallFontPointSize:       10
    property real mediumFontPointSize:      10
    property real largeFontPointSize:       10

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

    function mouseX() {
        return ScreenToolsController.mouseX()
    }

    function mouseY() {
        return ScreenToolsController.mouseY()
    }

    Text {
        id:     _defaultFont
        text:   "X"
        property real fontHeight: contentHeight
    }

    Text {
        id:     _textMeasure
        text:   "X"
        font.family:    normalFontFamily
        font.pointSize: {
            if(ScreenToolsController.isMobile) {
                // Small Devices
                if((Screen.width / Screen.pixelDensity) < 120) {
                    return 11;
                // iOS
                } else if(ScreenToolsController.isiOS) {
                    return 13;
                // Android
                } else {
                    return 14;
                }
            } else {
                //-- Mac OS
                if(ScreenToolsController.isMacOS)
                    return _defaultFont.font.pointSize - 1
                //-- Linux
                if(ScreenToolsController.isLinux)
                    return _defaultFont.font.pointSize - 3.25
                else
                    return _defaultFont.font.pointSize
            }
        }
        property real   fontWidth:    contentWidth
        property real   fontHeight:   contentHeight
        Component.onCompleted: {
            defaultFontPointSize    = _textMeasure.font.pointSize
            defaultFontPixelHeight  = _textMeasure.fontHeight
            defaultFontPixelWidth   = _textMeasure.fontWidth
            smallFontPointSize      = defaultFontPointSize  * _screenTools.smallFontPointRatio
            mediumFontPointSize     = defaultFontPointSize  * _screenTools.mediumFontPointRatio
            largeFontPointSize      = defaultFontPointSize  * _screenTools.largeFontPointRatio
        }
    }
}
