import QtQuick                  2.5
import QtQuick.Controls         1.4

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

/// The SliderSwitch control implements a sliding switch control similar to the power off
/// control on an iPhone.
Rectangle {
    id:     _root
    width:  label.contentWidth + (_diameter * 2) + (_border * 4)
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
        text:   "Slide to " + confirmText
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

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            anchors.verticalCenter:     parent.verticalCenter
            text: ">"
        }

        MouseArea {
            id:             sliderDragArea
            anchors.fill:   parent
            onClicked:      _root.accept()
            drag.target:    slider
            drag.axis:      Drag.XAxis
            drag.minimumX:  _border
            drag.maximumX:  _maxXDrag

            property real _maxXDrag:    _root.width - ((_diameter + _border) * 2)
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

    Rectangle {
        id:                     cancel
        anchors.rightMargin:    _border
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        height:                 _diameter
        width:                  _diameter
        radius:                 _diameter / 2
        color:                  qgcPal.windowShade
        opacity:                0.8

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            anchors.verticalCenter:     parent.verticalCenter
            text: "X"
        }

        MouseArea {
            anchors.fill:   parent
            onClicked:      _root.reject()
        }
    }
}
