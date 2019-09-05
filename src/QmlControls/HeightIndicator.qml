import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.11

import QGroundControl.ScreenTools 1.0

ColumnLayout {
    spacing: 0

    property color  textColor:  "white"
    property string heightText: "30 ft"

    Rectangle {
        width:  ScreenTools.defaultFontPixelWidth * 3
        height: 1
        color:  textColor
        Layout.alignment: Qt.AlignHCenter
    }

    Rectangle {
        width:  1
        height: ScreenTools.defaultFontPixelWidth * 1
        color:  textColor
        Layout.alignment: Qt.AlignHCenter
    }

    QGCLabel {
        text:   heightText
        color:  textColor
        Layout.alignment: Qt.AlignHCenter
    }

    Rectangle {
        width:  1
        height: ScreenTools.defaultFontPixelWidth * 1
        color:  textColor
        Layout.alignment: Qt.AlignHCenter
    }

    Rectangle {
        width:  ScreenTools.defaultFontPixelWidth * 3
        height: 1
        color:  textColor
        Layout.alignment: Qt.AlignHCenter
    }
}
