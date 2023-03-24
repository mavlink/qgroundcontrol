import QtQuick                  2.15
import QtQuick.Controls         2.15

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

/// The SliderSwitch control implements a sliding switch control similar to the power off
/// control on an iPhone. It supports holding the space bar to slide the switch.
Rectangle {
    id:             _root
    implicitWidth:  label.contentWidth + (_diameter * 2.5) + (_border * 4)
    implicitHeight: label.height * 2.5
    radius:         height /2
    color:          qgcPal.windowShade

    signal accept   ///< Action confirmed

    property string confirmText                         ///< Text for slider
    property alias  fontPointSize: label.font.pointSize ///< Point size for text

    property real _border:                      4   
    property real _diameter:                    height - (_border * 2)
    property real _dragStartX:                  _border
    property real _dragStopX:                   _root.width - (_diameter + _border)
    property bool _waitingForLastAutoRepeat:    false

    Keys.onSpacePressed: {
        if (visible && event.modifiers === Qt.NoModifier && event.isAutoRepeat && !sliderDragArea.drag.active) {
            event.accepted = true
            if (_waitingForLastAutoRepeat) {
                resetSpaceBarSliding()
                accept()
            } else {
                sliderAnimation.start()
                spaceBarTimout.restart()
            }
        }
    }

    function resetSpaceBarSliding() {
        _waitingForLastAutoRepeat = false
        spaceBarTimout.stop()
        slider.reset()
    }

    Timer {
        id:             spaceBarTimout
        interval:       200
        repeat:         false
        onTriggered:    _root.resetSpaceBarSliding()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCLabel {
        id:                         label
        x:                          _diameter + _border
        width:                      parent.width - x
        anchors.verticalCenter:     parent.verticalCenter
        horizontalAlignment:        Text.AlignHCenter
        text:                       confirmText
        color:                      qgcPal.buttonText
    }

    Rectangle {
        id:         slider
        x:          _border
        y:          _border
        height:     _diameter
        width:      _diameter
        radius:     _diameter / 2
        color:      qgcPal.primaryButton

        QGCColoredImage {
            anchors.centerIn:       parent
            width:                  parent.width  * 0.8
            height:                 parent.height * 0.8
            sourceSize.height:      height
            fillMode:               Image.PreserveAspectFit
            smooth:                 false
            mipmap:                 false
            color:                  qgcPal.buttonText
            cache:                  false
            source:                 "/res/ArrowRight.svg"
        }

        PropertyAnimation on x {
            id:         sliderAnimation
            duration:   1500
            from:       _dragStartX
            to:         _dragStopX
            running:    false
            onFinished: _waitingForLastAutoRepeat = true
        }

        function reset() {
            slider.x = _border
            sliderAnimation.stop()
        }
    }

    QGCMouseArea {
        id:                 sliderDragArea
        anchors.leftMargin: -ScreenTools.defaultFontPixelWidth * 15
        fillItem:           slider
        drag.target:        slider
        drag.axis:          Drag.XAxis
        drag.minimumX:      _dragStartX
        drag.maximumX:      _dragStopX
        preventStealing:    true

        property bool dragActive: drag.active

        onDragActiveChanged: {
            if (!sliderDragArea.drag.active) {
                if (slider.x > _dragStopX - _border) {
                    _root.accept()
                }
                slider.reset()
            }
        }
    }
}
