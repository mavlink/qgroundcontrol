pragma Singleton

import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Window 2.2

import QGroundControl.ScreenToolsController 1.0

Item {
    id: _screenTools

    signal repaintRequested

    property real availableHeight:          0

    property real defaultFontPointSize:     1
    property real defaultFontPixelHeight:   1
    property real defaultFontPixelWidth:    1
    property real smallFontPointSize:       1
    property real mediumFontPointSize:      1
    property real largeFontPointSize:       1

    readonly property real smallFontPointRatio:      0.75
    readonly property real mediumFontPointRatio:     1.25
    readonly property real largeFontPointRatio:      1.5

    // Font scaling based on system font
    readonly property real  fontHRatio: _textMeasure.fontHeight / _defaultFont.fontHeight

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
                //-- Linux and Mac OS we use a slightly smaller font
                if(ScreenToolsController.isMacOS || ScreenToolsController.isLinux)
                    return _defaultFont.font.pointSize - 1
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
