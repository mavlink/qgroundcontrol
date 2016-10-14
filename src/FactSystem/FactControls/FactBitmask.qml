import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Flow {
    spacing: ScreenTools.defaultFontPixelWidth

    /// true: Checking the first entry will clear and disable all other entries
    property bool firstEntryIsAll: false

    property Fact fact: Fact { }

    Component.onCompleted: {
        if (firstEntryIsAll && repeater.itemAt(0).checked) {
            for (var i=1; i<repeater.count; i++) {
                var otherCheckbox = repeater.itemAt(i)
                otherCheckbox.enabled = false
            }
        }
    }

    Repeater {
        id:     repeater
        model:  fact.bitmaskStrings

        QGCCheckBox {
            id:         checkbox
            text:       modelData
            checked:    fact.value & fact.bitmaskValues[index]

            onClicked: {
                if (checked) {
                    if (firstEntryIsAll && index == 0) {
                        for (var i=1; i<repeater.count; i++) {
                            var otherCheckbox = repeater.itemAt(i)
                            fact.value &= ~fact.bitmaskValues[i]
                            otherCheckbox.checked = false
                            otherCheckbox.enabled = false
                        }
                    }
                    fact.value |= fact.bitmaskValues[index]
                } else {
                    if (firstEntryIsAll && index == 0) {
                        for (var i=1; i<repeater.count; i++) {
                            var otherCheckbox = repeater.itemAt(i)
                            otherCheckbox.enabled = true
                        }
                    }
                    fact.value &= ~fact.bitmaskValues[index]
                }
            }
        }
    }
}
