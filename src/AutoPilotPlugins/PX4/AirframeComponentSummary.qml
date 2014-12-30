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

        Text { id: systemId; text: "System ID:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - systemId.contentWidth
            text: autopilot.parameters["MAV_SYS_ID"].value
        }
    }

    Row {
        width: parent.width

        Text { id: airframe; text: "Airframe:" }
        Text {
            horizontalAlignment: Text.AlignRight
            width: parent.width - airframe.contentWidth
            text: autopilot.parameters["SYS_AUTOSTART"].value == 0 ? "Setup required" : autopilot.parameters["SYS_AUTOSTART"].value
        }
    }
}
