import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width  // grows when Loader is wider than implicitWidth

    FactPanelController { id: controller; }

    property Fact   _returnAltFact:     controller.getParameterFact(-1, "RTL_RETURN_ALT")
    property Fact   _descendAltFact:    controller.getParameterFact(-1, "RTL_DESCEND_ALT")
    property Fact   _rtlLandDelayFact:  controller.getParameterFact(-1, "RTL_LAND_DELAY")
    property int    _rtlLandDelayValue: _rtlLandDelayFact ? _rtlLandDelayFact.value : 0

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("RTL Climb To")
            valueText: _returnAltFact ? _returnAltFact.valueString + " " + _returnAltFact.units : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("RTL, Then")
            valueText: _rtlLandDelayValue === 0 ?
                           qsTr("Land immediately") :
                           (_rtlLandDelayValue < 0 ?
                                qsTr("Loiter and do not land") :
                                qsTr("Loiter and land after specified time"))
        }

        VehicleSummaryRow {
            labelText: qsTr("Loiter Alt")
            valueText: _descendAltFact ? _descendAltFact.valueString + " " + _descendAltFact.units : ""
            visible:    _descendAltFact && _rtlLandDelayValue !== 0
        }

        VehicleSummaryRow {
            labelText: qsTr("Land Delay")
            valueText: _rtlLandDelayFact ? _rtlLandDelayValue + " " + _rtlLandDelayFact.units : ""
            visible:    _rtlLandDelayValue > 0
        }
    }
}
