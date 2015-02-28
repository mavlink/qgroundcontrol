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

    Row {
        width: parent.width

        QGCLabel { id: compass; text: "Compass:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - compass.contentWidth;
            text: autopilot.parameters["CAL_MAG0_ID"].value  == 0 ? "Setup required" : "Ready"
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: gyro; text: "Gyro:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - gyro.contentWidth;
            text: autopilot.parameters["CAL_GYRO0_ID"].value  == 0 ? "Setup required" : "Ready"
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: accel; text: "Accelerometer:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - accel.contentWidth;
            text: autopilot.parameters["CAL_ACC0_ID"].value  == 0 ? "Setup required" : "Ready"
        }
    }
}
