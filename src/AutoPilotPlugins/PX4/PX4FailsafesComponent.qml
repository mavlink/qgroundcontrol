import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4ConfigRows.js" as ConfigRows
import "PX4FailsafeRows.js" as FailsafeRows

SetupPage {
    id:             failsafesPage
    pageComponent:  pageComponent

    property bool _showActiveOnly:  false
    // Parameter values when the page first showed them, so a reboot is only flagged after a change
    property var _initialValues:    Object.create(null)

    readonly property var _vehicle: controller.vehicle
    readonly property var _allRows: FailsafeRows.failsafeRows()
    readonly property var _rows: ConfigRows.visibleRows(
        _allRows,
        {
            "multiRotor":   _vehicle ? _vehicle.multiRotor : false,
            "fixedWing":    _vehicle ? _vehicle.fixedWing : false,
            "vtol":         _vehicle ? _vehicle.vtol : false
        },
        function (name) { return controller.parameterExists(-1, name) })
    readonly property var _groups: ConfigRows.groupRows(_rows)
    readonly property int _shownCount: {
        let count = 0
        for (const row of _rows) {
            if (_rowShown(row)) {
                count++
            }
        }
        return count
    }

    function _fact(name) {
        return controller.getParameterFact(-1, name, false)
    }

    function _rowActive(row) {
        return ConfigRows.isRowActive(row, function (name) {
            const fact = _fact(name)
            return fact ? fact.rawValue : undefined
        })
    }

    function _rowShown(row) {
        return !_showActiveOnly || _rowActive(row)
    }

    function _groupShown(group) {
        return group.rows.some(row => _rowShown(row))
    }

    FactPanelController {
        id:                     controller
        // Reboot-required settings are marked on the page instead
        suppressRebootMessage:  true
    }

    QGCPalette {
        id: qgcPal
    }

    component SeverityDot: Rectangle {
        property string severity: "none"

        implicitWidth:  ScreenTools.defaultFontPixelHeight * 0.5
        implicitHeight: implicitWidth
        radius:         ScreenTools.defaultBorderRadius / 2
        color: {
            switch (severity) {
            case "warning":     return dotPal.colorYellow
            case "fallback":    return dotPal.colorYellowGreen
            case "hold":        return dotPal.colorBlue
            case "return":      return dotPal.accent
            case "land":        return dotPal.colorOrange
            case "terminate":   return dotPal.colorRed
            default:            return dotPal.textSecondary
            }
        }

        QGCPalette {
            id: dotPal
        }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, implicitWidth)
            spacing: _margins

            ConfigRowsPanel {
                ConfigRowsFilterBar {
                    objectName:         "failsafes_filter"
                    filterLabels:       [ qsTr("All Failsafes"), qsTr("Active") ]
                    filterIndex:        _showActiveOnly ? 1 : 0
                    summary:            _showActiveOnly ?
                                            qsTr("Showing %1 of %2 failsafes.").arg(_shownCount).arg(_rows.length) :
                                            qsTr("Showing all %1 failsafes.").arg(_rows.length)
                    onFilterActivated:  (index) => { _showActiveOnly = index === 1 }
                }

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth * 2

                    Repeater {
                        model: [
                            { "severity": "warning",    "text": qsTr("Warning") },
                            { "severity": "hold",       "text": qsTr("Hold") },
                            { "severity": "fallback",   "text": qsTr("Mode fallback") },
                            { "severity": "return",     "text": qsTr("Return") },
                            { "severity": "land",       "text": qsTr("Land") },
                            { "severity": "terminate",  "text": qsTr("Terminate / Disarm") }
                        ]

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth / 2

                            required property var modelData

                            SeverityDot {
                                severity: modelData.severity
                            }

                            QGCLabel {
                                text:           modelData.text
                                font.pointSize: ScreenTools.smallFontPointSize
                                color:          qgcPal.textSecondary
                            }
                        }
                    }
                }

                ConfigNote {
                    // A wrapped label reports its unwrapped width as its implicit width, which would widen the page
                    Layout.preferredWidth:  0
                    text:                   qsTr("Return actions use the settings on the Return & Land page.")
                }

                ConfigRowGroups {
                    model:              _groups
                    groupShown:         failsafesPage._groupShown
                    objectNamePrefix:   "failsafes"

                    rowDelegate: ConfigRow {
                        id:             rowLayout
                        objectName:     "failsafes_row_" + row.id
                        title:          row.title
                        params:         row.params
                        active:         _rowActive(row)
                        showSeparator:  index > 0
                        visible:        _rowShown(row)

                        required property var modelData
                        required property int index

                        readonly property var row: modelData

                        QGCChip {
                            text:       qsTr("Warning only")
                            visible:    rowLayout.row.warnOnly
                        }

                        ConfigNote {
                            text: rowLayout.row.description
                        }

                        cellsData: Repeater {
                            model: rowLayout.row.cells

                            ConfigParamCell {
                                id:                 paramCell
                                cell:               modelData
                                factFor:            failsafesPage._fact
                                initialValues:      failsafesPage._initialValues
                                objectNamePrefix:   "failsafes"
                                enumLeading:        modelData.severity !== undefined ? severityDotComponent : null

                                required property var modelData

                                Component {
                                    id: severityDotComponent

                                    SeverityDot {
                                        severity: FailsafeRows.actionSeverity(paramCell.modelData, paramCell.fact ? paramCell.fact.rawValue : undefined)
                                    }
                                }

                                extraData: RowLayout {
                                    spacing:    ScreenTools.defaultFontPixelWidth
                                    visible:    paramCell.modelData.fixed !== undefined

                                    SeverityDot {
                                        severity:   FailsafeRows.actionSeverity(paramCell.modelData)
                                        visible:    paramCell.modelData.severity !== undefined
                                    }

                                    QGCLabel {
                                        text: paramCell.modelData.fixed !== undefined ? paramCell.modelData.fixed : ""
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
