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

        Text { id: roll; text: "Roll:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - roll.contentWidth
            text: autopilot.parameters["RC_MAP_ROLL"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_ROLL"].value
        }
    }

    Row {
        width: parent.width

        Text { id: pitch; text: "Pitch:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - pitch.contentWidth
            text: autopilot.parameters["RC_MAP_PITCH"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_PITCH"].value
        }
    }

    Row {
        width: parent.width

        Text { id: yaw; text: "Yaw:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - yaw.contentWidth
            text: autopilot.parameters["RC_MAP_YAW"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_YAW"].value
        }
    }

    Row {
        width: parent.width

        Text { id: throttle; text: "Throttle:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - throttle.contentWidth
            text: autopilot.parameters["RC_MAP_THROTTLE"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_THROTTLE"].value
        }
    }

    Row {
        width: parent.width

        Text { id: mode; text: "Mode switch:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - mode.contentWidth
            text: autopilot.parameters["RC_MAP_MODE_SW"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_MODE_SW"].value
        }
    }
}
