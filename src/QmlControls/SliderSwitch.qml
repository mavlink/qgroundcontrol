import QtQuick                  2.3
import QtQuick.Controls         1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

/// The SliderSwitch control implements a sliding switch control similar to the power off
/// control on an iPhone.
Rectangle {
    id:             _root
    implicitWidth:  label.contentWidth + (_diameter * 2.5) + (_border * 4)
    implicitHeight: label.height * 2.5
    radius:         height /2
    color:          qgcPal.windowShade

    signal accept   ///< Action confirmed

    property string confirmText                         ///< Text for slider
    property alias  fontPointSize: label.font.pointSize ///< Point size for text

    property real _border: 4
    property real _diameter: height - (_border * 2)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCLabel {
        id:                         label
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.verticalCenter:     parent.verticalCenter
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

    }

    QGCMouseArea {
        id:                 sliderDragArea
        anchors.leftMargin: -ScreenTools.defaultFontPixelWidth * 15
        fillItem:           slider
        drag.target:        slider
        drag.axis:          Drag.XAxis
        drag.minimumX:      _border
        drag.maximumX:      _maxXDrag
        preventStealing:    true

        property real _maxXDrag:    _root.width - (_diameter + _border)
        property bool dragActive:   drag.active
        property real _dragOffset:  1

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
