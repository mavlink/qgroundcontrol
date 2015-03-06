import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Controls 1.0

/*
    IMPORTANT NOTE: Any changes made here must also be made to SensorsComponentSummaryFixedWing.qml
*/

Column {
    anchors.fill: parent
    anchors.margins: 8

    Component {
        id: component

        Row {
            width: parent.width

            QGCLabel { id: label; text: labelText }
            QGCLabel {
                property Fact fact:     Fact { name: factName }
                horizontalAlignment:    Text.AlignRight;
                width:                  parent.width - label.contentWidth;
                text:                   fact.value  == 0 ? "Setup required" : "Ready"
            }
        }
    }

    Loader {
        property string labelText: "Compass:"
        property string factName: "CAL_MAG0_ID"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Gyro:"
        property string factName: "CAL_GYRO0_ID"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Accelerometer:"
        property string factName: "CAL_ACC0_ID"
        width: parent.width
        sourceComponent: component
    }
}
