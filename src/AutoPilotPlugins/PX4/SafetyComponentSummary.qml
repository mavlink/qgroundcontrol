import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0

Column {
    anchors.fill: parent
    anchors.margins: 8

    Row {
        width: parent.width

        QGCLabel { id: rtlMinAlt; text: "RTL min alt:" }
        FactLabel {
            fact: Fact { name: "RTL_RETURN_ALT" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rtlMinAlt.contentWidth;
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: rtlHomeAlt; text: "RTL home alt:" }
        FactLabel {
            fact: Fact { name: "RTL_DESCEND_ALT" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rtlHomeAlt.contentWidth;
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: rtlLoiter; text: "RTL loiter delay:" }
        QGCLabel {
            property Fact fact: Fact { name: "RTL_LAND_DELAY" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rtlLoiter.contentWidth;
            text: fact.value < 0 ? "Disabled" : fact.valueString
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: commLoss; text: "Telemetry loss RTL:" }
        QGCLabel {
            property Fact fact: Fact { name: "COM_DL_LOSS_EN" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - commLoss.contentWidth;
            text: fact.value != 1 ? "Disabled" : fact.valueString
        }
    }

    Row {
        width: parent.width

        QGCLabel { id: rcLoss; text: "RC loss RTL (seconds):" }
        FactLabel {
            fact: Fact { name: "COM_RC_LOSS_T" }
            horizontalAlignment: Text.AlignRight;
            width: parent.width - rcLoss.contentWidth;
        }
    }
}
