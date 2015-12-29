import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Row {
    spacing: ScreenTools.defaultFontPixelWidth

    property Fact fact: Fact { }

    Repeater {
        model: fact.bitmaskStrings

        QGCCheckBox {
            text:       modelData
            checked:    fact.value & fact.bitmaskValues[index]

            onClicked: {
                if (checked) {
                    fact.value |= fact.bitmaskValues[index]
                } else {
                    fact.value &= ~fact.bitmaskValues[index]
                }
            }
        }
    }
}
