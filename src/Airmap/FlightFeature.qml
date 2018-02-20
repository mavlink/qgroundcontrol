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

Rectangle {
    id:                         _root
    height:                     questionCol.height + (ScreenTools.defaultFontPixelHeight * 1.25)
    color:                      qgcPal.windowShade
    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }
    Column {
        id:         questionCol
        spacing:    ScreenTools.defaultFontPixelHeight * 0.5
        anchors.centerIn:   parent
        QGCLabel {
            text:   "Question?"
            anchors.left: questionText.left
        }
        QGCTextField {
            id:     questionText
            width:  _root.width - (ScreenTools.defaultFontPixelWidth * 2)
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
