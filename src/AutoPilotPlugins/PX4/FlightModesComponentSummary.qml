import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

import "PX4FlightModeRows.js" as ModeRows

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width  // grows when Loader is wider than implicitWidth

    PX4FlightModesController { id: controller }

    readonly property var       _modeChannelFact:   controller.getParameterFact(-1, "RC_MAP_FLTMODE", false)
    readonly property var       _buttonMaskFact:    controller.getParameterFact(-1, "RC_MAP_FLTM_BTN", false)
    readonly property var       _bootModeFact:      controller.getParameterFact(-1, "COM_FLTMODE_BOOT", false)
    readonly property string    _method:            ModeRows.selectionMethod(_modeChannelFact ? _modeChannelFact.rawValue : 0,
                                                                             _buttonMaskFact ? _buttonMaskFact.rawValue : 0)

    function _modeName(fact) {
        if (!fact) {
            return ""
        }
        const name = ModeRows.modeName(fact.name, fact.enumStrings, fact.enumValues, controller.externalModeNames, fact.rawValue)
        return name !== "" ? name : fact.valueString
    }

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("Mode selection")
            valueText: {
                if (_method === "switch") {
                    return _modeChannelFact.enumStringValue
                }
                if (_method === "buttons") {
                    return qsTr("Buttons on %1").arg(ModeRows.buttonChannels(_buttonMaskFact.rawValue)
                                                     .map(channel => qsTr("Ch %1").arg(channel)).join(", "))
                }
                return qsTr("Setup required")
            }
        }

        Repeater {
            model: _method === "none" ? 0 : ModeRows.modeSlotCount

            VehicleSummaryRow {
                labelText: qsTr("Flight Mode %1").arg(index + 1)
                valueText: _modeName(controller.getParameterFact(-1, "COM_FLTMODE" + (index + 1), false))

                required property int index
            }
        }

        VehicleSummaryRow {
            labelText:  qsTr("Mode on boot")
            valueText:  _modeName(_bootModeFact)
            visible:    _bootModeFact !== null
        }
    }
}
