import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    property alias label:                   label.text
    property alias from:                    slider.from
    property alias to:                      slider.to
    property real  sliderPreferredWidth:    -1

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        id:                 label
        Layout.fillWidth:   true
    }

    QGCSlider {
        id:                     slider
        Layout.preferredWidth:  sliderPreferredWidth
    }
}
