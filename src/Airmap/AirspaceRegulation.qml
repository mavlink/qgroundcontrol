import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQml                    2.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Airmap        1.0

Item {
    id:     _root
    height: regCol.height

    property var textColor:     "white"
    property var regColor:      "white"
    property var regTitle:      ""
    property var regText:       ""

    Column {
        id:                     regCol
        spacing:                ScreenTools.defaultFontPixelHeight * 0.5
        anchors.left:           parent.left
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth  * 0.5
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth  * 0.5
        Row {
            spacing:                    ScreenTools.defaultFontPixelWidth
            anchors.left:               parent.left
            anchors.leftMargin:         ScreenTools.defaultFontPixelWidth * 0.5
            anchors.right:              parent.right
            anchors.rightMargin:        ScreenTools.defaultFontPixelWidth * 0.5
            Rectangle {
                width:                  height
                height:                 ScreenTools.defaultFontPixelWidth * 1.5
                radius:                 height * 0.5
                color:                  regColor
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:                   regTitle
                color:                  textColor
            }
        }
        QGCLabel {
            text:                       regText
            color:                      textColor
            anchors.left:               parent.left
            anchors.leftMargin:         ScreenTools.defaultFontPixelWidth * 0.5
            anchors.right:              parent.right
            anchors.rightMargin:        ScreenTools.defaultFontPixelWidth * 0.5
            wrapMode:                   Text.WordWrap
            font.pointSize:             ScreenTools.smallFontPointSize
        }
    }
}
