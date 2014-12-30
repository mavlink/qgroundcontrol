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

        Text { id: mode; text: "Mode switch:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - mode.contentWidth
            text: autopilot.parameters["RC_MAP_MODE_SW"].value == 0 ? "Setup required" : autopilot.parameters["RC_MAP_MODE_SW"].value
        }
    }

    Row {
        width: parent.width

        Text { id: posctl; text: "Position Ctl switch:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - posctl.contentWidth
            text: autopilot.parameters["RC_MAP_POSCTL_SW"].value == 0 ? "Not mapped" : autopilot.parameters["RC_MAP_POSCTL_SW"].value
        }
    }

    Row {
        width: parent.width

        Text { id: loiter; text: "Loiter switch:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - loiter.contentWidth
            text: autopilot.parameters["RC_MAP_LOITER_SW"].value == 0 ? "Not mapped" : autopilot.parameters["RC_MAP_LOITER_SW"].value
        }
    }

    Row {
        width: parent.width

        Text { id: rtl; text: "Return switch:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - rtl.contentWidth
            text: autopilot.parameters["RC_MAP_RETURN_SW"].value == 0 ? "Not mapped" : autopilot.parameters["RC_MAP_RETURN_SW"].value
        }
    }
}
