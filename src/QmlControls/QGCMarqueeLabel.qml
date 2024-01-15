import QtQuick
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

Item {
    property color  color:          qgcPal.text
    property alias  contentHeight:  _measureText.contentHeight
    property font   font
    property string text:           ""
    property real   maxWidth:       _measureText.implicitWidth

    id:             _root
    clip:           true
    implicitWidth:  Math.min(_measureText.implicitWidth, maxWidth)
    implicitHeight: _measureText.implicitHeight
    font.pointSize: ScreenTools.defaultFontPointSize
    font.family:    ScreenTools.normalFontFamily

    property bool _scrollMarquee:       _measureText.implicitWidth > maxWidth
    property real _scrollWidth:         _measureText.implicitWidth + _measureBlanks.implicitWidth
    property int  _scrollDuration:      _root.text.length * 500
    property real _innerText1StartX:    0
    property real _innerText2StartX:    _scrollWidth
    property bool _componentCompleted:  false

    Component.onCompleted: {
        _componentCompleted = true
        if (_scrollMarquee) {
            _innerText1Animation.start()
            _innerText2Animation.start()
        }
    }

    on_ScrollWidthChanged: _recalcTimer.restart()    // Wait for update storm to settle out before recalcing

    Timer {
        id:         _recalcTimer
        interval:   100

        onTriggered: {
            if (!_componentCompleted) {
                return
            }
            _innerText1Animation.stop()
            _innerText2Animation.stop()
            _innerText1.startX  = _innerText1StartX
            _innerText2.startX  = _innerText2StartX
            _innerText1.x       = _innerText1.startX
            _innerText2.x       = _innerText2.startX
            if (_measureText.implicitWidth > maxWidth) {
                _innerText1Animation.start()
                _innerText2Animation.start()
            }
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Text {
        id:                     _innerText1
        anchors.verticalCenter: parent.verticalCenter
        font:                   _root.font
        color:                 _root.color
        antialiasing:           true
        text:                   _root.text

        property real startX: _innerText1StartX

        NumberAnimation on x {
            id:         _innerText1Animation
            from:       _innerText1.startX
            to:         _innerText1.startX -_scrollWidth
            duration:   _scrollDuration

            onFinished: {
                _innerText2Animation.stop()
                var text1StartX = _innerText1.startX
                var text2StartX = _innerText2.startX
                _innerText1.startX = text2StartX
                _innerText2.startX = text1StartX
                start()
                _innerText2Animation.start()
            }
        }
    }

    Text {
        id:                     _innerText2
        anchors.verticalCenter: parent.verticalCenter
        font:                   _root.font
        color:                 _root.color
        antialiasing:           true
        text:                   _root.text
        visible:                _scrollMarquee

        property real startX: _innerText2StartX

        NumberAnimation on x {
            id:         _innerText2Animation
            from:       _innerText2.startX
            to:         _innerText2.startX -_scrollWidth
            duration:   _scrollDuration
        }
    }

    Text {
        id:             _measureText
        font:           _root.font
        antialiasing:   true
        text:           _root.text
        visible:        false
    }

    Text {
        id:             _measureBlanks
        font:           _root.font
        antialiasing:   true
        text:           "  "
        visible:        false
    }
}
