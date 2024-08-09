import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Palette
import QGroundControl.ScreenTools

Item {
    id:             _joyRoot

    property alias  lightColors:            mapPal.lightColors  ///< true: use light colors from QGCMapPalette for drawing
    property real   xAxis:                  0                   ///< Value range [-1,1], negative values left stick, positive values right stick
    property real   yAxis:                  0                   ///< Value range [-1,1], negative values down stick, positive values up stick
    property bool   yAxisPositiveRangeOnly: false               ///< true: value range [0,1], false: value range [-1,1]
    property bool   yAxisReCenter:          false               ///< true: snaps back to center on release, false: stays at current position on release
    property real   xPositionDelta:         0                   ///< Amount to move the control on x axis
    property real   yPositionDelta:         0                   ///< Amount to move the control on y axis

    property real   _centerXY:              width / 2
    property bool   _processTouchPoints:    false
    property color  _fgColor:               QGroundControl.globalPalette.text
    property color  _bgColor:               QGroundControl.globalPalette.window
    property real   _hatWidth:              ScreenTools.defaultFontPixelHeight
    property real   _hatWidthHalf:          _hatWidth / 2
    property bool   calculateYAxisMutex:    true
    property real   stickPositionX:         _centerXY
    property real   stickPositionY:         !yAxisReCenter ? height : height / 2
    property bool   alredyCreated:          false
    
    QGCMapPalette { id: mapPal }

    onStickPositionXChanged:            calculateXAxis()
    onStickPositionYChanged:            calculateYAxis()
    onYAxisPositiveRangeOnlyChanged:    calculateYAxis()
    onYAxisReCenterChanged:             yAxisReCentered()     
    
    function yAxisReCentered() {
        if( yAxisReCenter ) {
            yAxis = yAxisPositiveRangeOnly ? 0.5 : 0
            stickPositionY = _joyRoot.height / 2
        }
        if( !alredyCreated && !yAxisReCenter ) {
            yAxis = yAxisPositiveRangeOnly ? 0 : -1
            stickPositionY = _joyRoot.height            
        }
        if ( alredyCreated && !yAxisReCenter ){
            yAxis = yAxisPositiveRangeOnly ? 0.5 : 0
            stickPositionY = _joyRoot.height / 2
        }
        alredyCreated = true
        return yAxis
    }

    //We prevent Joystick to move while the screen is resizing 
    function resize( yPositionAfterResize ) {
        if(_joyRoot.height <= 0) {
            return;
        }
        calculateYAxisMutex = false
        stickPositionY = ( 1 - ( ( yPositionAfterResize + ( !yAxisPositiveRangeOnly ? 1 : 0) ) /  ( yAxisPositiveRangeOnly ? 1 : 2 ) )) * _joyRoot.height // Reverse the CalculateYAxis Procedure
        stickPositionX = _joyRoot.width / 2 // Manual recenter
        calculateYAxisMutex = true
    }

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
        if(!calculateYAxisMutex) {
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
        _centerXY = _joyRoot.width / 2 // Reload before using it to make sure of using the right value
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
        _centerXY = _joyRoot.width / 2  // make sure to know the correct center of the item

        var limitOffset = uiRealX >= _joyRoot.width / 2 ? true : false // as the joystick become small the UI too so we limit the maxOffset for reCentering joystick to prevent misclicks
        var maxDelta = _joyRoot.x > uiTotalWidth / 2  ? uiTotalWidth - uiRealX - _joyRoot.x - _centerXY : uiRealX
        var isRightJoystick = _joyRoot.x > uiTotalWidth / 2 ? true : false

        // Check if new xDelta will make joystick to be beyond screen boundaries or can cause a misclick
        if (!limitOffset && isRightJoystick && touchPoints[0].x  <= maxDelta || !limitOffset && !isRightJoystick && touchPoints[0].x >= maxDelta) {
            xPositionDelta = touchPoints[0].x - _centerXY
        } else if (limitOffset && !isRightJoystick && touchPoints[0].x >= _centerXY * 0.25 && touchPoints[0].x <= _centerXY * 2) { // more offset at the side near to the center
            xPositionDelta = touchPoints[0].x - _centerXY
        } else if (limitOffset && isRightJoystick && touchPoints[0].x >= 0 && touchPoints[0].x <= _centerXY * 1.75) {
            xPositionDelta = touchPoints[0].x - _centerXY
        } else {
            return;
        }

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
        onPressed:              touchPoints => _joyRoot.thumbDown(touchPoints)
        onReleased:             _joyRoot.reCenter()
    }
}
