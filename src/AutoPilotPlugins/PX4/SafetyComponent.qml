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
        Row {
            Label { text: "Climb to minimum altitude "; color: palette.windowText }
            FactTextField { fact: autopilot.parameters["RTL_RETURN_ALT"]; showUnits: true }
        }
        Row {
            Label { text: "When Home is reached, descend to altitude "; color: palette.windowText }
            FactTextField { fact: autopilot.parameters["RTL_DESCEND_ALT"]; showUnits: true }
        }
        Row {
            FactCheckBox {
                id: homeLoiterCheckbox
                fact: autopilot.parameters["RTL_LAND_DELAY"]
                checkedValue: 60
                uncheckedValue: -1
                text: "Loiter at Home altitude for "
            }
            FactTextField {
                fact: autopilot.parameters["RTL_LAND_DELAY"];
                showUnits: true
                enabled: homeLoiterCheckbox.checked
            }
        }

        Label { text: "Return to Land Triggers"; color: palette.windowText }
        Row {
            FactCheckBox {
                id: telemetryLossCheckbox
                fact: autopilot.parameters["COM_DL_LOSS_EN"]
                checkedValue: 1
                uncheckedValue: 0
                text: "Telemetry signal timeout - Return to Land"
            }
            Label { text: " after "; color: palette.windowText }
            FactTextField {
                fact: autopilot.parameters["NAV_DLL_N"];
                showUnits: true
                enabled: telemetryLossCheckbox.checked
            }
        }
        Row {
            Label { text: "RC Transmitter signal loss - Return to Land after "; color: palette.windowText }
            FactTextField { fact: autopilot.parameters["COM_RC_LOSS_T"]; showUnits: true }
        }

        Text {
            width: parent.width
            text: "Warning: You have an advanced safety configuration set using the NAV_RCL_OBC parameter. The above settings may not apply.";
            visible: autopilot.parameters["NAV_RCL_OBC"].value == 1
            color: palette.windowText
            wrapMode: Text.Wrap
        }
        Text {
            width: parent.width
            text: "Warning: You have an advanced safety configuration set using the NAV_DLL_OBC parameter. The above settings may not apply.";
            visible: autopilot.parameters["NAV_DLL_OBC"].value == 1
            color: palette.windowText
            wrapMode: Text.Wrap
        }
    }
}
