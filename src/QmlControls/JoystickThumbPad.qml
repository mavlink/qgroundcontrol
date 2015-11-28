import QtQuick                  2.5
import QtQuick.Controls         1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    radius:         width / 2
    border.color:   mapPal.thumbJoystick
    border.width:   2
    color:          "transparent"

    property alias  lightColors:            mapPal.lightColors  /// true: use light colors from QGCMapPalette for drawing
    property var    stickPosition:          Qt.point(0, 0)
    property real   xAxis:                  0                   /// Value range [-1,1], negative values left stick, positive values right stick
    property real   yAxis:                  0                   /// Value range [-1,1], negative values up stick, positive values down stick
    property bool   yAxisThrottle:          false               /// true: yAxis used for throttle, range [1,0], positive value are stick up

    property bool   _stickCenteredOnce: false

    QGCMapPalette { id: mapPal }

    onWidthChanged: {
        if (!_stickCenteredOnce && width != 0) {
            reCenter()
        }
    }

    onStickPositionChanged: {
        var xAxisTemp = stickPosition.x / width
        xAxisTemp *= 2.0
        xAxisTemp -= 1.0
        xAxis = xAxisTemp

        var yAxisTemp = stickPosition.y / width
        yAxisTemp *= 2.0
        yAxisTemp -= 1.0
        if (yAxisThrottle) {
            yAxisTemp = ((yAxisTemp * -1.0) / 2.0) + 0.5
        }
        yAxis = yAxisTemp
    }

    function reCenter()
    {
        stickPosition = Qt.point(width / 2, width / 2)
    }

    Column {
        QGCLabel { text: xAxis }
        QGCLabel { text: yAxis }
    }

    Rectangle {
        anchors.margins:    parent.width / 4
        anchors.fill:       parent
        radius:             width / 2
        border.color:       mapPal.thumbJoystick
        border.width:       2
        color:              "transparent"
    }

    Rectangle {
        width:  hatWidth
        height: hatWidth
        radius: hatWidthHalf
        color:  mapPal.thumbJoystick
        x:      stickPosition.x - hatWidthHalf
        y:      stickPosition.y - hatWidthHalf

        readonly property real hatWidth:        ScreenTools.defaultFontPixelHeight
        readonly property real hatWidthHalf:    ScreenTools.defaultFontPixelHeight / 2
    }
}
