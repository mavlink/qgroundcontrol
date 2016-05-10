import QtQuick                  2.5
import QtQuick.Controls         1.4

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

/// The SliderSwitch control implements a sliding switch control similar to the power off
/// control on an iPhone.
Rectangle {
    id:     _root
    width:  label.contentWidth + (_diameter * 2.5) + (_border * 4)
    radius: height /2
    color:  qgcPal.window

    signal accept   ///< Action confirmed
    signal reject   ///< Action rejected

    property string confirmText ///< Text for slider

    property real _border: 4
    property real _diameter: height - (_border * 2)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCLabel {
        id: label
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.verticalCenter:     parent.verticalCenter
        text:   qsTr("Slide to %1").arg(confirmText)
    }

    Rectangle {
        id:         slider
        x:          _border
        y:          _border
        height:     _diameter
        width:      _diameter
        radius:     _diameter / 2
        color:      qgcPal.windowShade
        opacity:    0.8

        QGCColoredImage {
            anchors.centerIn:       parent
            width:                  parent.width  * 0.8
            height:                 parent.height * 0.8
            sourceSize.height:      height
            fillMode:               Image.PreserveAspectFit
            smooth:                 false
            mipmap:                 false
            color:                  qgcPal.text
            cache:                  false
            source:                 "/res/ArrowRight.svg"
        }

        MouseArea {
            id:             sliderDragArea
            anchors.fill:   parent
            drag.target:    slider
            drag.axis:      Drag.XAxis
            drag.minimumX:  _border
            drag.maximumX:  _maxXDrag

            property real _maxXDrag:    _root.width - (_diameter + _border)
            property bool dragActive:   drag.active

            onDragActiveChanged: {
                if (!sliderDragArea.drag.active) {
                    if (slider.x > _maxXDrag - _border) {
                        _root.accept()
                    }
                    slider.x = _border
                }
            }
        }
    }
}
