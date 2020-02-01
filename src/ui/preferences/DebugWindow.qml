/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2
import QtQuick.Window   2.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.ScreenTools 1.0

Item {

    Text {
        id:             _textMeasure
        text:           "X"
        color:          qgcPal.window
        font.family:    ScreenTools.normalFontFamily
    }

    GridLayout {
        anchors.margins: 20
        anchors.top:     parent.top
        anchors.left:    parent.left
        columns: 3
        Text {
            text:   qsTr("Qt Platform:")
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   Qt.platform.os
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Font Point Size 10")
            color:  qgcPal.text
            font.pointSize: 10
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Default font width:")
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   _textMeasure.contentWidth
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Font Point Size 10.5")
            color:  qgcPal.text
            font.pointSize: 10.5
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Default font height:")
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   _textMeasure.contentHeight
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Font Point Size 11")
            color:  qgcPal.text
            font.pointSize: 11
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Default font pixel size:")
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   _textMeasure.font.pointSize
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Font Point Size 11.5")
            color:  qgcPal.text
            font.pointSize: 11.5
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Default font point size:")
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   _textMeasure.font.pointSize
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Font Point Size 12")
            color:  qgcPal.text
            font.pointSize: 12
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("QML Screen Desktop:")
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   Screen.desktopAvailableWidth + " x " + Screen.desktopAvailableHeight
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Font Point Size 12.5")
            color:  qgcPal.text
            font.pointSize: 12.5
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("QML Screen Size:")
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   Screen.width + " x " + Screen.height
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("Font Point Size 13")
            color:  qgcPal.text
            font.pointSize: 13
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:   qsTr("QML Pixel Density:")
            color:  qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           Screen.pixelDensity.toFixed(4)
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Font Point Size 13.5")
            color:          qgcPal.text
            font.pointSize: 13.5
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("QML Pixel Ratio:")
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           Screen.devicePixelRatio
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Font Point Size 14")
            color:          qgcPal.text
            font.pointSize: 14
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Default Point:")
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           ScreenTools.defaultFontPointSize
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Font Point Size 14.5")
            color:          qgcPal.text
            font.pointSize: 14.5
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Computed Font Height:")
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           ScreenTools.defaultFontPixelHeight
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Font Point Size 15")
            color:          qgcPal.text
            font.pointSize: 15
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Computed Screen Height:")
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           (Screen.height / Screen.pixelDensity * Screen.devicePixelRatio).toFixed(0)
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Font Point Size 15.5")
            color:          qgcPal.text
            font.pointSize: 15.5
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Computed Screen Width:")
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           (Screen.width / Screen.pixelDensity * Screen.devicePixelRatio).toFixed(0)
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Font Point Size 16")
            color:          qgcPal.text
            font.pointSize: 16
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Desktop Available Width:")
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           Screen.desktopAvailableWidth
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Font Point Size 16.5")
            color:          qgcPal.text
            font.pointSize: 16.5
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Desktop Available Height:")
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           Screen.desktopAvailableHeight
            color:          qgcPal.text
            font.family:    ScreenTools.normalFontFamily
        }
        Text {
            text:           qsTr("Font Point Size 17")
            color:          qgcPal.text
            font.pointSize: 17
            font.family:    ScreenTools.normalFontFamily
        }
    }

    Rectangle {
        id:                 square
        width:              100
        height:             100
        color:              qgcPal.text
        anchors.right:      parent.right
        anchors.bottom:     parent.bottom
        anchors.margins:    10
        Text {
            text: "100x100"
            anchors.centerIn: parent
            color:  qgcPal.window
        }
    }

    Component.onCompleted: {
        for (var i = 10; i < 360; i = i + 60) {
            var colorValue = Qt.hsla(i/360, 0.85, 0.5, 1);
            seriesColors.push(colorValue)
            colorListModel.append({"colorValue": colorValue.toString()})
        }
    }

    property var seriesColors: []

    ListModel {
        id: colorListModel
    }

    Column {
        width:              100
        spacing:            0
        anchors.right:      square.left
        anchors.bottom:     parent.bottom
        anchors.margins:    10
        Repeater {
            model: colorListModel
            delegate: Rectangle {
                width:      100
                height:     100 / 6
                color:      colorValue
                Text {
                    text:   colorValue
                    color:  "#202020"
                    font.pointSize:     _textMeasure.font.pointSize * 0.75
                    anchors.centerIn:   parent
                }
            }
        }
    }

}
