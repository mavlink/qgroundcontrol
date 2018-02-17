import QtQuick                  2.3
import QtQuick.Controls         1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id:             _joyRoot

    property alias  lightColors:    mapPal.lightColors  ///< true: use light colors from QGCMapPalette for drawing
    property real   xAxis:          0                   ///< Value range [-1,1], negative values left stick, positive values right stick
    property real   yAxis:          0                   ///< Value range [-1,1], negative values up stick, positive values down stick
    property bool   yAxisThrottle:  false               ///< true: yAxis used for throttle, range [1,0], positive value are stick up
    property real   xPositionDelta: 0                   ///< Amount to move the control on x axis
    property real   yPositionDelta: 0                   ///< Amount to move the control on y axis
    property bool   gimbalMode:     false               ///< true: Gimbal Control Mode false: Joystick Mode
    property real   stickPositionX:         _centerXY
    property real   stickPositionY:         yAxisThrottle ? height : _centerXY

    property real   _centerXY:              width * 0.5
    property bool   _processTouchPoints:    false
    property bool   _stickCenteredOnce:     false
    property bool   _zoomControl:           yAxisThrottle ? gimbalMode : false
    property real   _radius:                ScreenTools.defaultFontPixelWidth * 0.25

    readonly property real _hatWidth:       ScreenTools.defaultFontPixelHeight
    readonly property real _hatWidthHalf:   ScreenTools.defaultFontPixelHeight / 2

    QGCMapPalette { id: mapPal }

    onStickPositionXChanged: {
        if(!_zoomControl) {
            var xAxisTemp = stickPositionX / width
            xAxisTemp *= 2.0
            xAxisTemp -= 1.0
            xAxis = xAxisTemp
        }
    }

    onStickPositionYChanged: {
        var yAxisTemp = stickPositionY / height
        yAxisTemp *= 2.0
        yAxisTemp -= 1.0
        if (yAxisThrottle) {
            yAxisTemp = ((yAxisTemp * -1.0) / 2.0) + 0.5
        }
        yAxis = yAxisTemp
    }

    function reCenter()
    {
        _processTouchPoints = false
        // Move control back to original position
        xPositionDelta = 0
        yPositionDelta = 0
        // Center sticks
        stickPositionX = _centerXY
        if (!yAxisThrottle) {
            stickPositionY = _centerXY
        }
    }

    function thumbDown(touchPoints)
    {
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
        visible:            !_zoomControl
    }

    Rectangle {
        anchors.fill:       parent
        anchors.margins:    parent.width / 4
        radius:             width * 0.5
        border.color:       mapPal.thumbJoystick
        border.width:       2
        color:              Qt.rgba(0,0,0,0)
        visible:            !_zoomControl
    }

    Rectangle {
        id:                 outerRing
        anchors.fill:       _zoomControl ? undefined : parent
        height:             _zoomControl ? parent.height : undefined
        width:              _zoomControl ? parent.height * 0.25 : undefined
        anchors.horizontalCenter: _zoomControl ? parent.horizontalCenter : undefined
        radius:             _zoomControl ? _radius : width * 0.5
        border.color:       mapPal.thumbJoystick
        border.width:       2
        color:              _zoomControl ? Qt.rgba(0,0,0,0.25) : Qt.rgba(0,0,0,0)
    }

    Rectangle {
        visible:            _zoomControl
        width:              ScreenTools.defaultFontPixelWidth * 8
        height:             ScreenTools.defaultFontPixelHeight
        radius:             _radius
        anchors.bottom:     outerRing.top
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.25
        anchors.horizontalCenter: outerRing.horizontalCenter
        color:              Qt.rgba(0,0,0,0.5)
        QGCLabel {
            text:               qsTr("ZOOM MAX")
            color:              "white"
            font.pointSize:     ScreenTools.smallFontPointSize
            anchors.centerIn:   parent
        }
    }

    Rectangle {
        visible:            _zoomControl
        width:              ScreenTools.defaultFontPixelWidth * 8
        height:             ScreenTools.defaultFontPixelHeight
        radius:             _radius
        anchors.top:        outerRing.bottom
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.25
        anchors.horizontalCenter: outerRing.horizontalCenter
        color:              Qt.rgba(0,0,0,0.5)
        QGCLabel {
            text:               qsTr("ZOOM MIN")
            color:              "white"
            font.pointSize:     ScreenTools.smallFontPointSize
            anchors.centerIn:   parent
        }
    }

    Rectangle {
        width:              _hatWidth
        height:             _hatWidth
        radius:             _hatWidthHalf
        color:              mapPal.thumbJoystick
        x:                  stickPositionX - _hatWidthHalf
        y:                  stickPositionY - _hatWidthHalf
    }

    Connections {
        target: touchPoint
        onXChanged: {
            if (_processTouchPoints && !_zoomControl) {
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
        onReleased:         _joyRoot.reCenter()
    }
}
