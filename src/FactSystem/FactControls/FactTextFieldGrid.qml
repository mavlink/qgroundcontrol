import QtQuick          2.3
import QtQuick.Layouts  1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Controls      1.0

GridLayout {
    property var factList       ///< List of Facts to show
    property var factLabels     ///< Labels for facts, if not set, use Fact.name

    rows: factList.length
    flow: GridLayout.TopToBottom

    Repeater {
        model: parent.factList

        QGCLabel { text: factLabels ? factLabels[index] : modelData.name }
    }

    Repeater {
        model: parent.factList

        FactTextField {
            Layout.fillWidth:   true
            fact:               modelData
        }
    }
}
