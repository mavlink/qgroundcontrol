import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Generic increment/decrement control for a numeric Fact.
/// Shows a label, a "-" button, the current value with its units,
/// and a "+" button.  Clamped to fact.min / fact.max.
///
/// Properties:
///   fact    - The Fact to control (required).
///   label   - Display label (defaults to fact.label).
///   step    - Amount added/subtracted per click (default 5).
RowLayout {
    id:      root

    property string label:      fact.label
    property Fact   fact
    property real   step:       5

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        Layout.fillWidth:    true
        Layout.minimumWidth: implicitWidth
        text:                root.label
    }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth * 2

        QGCButton {
            Layout.preferredWidth:  height
            height:                 valueLabel.height * 1.5
            text:                   "-"
            onClicked: {
                if (root.fact.value > root.fact.min) {
                    root.fact.value = root.fact.value - root.step
                }
            }
        }

        QGCLabel {
            id:     valueLabel
            width:  ScreenTools.defaultFontPixelWidth * 6
            text:   root.fact.valueString
        }

        QGCButton {
            Layout.preferredWidth:  height
            height:                 valueLabel.height * 1.5
            text:                   "+"
            onClicked: {
                if (root.fact.value < root.fact.max) {
                    root.fact.value = root.fact.value + root.step
                }
            }
        }
    }
}
