import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4ConfigRows.js" as ConfigRows
import "PX4EscRows.js" as EscRows

SetupPage {
    id:             escPage
    pageComponent:  pageComponent

    // Parameter values when the page first showed them, so a reboot is only flagged after a change
    property var _initialValues: Object.create(null)

    readonly property int _motorCount: {
        const fact = _fact("CA_ROTOR_COUNT")
        return fact && fact.rawValue > 0 ? fact.rawValue : 12
    }
    readonly property var _outputKinds: {
        const protocols = []
        for (const name of EscRows.timerParams()) {
            const fact = _fact(name)
            if (fact) {
                protocols.push(fact.rawValue)
            }
        }
        return EscRows.outputKinds(protocols)
    }
    readonly property var _rows: ConfigRows.visibleRows(
        EscRows.escRows(_motorCount),
        ({}),
        function (name) { return controller.parameterExists(-1, name) })
    readonly property var _groups: ConfigRows.groupRows(_rows)

    function _fact(name) {
        return controller.getParameterFact(-1, name, false)
    }

    function _rawValue(name) {
        const fact = _fact(name)
        return fact ? fact.rawValue : undefined
    }

    function _rowShown(row) {
        return EscRows.rowShown(row, _outputKinds)
    }

    function _groupShown(group) {
        return group.rows.some(row => _rowShown(row))
    }

    PX4EscController {
        id:                     controller
        // Reboot-required settings are marked on the page instead
        suppressRebootMessage:  true
    }

    QGCPalette {
        id: qgcPal
    }

    QGCPopupDialogFactory {
        id:                 escCalibrationDialogFactory
        dialogComponent:    escCalibrationDialogComponent
    }

    Component {
        id: escCalibrationDialogComponent

        ESCCalibrationDialog { }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, implicitWidth)
            spacing: _margins

            ConfigRowsPanel {
                ColumnLayout {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                    visible:            EscRows.calibrationShown(escPage._outputKinds)

                    ConfigGroupHeader {
                        text: qsTr("Calibration")
                    }

                    ConfigRow {
                        objectName: "esc_row_calibration"
                        title:      qsTr("PWM calibration")
                        active:     true

                        ConfigNote {
                            text: qsTr("Teaches ESCs which take a PWM signal the full throttle range. DShot and DroneCAN ESCs do not need it.")
                        }

                        ConfigNote {
                            color:  qgcPal.warningText
                            text:   qsTr("Remove the propellers first. Use a USB connection, with the battery disconnected until calibration asks for it.")
                        }

                        cellsData: QGCCaptionCell {
                            QGCButton {
                                objectName: "esc_calibrateButton"
                                text:       qsTr("Calibrate")
                                onClicked:  escCalibrationDialogFactory.open()
                            }
                        }
                    }
                }

                ConfigRowGroups {
                    model:              escPage._groups
                    groupShown:         escPage._groupShown
                    objectNamePrefix:   "esc"

                    rowDelegate: ConfigRow {
                        id:             rowLayout
                        objectName:     "esc_row_" + row.id
                        title:          row.title
                        params:         row.params
                        active:         true
                        showSeparator:  index > 0
                        visible:        escPage._rowShown(row)

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
                                factFor:            escPage._fact
                                initialValues:      escPage._initialValues
                                objectNamePrefix:   "esc"
                                visible:            EscRows.cellShown(modelData, escPage._rawValue)

                                required property var modelData
                            }
                        }
                    }
                }
            }
        }
    }
}
