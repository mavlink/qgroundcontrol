pragma Singleton
import QtQuick
import QGroundControl
import QGroundControl.Controls

QtObject {
    id: root

    property int objectWidth: ScreenTools.defaultFontPixelWidth * 7
    property int objectHeight: ScreenTools.defaultFontPixelHeight * 7
    property int width: ScreenTools.defaultFontPixelWidth
    property int height: ScreenTools.defaultFontPixelHeight
    property int bigMargin: ScreenTools.defaultFontPixelWidth
    property int margin: ScreenTools.defaultFontPixelWidth / 2
    property real smallMargin: (ScreenTools.defaultFontPixelWidth / 2) * 0.4
    property real radius: ScreenTools.defaultFontPixelWidth / 2

    property int lineWidth: 1
    property int thickLineWidth: 3

    property real smallText: ScreenTools.smallFontPointSize
    property real svText: ScreenTools.mediumFontPointSize  * 0.7
    property real mediumText: ScreenTools.mediumFontPointSize
    property real largeText: ScreenTools.largeFontPointSize

    property real buttonHeight: ScreenTools.defaultFontPixelHeight * 1.6
    //property real buttonWidth: 
}
