import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

Column {
    anchors.fill: parent
    anchors.margins: 8

    Row {
        width: parent.width

        Text { id: compass; text: "Compass:" }
        Text {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - compass.contentWidth;
            property bool setupRequiredValue: autopilot.parameters["SENS_MAG_XOFF"] ? autopilot.parameters["SENS_MAG_XOFF"].value : autopilot.parameters["CAL_MAG0_ID"].value
            text: setupRequiredValue  == 0 ? "Setup required" : "Ready"
        }
    }

    Row {
        width: parent.width

        Text { id: gyro; text: "Gyro:" }
        Text {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - gyro.contentWidth;
            property bool setupRequiredValue: autopilot.parameters["SENS_GYRO_XOFF"] ? autopilot.parameters["SENS_GYRO_XOFF"].value : autopilot.parameters["CAL_GYRO0_ID"].value
            text: setupRequiredValue  == 0 ? "Setup required" : "Ready"
        }
    }

    Row {
        width: parent.width

        Text { id: accel; text: "Accelerometer:" }
        Text {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - accel.contentWidth;
            property bool setupRequiredValue: autopilot.parameters["SENS_ACC_XOFF"] ? autopilot.parameters["SENS_ACC_XOFF"].value : autopilot.parameters["CAL_ACC0_ID"].value
            text: setupRequiredValue  == 0 ? "Setup required" : "Ready"
        }
    }

    Row {
        width: parent.width

        Text { id: airspeed; text: "Airspeed:" }
        Text {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - airspeed.contentWidth;
            text: autopilot.parameters["SENS_DPRES_OFF"].value == 0.0 ? "Setup required" : "Ready"
        }
    }
}
