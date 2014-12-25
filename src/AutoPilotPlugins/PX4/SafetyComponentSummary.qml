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

        Text { id: rtlMinAlt; text: "RTL min alt:" }
        Text {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rtlMinAlt.contentWidth;
            text: autopilot.parameters["RTL_RETURN_ALT"].valueString
        }
    }

    Row {
        width: parent.width

        Text { id: rtlHomeAlt; text: "RTL home alt:" }
        Text {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rtlHomeAlt.contentWidth;
            text: autopilot.parameters["RTL_DESCEND_ALT"].valueString
        }
    }

    Row {
        width: parent.width

        Text { id: rtlLoiter; text: "RTL loiter delay:" }
        Text {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rtlLoiter.contentWidth;
            text: autopilot.parameters["RTL_LAND_DELAY"].value < 0 ? "Disabled" : autopilot.parameters["RTL_LAND_DELAY"].valueString
        }
    }

    Row {
        width: parent.width

        Text { id: commLoss; text: "Telemetry loss RTL:" }
        Text {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - commLoss.contentWidth;
            text: autopilot.parameters["COM_DL_LOSS_EN"].value != 1 ? "Disabled" : autopilot.parameters["NAV_DLL_N"].valueString
        }
    }

    Row {
        width: parent.width

        Text { id: rcLoss; text: "RC loss RTL (seconds):" }
        Text {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rcLoss.contentWidth;
            text: autopilot.parameters["COM_RC_LOSS_T"].valueString
        }
    }
}
