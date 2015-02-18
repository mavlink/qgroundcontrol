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

        QGCLabel { id: rtlMinAlt; text: "RTL min alt:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rtlMinAlt.contentWidth;
            text: autopilot.parameters["RTL_RETURN_ALT"].valueString
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: rtlHomeAlt; text: "RTL home alt:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rtlHomeAlt.contentWidth;
            text: autopilot.parameters["RTL_DESCEND_ALT"].valueString
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: rtlLoiter; text: "RTL loiter delay:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rtlLoiter.contentWidth;
            text: autopilot.parameters["RTL_LAND_DELAY"].value < 0 ? "Disabled" : autopilot.parameters["RTL_LAND_DELAY"].valueString
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: commLoss; text: "Telemetry loss RTL:" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - commLoss.contentWidth;
            text: autopilot.parameters["COM_DL_LOSS_EN"].value != 1 ? "Disabled" : autopilot.parameters["COM_DL_LOSS_T"].valueString
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: rcLoss; text: "RC loss RTL (seconds):" }
        QGCLabel {
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rcLoss.contentWidth;
            text: autopilot.parameters["COM_RC_LOSS_T"].valueString
        }
    }
}
