import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQml                    2.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Airmap            1.0
import QGroundControl.SettingsManager   1.0

Item {
    id:             _root
    height:         ScreenTools.defaultFontPixelHeight * 2
    property alias text:        title.text
    readonly property color     _colorOrange:       "#d75e0d"
    readonly property color     _colorYellow:       "#d7c61d"
    readonly property color     _colorRed:          "#aa1200"
    readonly property color     _colorGreen:        "#125f00"
    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }
    Rectangle {
        anchors.fill:       parent
        color:              qgcPal.windowShade
    }
    Row {
        spacing:            ScreenTools.defaultFontPixelWidth * 2
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            height:         _root.height
            width:          ScreenTools.defaultFontPixelWidth * 0.75
            color:          _colorGreen
        }
        QGCLabel {
            id:                 title
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    QGCColoredImage {
        source:                 "qrc:/airmap/expand.svg"
        height:                 ScreenTools.defaultFontPixelHeight
        width:                  height
        color:                  qgcPal.text
        fillMode:               Image.PreserveAspectFit
        sourceSize.height:      height
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter: parent.verticalCenter
    }
}
