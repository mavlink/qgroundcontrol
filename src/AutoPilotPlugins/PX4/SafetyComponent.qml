import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

Rectangle {
    QGCPalette { id: palette; colorGroup: QGCPalette.Active }

    width: 600
    height: 400
    color: palette.window

    Column {
        anchors.fill: parent
        Label { text: "Safety Config (Work in Progress)"; color: palette.windowText; font.pointSize: 20 }
        Label { text: "Return to Land setup"; color: palette.windowText }
        Column {
            Row {
                Label { text: "Climb to minimum altitude "; color: palette.windowText }
                FactTextField { fact: autopilot.parameters["RTL_RETURN_ALT"]; showUnits: true }
            }
            Row {
                Label { text: "When Home is reached, descend to altitude "; color: palette.windowText }
                FactTextField { fact: autopilot.parameters["RTL_DESCEND_ALT"]; showUnits: true }
            }
            Row {
                CheckBox {
                    property Fact fact: autopilot.parameters["RTL_LAND_DELAY"]
                    checked: fact.value >= 0
                    text: "Loiter at Home altitude for "
                    onClicked: {
                        fact.value = checked ? 60 : -1
                    }
                    style: CheckBoxStyle {
                        label: Text {
                            color: palette.windowText
                            text: control.text
                        }
                    }
                }
                FactTextField { id: homeLoiterDelay; fact: autopilot.parameters["RTL_LAND_DELAY"]; showUnits: true }
            }
        }
        Label { text: "Return to Land Triggers"; color: palette.windowText }
        Row {
            Label { text: "Telemetry signal timeout - Return to Land after "; color: palette.windowText }
            FactTextField { fact: autopilot.parameters["NAV_DLL_N"]; showUnits: true }
        }
        Row {
            Label { text: "RC Transmitter signal loss - Return to Land after "; color: palette.windowText }
            FactTextField { fact: autopilot.parameters["COM_RC_LOSS_T"]; showUnits: true }
        }
    }
}
