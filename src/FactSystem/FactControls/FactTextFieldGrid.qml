import QtQuick          2.7
import QtQuick.Layouts  1.3

import QGroundControl.FactSystem    1.0
import QGroundControl.Controls      1.0

GridLayout {
    property var factList   ///< List of Facts to show

    rows: factList.length
    flow: GridLayout.TopToBottom

    Repeater {
        model: parent.factList

        QGCLabel { text: modelData.name + ":" }
    }

    Repeater {
        model: parent.factList

        FactTextField {
            Layout.fillWidth:   true
            fact:               modelData
        }
    }
}
