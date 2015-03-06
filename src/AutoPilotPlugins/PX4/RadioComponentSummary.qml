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
                Fact { id: fact; name: factName }
                horizontalAlignment: Text.AlignRight
                width: parent.width - label.contentWidth
                text: fact.value == 0 ? zeroText : fact.value
            }
        }
    }

    Loader {
        property string labelText: "Roll:"
        property string zeroText: "Setup required"
        property string factName: "RC_MAP_ROLL"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Pitch:"
        property string zeroText: "Setup required"
        property string factName: "RC_MAP_PITCH"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Yaw:"
        property string zeroText: "Setup required"
        property string factName: "RC_MAP_YAW"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Throttle:"
        property string zeroText: "Setup required"
        property string factName: "RC_MAP_THROTTLE"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Flaps:"
        property string zeroText: "Disabled"
        property string factName: "RC_MAP_FLAPS"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Aux1:"
        property string zeroText: "Disabled"
        property string factName: "RC_MAP_AUX1"
        width: parent.width
        sourceComponent: component
    }

    Loader {
        property string labelText: "Aux2:"
        property string zeroText: "Disabled"
        property string factName: "RC_MAP_AUX2"
        width: parent.width
        sourceComponent: component
    }
}
