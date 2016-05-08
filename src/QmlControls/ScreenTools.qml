pragma Singleton

import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Window 2.2

import QGroundControl                       1.0
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

    /* This mostly works but for some reason, reflowWidths() in SetupView doesn't change size.
       I've disabled (in release builds) until I figure out why. Changes require a restart for now.
    */
    Connections {
        target: QGroundControl
        onBaseFontPointSizeChanged: {
            if(ScreenToolsController.isDebug)
                setBasePointSize(QGroundControl.baseFontPointSize)
        }
    }

    function mouseX() {
        return ScreenToolsController.mouseX()
    }

    function mouseY() {
        return ScreenToolsController.mouseY()
    }

    function setBasePointSize(pointSize) {
        _textMeasure.font.pointSize = pointSize
        defaultFontPointSize    = pointSize
        defaultFontPixelHeight  = _textMeasure.fontHeight
        defaultFontPixelWidth   = _textMeasure.fontWidth
        smallFontPointSize      = defaultFontPointSize  * _screenTools.smallFontPointRatio
        mediumFontPointSize     = defaultFontPointSize  * _screenTools.mediumFontPointRatio
        largeFontPointSize      = defaultFontPointSize  * _screenTools.largeFontPointRatio
        console.log("setBasePointSize", defaultFontPixelWidth)
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
                    // Small Devices
                    if((Screen.width / Screen.pixelDensity) < 120)
                        baseSize = 11;
                    // iOS
                    else if(ScreenToolsController.isiOS)
                        baseSize = 13;
                    // Android
                    else
                        baseSize = 14;
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
                    _screenTools.setBasePointSize(baseSize);
            } else {
                //-- Set size saved in settings
                _screenTools.setBasePointSize(baseSize);
            }
        }
    }
}
