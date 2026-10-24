import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4CollisionRows.js" as CollisionRows
import "PX4ConfigRows.js" as ConfigRows

SetupPage {
    id:             collisionPage
    pageComponent:  pageComponent

    // Parameter values when the page first showed them, so a reboot is only flagged after a change
    property var _initialValues: Object.create(null)

    readonly property var _vehicle:         controller.vehicle
    readonly property var _overlayFact:     QGroundControl.settingsManager.flyViewSettings.showObstacleDistanceOverlay
    readonly property var _rows: ConfigRows.visibleRows(
        CollisionRows.collisionRows(),
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

    function _rowActive(row) {
        return ConfigRows.isRowActive(row, function (name) {
            const fact = _fact(name)
            return fact ? fact.rawValue : undefined
        })
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
                    objectNamePrefix:   "collision"

                    rowDelegate: ConfigRow {
                        id:             rowLayout
                        objectName:     "collision_row_" + row.id
                        title:          row.title
                        params:         row.params
                        active:         _rowActive(row)
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
                                factFor:            collisionPage._fact
                                initialValues:      collisionPage._initialValues
                                objectNamePrefix:   "collision"

                                required property var modelData
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.5

                    ConfigGroupHeader {
                        text: qsTr("Display")
                    }

                    ConfigRow {
                        objectName: "collision_row_overlay"
                        title:      qsTr("Obstacle overlay")
                        active:     collisionPage._overlayFact.rawValue > 0

                        ConfigNote {
                            text: qsTr("Draws the distances measured around the vehicle on the fly view.")
                        }

                        cellsData: QGCCaptionCell {
                            caption: qsTr("Show obstacle distance overlay")

                            FactCheckBoxSlider {
                                objectName: "collision_setting_showObstacleDistanceOverlay"
                                fact:       collisionPage._overlayFact
                            }
                        }
                    }
                }
            }
        }
    }
}
