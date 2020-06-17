import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.11

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

ColumnLayout {
    id:         _root
    spacing:    0

    property var map
    property string heightText: "30 ft"

    property color _textColor:  _mapPalette.text

    QGCMapPalette { id: _mapPalette; lightColors: _root.map.isSatelliteMap }

    Rectangle {
        width:              ScreenTools.defaultFontPixelWidth * 3
        height:             1
        color:              _textColor
        Layout.alignment:   Qt.AlignHCenter
    }

    Rectangle {
        width:              1
        height:             ScreenTools.defaultFontPixelWidth * 1
        color:              _textColor
        Layout.alignment:   Qt.AlignHCenter
    }

    QGCMapLabel {
        map:                _root.map
        text:               heightText
        Layout.alignment:   Qt.AlignHCenter
    }

    Rectangle {
        width:              1
        height:             ScreenTools.defaultFontPixelWidth * 1
        color:              _textColor
        Layout.alignment:   Qt.AlignHCenter
    }

    Rectangle {
        width:              ScreenTools.defaultFontPixelWidth * 3
        height:             1
        color:              _textColor
        Layout.alignment:   Qt.AlignHCenter
    }
}
