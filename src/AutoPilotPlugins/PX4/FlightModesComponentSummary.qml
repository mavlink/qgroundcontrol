import QtQuick          2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property Fact _nullFact
    property Fact _rcMapFltmode:    controller.parameterExists(-1, "RC_MAP_FLTMODE") ? controller.getParameterFact(-1, "RC_MAP_FLTMODE") : _nullFact
    property Fact _rcMapModeSw:     controller.getParameterFact(-1, "RC_MAP_MODE_SW")
    property bool _simpleMode:      _rcMapFltmode.value > 0 || _rcMapModeSw.value == 0


    Loader {
        anchors.fill:       parent
        sourceComponent:    _simpleMode ? simple : advanced
    }

    Component {
        id: simple

        Column {
            anchors.margins:    8

            VehicleSummaryRow {
                labelText: "Mode switch:"
                valueText: _rcMapFltmode.value === 0 ? "Setup required" : _rcMapFltmode.enumStringValue
            }

            Repeater {
                model: 6

                VehicleSummaryRow {
                    labelText: "Flight Mode " + (index + 1) + ":"
                    valueText: controller.getParameterFact(-1, "COM_FLTMODE" + (index + 1)).enumStringValue
                }
            }
        }
    }

    Component {
        id: advanced

        Column {
            anchors.margins:    8

            property Fact posCtlSwFact: controller.getParameterFact(-1, "RC_MAP_POSCTL_SW")
            property Fact loiterSwFact: controller.getParameterFact(-1, "RC_MAP_LOITER_SW")
            property Fact returnSwFact: controller.getParameterFact(-1, "RC_MAP_RETURN_SW")

            VehicleSummaryRow {
                labelText: "Mode switch:"
                valueText: _rcMapModeSw.value === 0 ? "Setup required" : _rcMapModeSw.valueString
            }

            VehicleSummaryRow {
                labelText: "Position Ctl switch:"
                valueText: posCtlSwFact.value === 0 ? "Disabled" : posCtlSwFact.valueString
            }

            VehicleSummaryRow {
                labelText: "Loiter switch:"
                valueText: loiterSwFact.value === 0 ? "Disabled" : loiterSwFact.valueString
            }

            VehicleSummaryRow {
                labelText: "Return switch:"
                valueText: returnSwFact.value === 0 ? "Disabled" : returnSwFact.valueString
            }
        }
    }
}
