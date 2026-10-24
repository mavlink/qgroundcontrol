import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4ConfigRows.js" as ConfigRows
import "PX4ReturnLandRows.js" as ReturnLandRows

SetupPage {
    id:             returnLandPage
    pageComponent:  pageComponent

    // Parameter values when the page first showed them, so a reboot is only flagged after a change
    property var _initialValues: Object.create(null)

    readonly property var _vehicle: controller.vehicle
    readonly property var _rows: ConfigRows.visibleRows(
        ReturnLandRows.returnLandRows(),
        {
            "multiRotor":   _vehicle ? _vehicle.multiRotor : false,
            "fixedWing":    _vehicle ? _vehicle.fixedWing : false,
            "vtol":         _vehicle ? _vehicle.vtol : false
        },
        function (name) { return controller.parameterExists(-1, name) })
    readonly property var _groups: ConfigRows.groupRows(_rows)

    function _fact(name) {
        return controller.getParameterFact(-1, name, false)
    }

    function _rawValue(name) {
        const fact = _fact(name)
        return fact ? fact.rawValue : undefined
    }

    FactPanelController {
        id:                     controller
        // Reboot-required settings are marked on the page instead
        suppressRebootMessage:  true
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, implicitWidth)
            spacing: _margins

            ConfigRowsPanel {
                ConfigRowGroups {
                    model:              _groups
                    objectNamePrefix:   "returnLand"

                    rowDelegate: ConfigRow {
                        id:             rowLayout
                        objectName:     "returnLand_row_" + row.id
                        title:          row.title
                        params:         row.params
                        active:         true
                        showSeparator:  index > 0

                        required property var modelData
                        required property int index

                        readonly property var row: modelData

                        ConfigNote {
                            text: rowLayout.row.description
                        }

                        cellsData: Repeater {
                            model: rowLayout.row.cells

                            ConfigParamCell {
                                cell:               modelData
                                factFor:            returnLandPage._fact
                                initialValues:      returnLandPage._initialValues
                                objectNamePrefix:   "returnLand"
                                enabled:            !ReturnLandRows.cellOverridden(modelData, returnLandPage._rawValue)

                                required property var modelData
                            }
                        }
                    }
                }
            }
        }
    }
}
