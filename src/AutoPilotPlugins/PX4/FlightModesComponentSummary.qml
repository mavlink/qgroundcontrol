import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Controls 1.0

Column {
    anchors.fill: parent
    anchors.margins: 8

    Component {
        id: component

        Row {
            width: parent.width

            QGCLabel { id: label; text: labelText }
            QGCLabel {
                property Fact fact: Fact { name: factName }
                horizontalAlignment: Text.AlignRight
                width: parent.width - label.contentWidth
                text: fact.value == 0 ? zeroText : fact.value
            }
        }
    }

    Loader {
        property string labelText: "Mode switch:"
        property string zeroText: "Setup required"
        property string factName: "RC_MAP_MODE_SW"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Position Ctl switch:"
        property string zeroText: "Disabled"
        property string factName: "RC_MAP_POSCTL_SW"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Position Ctl switch:"
        property string zeroText: "Disabled"
        property string factName: "RC_MAP_LOITER_SW"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Return switch:"
        property string zeroText: "Disabled"
        property string factName: "RC_MAP_RETURN_SW"
        width: parent.width
        sourceComponent: component
    }
}
