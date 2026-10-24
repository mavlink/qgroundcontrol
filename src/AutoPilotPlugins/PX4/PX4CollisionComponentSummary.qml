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

    property Fact _distanceFact: controller.getParameterFact(-1, "CP_DIST")

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("Collision Prevention")
            valueText: !_distanceFact ? "" :
                           (_distanceFact.rawValue > 0 ?
                                _distanceFact.valueString + " " + _distanceFact.units :
                                qsTr("Disabled"))
        }
    }
}
