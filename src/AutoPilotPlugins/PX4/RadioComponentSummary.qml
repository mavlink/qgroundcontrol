import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Controls 1.0

Column {
    anchors.fill: parent
    anchors.margins: 8

    Row {
        width: parent.width

        QGCLabel { id: roll; text: "Roll:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight
            width: parent.width - roll.contentWidth
            text: autopilot.parameters["RC_MAP_ROLL"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_ROLL"].value
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: pitch; text: "Pitch:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight
            width: parent.width - pitch.contentWidth
            text: autopilot.parameters["RC_MAP_PITCH"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_PITCH"].value
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: yaw; text: "Yaw:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight
            width: parent.width - yaw.contentWidth
            text: autopilot.parameters["RC_MAP_YAW"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_YAW"].value
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: throttle; text: "Throttle:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight
            width: parent.width - throttle.contentWidth
            text: autopilot.parameters["RC_MAP_THROTTLE"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_THROTTLE"].value
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: mode; text: "Mode switch:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight
            width: parent.width - mode.contentWidth
            text: autopilot.parameters["RC_MAP_MODE_SW"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_MODE_SW"].value
        }
    }
}
