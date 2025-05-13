/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls

import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    anchors.fill: parent

    FactPanelController { id: controller }

    function getFact(name) {
        return controller.getParameterFact(-1, name, false)
    }

    property bool followParamsAvailable: controller.parameterExists(-1, "FOLL_SYSID")

    property var followItems: [
        { label: qsTr("Follow Enabled"),    fact: getFact("FOLL_ENABLE"),       visible: true},
        { label: qsTr("Follow System ID"),  fact: getFact("FOLL_SYSID"),        visible: followParamsAvailable },
        { label: qsTr("Max Distance"),      fact: getFact("FOLL_DIST_MAX"),     visible: followParamsAvailable },
        { label: qsTr("Offset X"),          fact: getFact("FOLL_OFS_X"),        visible: followParamsAvailable },
        { label: qsTr("Offset Y"),          fact: getFact("FOLL_OFS_Y"),        visible: followParamsAvailable },
        { label: qsTr("Offset Z"),          fact: getFact("FOLL_OFS_Z"),        visible: followParamsAvailable },
        { label: qsTr("Offset Type"),       fact: getFact("FOLL_OFS_TYPE"),     visible: followParamsAvailable },
        { label: qsTr("Altitude Type"),     fact: getFact("FOLL_ALT_TYPE"),     visible: followParamsAvailable },
        { label: qsTr("Yaw Behavior"),      fact: getFact("FOLL_YAW_BEHAVE"),   visible: followParamsAvailable }
    ]

    Column {
        anchors.fill: parent

        Repeater {
            model: followItems
            delegate: VehicleSummaryRow {
                labelText: modelData.label
                valueText: formatFact(modelData.fact)
                visible: modelData.visible

                function formatFact(fact) {
                    return (fact && (fact.enumStringValue || (fact.valueString + " " + fact.units)))
                }
            }
        }
    }
}
