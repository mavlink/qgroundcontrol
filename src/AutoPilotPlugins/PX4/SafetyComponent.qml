import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0

Rectangle {
    QGCPalette { id: palette; colorGroup: QGCPalette.Active }

    width: 600
    height: 600
    color: palette.window
    property var leftColWidth: 350

    Column {
        anchors.fill: parent
        spacing: 40
        //-----------------------------------------------------------------
        //-- Return Home Triggers
        Column {
            spacing: 18
            Label { text: "Triggers For Return Home"; color: palette.windowText; font.pointSize: 20 }
            Row {
                Label {
                    width: leftColWidth
                    text: "RC Transmitter Signal Loss - Return Home After"
                    color: palette.windowText
                    anchors.baseline: rcLossField.baseline
                }
                FactTextField {
                    id: rcLossField
                    fact: autopilot.parameters["COM_RC_LOSS_T"]
                    showUnits: true
                }
            }
            Row {
                FactCheckBox {
                    id: telemetryLossCheckbox
                    fact: autopilot.parameters["COM_DL_LOSS_EN"]
                    width: leftColWidth
                    checkedValue: 1
                    uncheckedValue: 0
                    text: "Telemetry Signal Timeout - Return Home After"
                    anchors.baseline: telemetryLossField.baseline
                }
                FactTextField {
                    id: telemetryLossField
                    fact: autopilot.parameters["NAV_DLL_N"];
                    showUnits: true
                }
            }
        }
        //-----------------------------------------------------------------
        //-- Return Home Options
        Column {
            spacing: 18
            Label { text: "Return Home Options"; color: palette.windowText; font.pointSize: 20 }
            Row {
                Label {
                    width: leftColWidth
                    text: "Climb to minimum altitude of "
                    color: palette.windowText
                    anchors.baseline: climbField.baseline
                }
                FactTextField {
                    id: climbField
                    fact: autopilot.parameters["RTL_RETURN_ALT"]
                    showUnits: true
                }
            }
            Row {
                CheckBox {
                    id: homeLoiterCheckbox
                    property Fact fact: autopilot.parameters["RTL_LAND_DELAY"]
                    width: leftColWidth
                    checked: fact.value > 0
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
                FactTextField {
                    fact: autopilot.parameters["RTL_LAND_DELAY"];
                    showUnits: true
                    anchors.baseline: homeLoiterCheckbox.baseline
                    enabled: homeLoiterCheckbox.checked == true
                }
            }
            //-------------------------------------------------------------
            //-- Visible only if loiter above is checked
            //   TODO The "enabled" property could be used instead but it
            //   would have to handle a different "disabled" palette.
            Row {
                Label {
                    width: leftColWidth;
                    text: "When Home is reached, loiter at an altitude of ";
                    color: palette.windowText;
                    anchors.baseline: descendField.baseline
                    visible: homeLoiterCheckbox.checked == true
                }
                FactTextField {
                    id: descendField;
                    fact: autopilot.parameters["RTL_DESCEND_ALT"];
                    visible: homeLoiterCheckbox.checked == true
                    showUnits: true
                }
            }
        }

        Text {
            width: parent.width
            font.pointSize: 14
            text: "Warning: You have an advanced safety configuration set using the NAV_RCL_OBC parameter. The above settings may not apply.";
            visible: autopilot.parameters["NAV_RCL_OBC"].value != 0
            color: palette.windowText
            wrapMode: Text.Wrap
        }
        Text {
            width: parent.width
            font.pointSize: 14
            text: "Warning: You have an advanced safety configuration set using the NAV_DLL_OBC parameter. The above settings may not apply.";
            visible: autopilot.parameters["NAV_DLL_OBC"].value != 0
            color: palette.windowText
            wrapMode: Text.Wrap
        }
    }
}
