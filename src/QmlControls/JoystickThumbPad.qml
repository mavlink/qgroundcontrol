import QtQuick                  2.3
import QtQuick.Controls         1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id:             _joyRoot

    property alias  lightColors:    mapPal.lightColors  ///< true: use light colors from QGCMapPalette for drawing
    property real   xAxis:          0                   ///< Value range [-1,1], negative values left stick, positive values right stick
    property real   yAxis:          0                   ///< Value range [-1,1], negative values up stick, positive values down stick
    property bool   yAxisThrottle:  false               ///< true: yAxis used for throttle, range [1,0], positive value are stick up
    property bool   yAxisThrottleCentered: false        ///< false: center yAxis in throttle for reverser and forward
    property real   xPositionDelta: 0                   ///< Amount to move the control on x axis
    property real   yPositionDelta: 0                   ///< Amount to move the control on y axis
    property bool   springYToCenter:true                ///< true: Spring Y to center on release

    property real   _centerXY:              width / 2
    property bool   _processTouchPoints:    false
    property real   stickPositionX:         _centerXY
    property real   stickPositionY:         yAxisThrottleCentered ? _centerXY : height

    QGCMapPalette { id: mapPal }

    onWidthChanged: calculateXAxis()
    onStickPositionXChanged: calculateXAxis()
    onHeightChanged: calculateYAxis()
    onStickPositionYChanged: calculateYAxis()

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
        var yAxisTemp = stickPositionY / height
        yAxisTemp *= 2.0
        yAxisTemp -= 1.0
        if (yAxisThrottle) {
            yAxisTemp = ((yAxisTemp * -1.0) / 2.0) + 0.5
        }
        yAxis = yAxisTemp
    }

    function reCenter() {
        _processTouchPoints = false

        // Move control back to original position
        xPositionDelta = 0
        yPositionDelta = 0

        // Center sticks
        stickPositionX = _centerXY
        if (yAxisThrottleCentered) {
            stickPositionY = _centerXY
        }
    }

    function thumbDown(touchPoints) {
        // Position the control around the initial thumb position
        xPositionDelta = touchPoints[0].x - _centerXY
        if (yAxisThrottle) {
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
        source:             lightColors ? "/res/JoystickBezel.png" : "/res/JoystickBezelLight.png"
        mipmap:             true
        smooth:             true
    }

    QGCColoredImage {
        color:                      lightColors ? "white" : "black"
        visible:                    yAxisThrottle
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
        color:                      lightColors ? "white" : "black"
        visible:                    yAxisThrottle
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
        color:                      lightColors ? "white" : "black"
        visible:                    yAxisThrottle
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
        color:                      lightColors ? "white" : "black"
        visible:                    yAxisThrottle
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
        anchors.margins:    parent.width / 4
        anchors.fill:       parent
        radius:             width / 2
        border.color:       mapPal.thumbJoystick
        border.width:       2
        color:              Qt.rgba(0,0,0,0)
    }

    Rectangle {
        anchors.fill:       parent
        radius:             width / 2
        border.color:       mapPal.thumbJoystick
        border.width:       2
        color:              Qt.rgba(0,0,0,0)
    }

    Rectangle {
        width:  hatWidth
        height: hatWidth
        radius: hatWidthHalf
        border.color: lightColors ? "white" : "black"
        border.width: 1
        color:  mapPal.thumbJoystick
        x:      stickPositionX - hatWidthHalf
        y:      stickPositionY - hatWidthHalf

        readonly property real hatWidth:        ScreenTools.defaultFontPixelHeight
        readonly property real hatWidthHalf:    ScreenTools.defaultFontPixelHeight / 2
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
        anchors.fill:       parent
        minimumTouchPoints: 1
        maximumTouchPoints: 1
        touchPoints:        [ TouchPoint { id: touchPoint } ]
        onPressed:          _joyRoot.thumbDown(touchPoints)
        onReleased: {
            if(springYToCenter)
                _joyRoot.reCenter()
        }
    }
}
