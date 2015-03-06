import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Controls 1.0

/*
    IMPORTANT NOTE: Any changes made here must also be made to SensorsComponentSummary.qml
*/

Column {
    anchors.fill: parent
    anchors.margins: 8

    Row {
        width: parent.width

        QGCLabel { id: compass; text: "Compass:" }
        QGCLabel {
            property Fact fact:     Fact { name: "CAL_MAG0_ID" }
            horizontalAlignment:    Text.AlignRight;
            width:                  parent.width - compass.contentWidth;
            text:                   fact.value  == 0 ? "Setup required" : "Ready"
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: gyro; text: "Gyro:" }
        QGCLabel {
            property Fact fact:     Fact { name: "CAL_GYRO0_ID" }
            horizontalAlignment:    Text.AlignRight;
            width:                  parent.width - compass.contentWidth;
            text:                   fact.value  == 0 ? "Setup required" : "Ready"
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: accel; text: "Accelerometer:" }
        QGCLabel {
            property Fact fact:     Fact { name: "CAL_ACC0_ID" }
            horizontalAlignment:    Text.AlignRight;
            width:                  parent.width - compass.contentWidth;
            text:                   fact.value  == 0 ? "Setup required" : "Ready"
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: airspeed; text: "Airspeed:" }
        QGCLabel {
            property Fact fact:     Fact { name: "SENS_DPRES_OFF" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - airspeed.contentWidth;
            text: fact.value == 0.0 ? "Setup required" : "Ready"
        }
    }
}
