import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// A USB camera setting set with a slider, over the range the camera offers, with the value shown beside it.
RowLayout {
    id: root

    required property Fact setting
    property real from: root.setting.min
    property real to: root.setting.max
    property real stepSize: 0.1
    property string valueText: root.setting.valueString + (root.setting.units ? " " + root.setting.units : "")

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        text:               root.setting.label
        Layout.fillWidth:   true
    }

    QGCSlider {
        id: slider

        from:                   root.from
        to:                     root.to
        stepSize:               root.stepSize
        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 20
        onMoved:                root.setting.rawValue = slider.value

        Binding on value {
            value:          root.setting.rawValue
            when:           !slider.pressed
            restoreMode:    Binding.RestoreNone
        }
    }

    QGCLabel {
        text:                   root.valueText
        horizontalAlignment:    Text.AlignRight
        Layout.minimumWidth:    ScreenTools.defaultFontPixelWidth * 7
    }
}
