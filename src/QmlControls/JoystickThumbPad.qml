import QtQuick                  2.12
import QtQuick.Controls         1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id:             _joyRoot

    property alias  lightColors:            mapPal.lightColors  ///< true: use light colors from QGCMapPalette for drawing
    property real   xAxis:                  0                   ///< Value range [-1,1], negative values left stick, positive values right stick
    property real   yAxis:                  0                   ///< Value range [-1,1], negative values down stick, positive values up stick
    property bool   yAxisPositiveRangeOnly: false               ///< true: value range [0,1], false: value range [-1,1]
    property bool   yAxisReCenter:          true                ///< true: snaps back to center on release, false: stays at current position on release
    property real   xPositionDelta:         0                   ///< Amount to move the control on x axis
    property real   yPositionDelta:         0                   ///< Amount to move the control on y axis

    property real   _centerXY:              width / 2
    property bool   _processTouchPoints:    false
    property color  _fgColor:               QGroundControl.globalPalette.text
    property color  _bgColor:               QGroundControl.globalPalette.window
    property real   _hatWidth:              ScreenTools.defaultFontPixelHeight
    property real   _hatWidthHalf:          _hatWidth / 2

    property real   stickPositionX:         _centerXY
    property real   stickPositionY:         yAxisReCenter ? _centerXY : height

    QGCMapPalette { id: mapPal }

    onWidthChanged:                     calculateXAxis()
    onStickPositionXChanged:            calculateXAxis()
    onHeightChanged:                    calculateYAxis()
    onStickPositionYChanged:            calculateYAxis()
    onYAxisPositiveRangeOnlyChanged:    calculateYAxis()

    function calculateXAxis() {
        if(!_joyRoot.visible) {
            return;
        }
        var xAxisTemp = stickPositionX / width
        xAxisTemp *= 2.0
        xAxisTemp -= 1.0
        xAxis = xAxisTemp
    }

    function calculateYAxis() {
        if(!_joyRoot.visible) {
            return;
        }
        var fullRange = yAxisPositiveRangeOnly ? 1 : 2
        var pctUp = 1.0 - (stickPositionY / height)
        var rangeUp = pctUp * fullRange
        if (!yAxisPositiveRangeOnly) {
            rangeUp -= 1
        }
        yAxis = rangeUp
    }

    function reCenter() {
        _processTouchPoints = false

        // Move control back to original position
        xPositionDelta = 0
        yPositionDelta = 0

        // Re-Center sticks as needed
        stickPositionX = _centerXY
        if (yAxisReCenter) {
            stickPositionY = _centerXY
        }
    }

    function thumbDown(touchPoints) {
        // Position the control around the initial thumb position
        xPositionDelta = touchPoints[0].x - _centerXY
        if (yAxisPositiveRangeOnly) {
            yPositionDelta = touchPoints[0].y - stickPositionY
        } else {
            yPositionDelta = touchPoints[0].y - _centerXY
        }
        // We need to wait until we move the control to the right position before we process touch points
        _processTouchPoints = true
    }

    /*
    // Keep in for debugging
    Column {
        QGCLabel { text: xAxis }
        QGCLabel { text: yAxis }
    }
    */

    Image {
        anchors.fill:       parent
        source:             "/res/JoystickBezelLight.png"
        mipmap:             true
        smooth:             true
    }

    Rectangle {
        anchors.fill:       parent
        radius:             width / 2
        color:              _bgColor
        opacity:            0.5

        Rectangle {
            anchors.margins:    parent.width / 4
            anchors.fill:       parent
            radius:             width / 2
            border.color:       _fgColor
            border.width:       2
            color:              "transparent"
        }

        Rectangle {
            anchors.fill:       parent
            radius:             width / 2
            border.color:       _fgColor
            border.width:       2
            color:              "transparent"
        }
    }

    QGCColoredImage {
        color:                      _fgColor
        visible:                    yAxisPositiveRangeOnly
        height:                     ScreenTools.defaultFontPixelHeight
        width:                      height
        sourceSize.height:          height
        mipmap:                     true
        fillMode:                   Image.PreserveAspectFit
        source:                     "/res/clockwise-arrow.svg"
        anchors.right:              parent.right
        anchors.rightMargin:        ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter:     parent.verticalCenter
    }

    QGCColoredImage {
        color:                      _fgColor
        visible:                    yAxisPositiveRangeOnly
        height:                     ScreenTools.defaultFontPixelHeight
        width:                      height
        sourceSize.height:          height
        mipmap:                     true
        fillMode:                   Image.PreserveAspectFit
        source:                     "/res/counter-clockwise-arrow.svg"
        anchors.left:               parent.left
        anchors.leftMargin:         ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter:     parent.verticalCenter
    }

    QGCColoredImage {
        color:                      _fgColor
        visible:                    yAxisPositiveRangeOnly
        height:                     ScreenTools.defaultFontPixelHeight
        width:                      height
        sourceSize.height:          height
        mipmap:                     true
        fillMode:                   Image.PreserveAspectFit
        source:                     "/res/chevron-up.svg"
        anchors.top:                parent.top
        anchors.topMargin:          ScreenTools.defaultFontPixelWidth
        anchors.horizontalCenter:   parent.horizontalCenter
    }

    QGCColoredImage {
        color:                      _fgColor
        visible:                    yAxisPositiveRangeOnly
        height:                     ScreenTools.defaultFontPixelHeight
        width:                      height
        sourceSize.height:          height
        mipmap:                     true
        fillMode:                   Image.PreserveAspectFit
        source:                     "/res/chevron-down.svg"
        anchors.bottom:             parent.bottom
        anchors.bottomMargin:       ScreenTools.defaultFontPixelWidth
        anchors.horizontalCenter:   parent.horizontalCenter
    }

    Rectangle {
        width:          _hatWidth
        height:         _hatWidth
        radius:         _hatWidthHalf
        border.color:   _fgColor
        border.width:   1
        color:          Qt.rgba(_fgColor.r, _fgColor.g, _fgColor.b, 0.5)
        x:              stickPositionX - _hatWidthHalf
        y:              stickPositionY - _hatWidthHalf
    }

    Connections {
        target: touchPoint

        onXChanged: {
            if (_processTouchPoints) {
                _joyRoot.stickPositionX = Math.max(Math.min(touchPoint.x, _joyRoot.width), 0)
            }
        }
        onYChanged: {
            if (_processTouchPoints) {
                _joyRoot.stickPositionY = Math.max(Math.min(touchPoint.y, _joyRoot.height), 0)
            }
        }
    }

    MultiPointTouchArea {
        anchors.fill:           parent
        anchors.bottomMargin:   yAxisReCenter ? 0 : -_hatWidthHalf
        minimumTouchPoints:     1
        maximumTouchPoints:     1
        touchPoints:            [ TouchPoint { id: touchPoint } ]
        onPressed:              _joyRoot.thumbDown(touchPoints)
        onReleased:             _joyRoot.reCenter()
    }
}
