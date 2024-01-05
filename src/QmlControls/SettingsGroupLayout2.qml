import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette

Rectangle {
    implicitWidth:  layout.implicitWidth + (_margin * 2)
    implicitHeight: layout.implicitHeight + (_margin * 2)
    color:          "transparent"
    border.color:   Qt.darker(QGroundControl.globalPalette.text, 4)
    border.width:   1
    radius:         ScreenTools.defaultFontPixelHeight / 2

    default property alias contentItem: _contentLayout.data

    property string heading
    property string headingDescription

    property real _margin:          ScreenTools.defaultFontPixelHeight / 2
    property real _headingMargin:   ScreenTools.defaultFontPixelWidth

    ColumnLayout {
        id:         layout
        x:          _margin
        y:          _margin
        width:      Math.max(_contentLayout.implicitWidth, parent.width - (_margin * 2))
        spacing:    _margin

        ColumnLayout {
            Layout.leftMargin:  _headingMargin
            spacing:            0
            visible:            heading !== ""

            QGCLabel { 
                text:               heading
                font.bold:          true
            }

            QGCLabel { 
                Layout.fillWidth:   true
                text:               headingDescription
                wrapMode:           Text.WordWrap
                font.pointSize:     ScreenTools.smallFontPointSize
                visible:            headingDescription !== ""
            }
        }

        ColumnLayout {
            id:                 _contentLayout
            Layout.fillWidth:   children[0].Layout.fillWidth
            spacing:            _margin
        }
    }
}
