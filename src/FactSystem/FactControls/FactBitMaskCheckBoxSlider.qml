import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Toggle slider bound to a single bit within a Fact's bitmask rawValue.
/// The slider stays in sync with the Fact and sets/clears the specified bit on click.
///
/// Example:
///     FactBitMaskCheckBoxSlider {
///         text:    qsTr("Continue in Auto mode")
///         fact:    _fsOptions
///         bitMask: 2   // bit 1
///     }
QGCCheckBoxSlider {
    id: control

    property Fact   fact:    Fact { }
    property int    bitMask: 0

    checked: fact && (fact.rawValue & bitMask)

    Connections {
        target: fact
        function onRawValueChanged() { control.checked = fact.rawValue & control.bitMask }
    }

    onClicked: {
        if (checked) {
            fact.rawValue |= bitMask
        } else {
            fact.rawValue &= ~bitMask
        }
    }
}
