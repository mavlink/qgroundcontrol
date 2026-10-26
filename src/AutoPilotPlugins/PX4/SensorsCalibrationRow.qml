import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "PX4SensorsRows.js" as SensorsRows

/// A calibration row of the Sensors page: its status, the button which starts it and, for calibrations which run in the
/// row, their progress, hint and result. It does not start anything itself, the page handles the signals.
ConfigRow {
    id: control

    /// A row from PX4SensorsRows.js calibrationRows()
    required property var row

    /// "ok", "needs" or "optional", from SensorsRows.calibrationState()
    property string calibrationState:   "optional"
    /// For a calibration which runs in the row: "idle", "running", "done" or "failed"
    property string inlineState:        "idle"
    property string inlineMessage:      ""
    /// 0 to 1
    property real   progress:           0
    /// From SensorsRows.gpsReadiness(), used by rows which show the GPS
    property var    gps:                SensorsRows.gpsReadiness(0, 0, NaN)
    /// False while something else prevents this calibration from starting, such as another calibration running
    property bool   calibrateEnabled:   true
    /// False until the vehicle has started a calibration which runs in the row, and while it is cancelling
    property bool   cancelEnabled:      false
    /// False while a calibration is running, so the parameters it changes cannot be edited
    property bool   paramsEnabled:      true
    property var    factFor
    property var    initialValues

    signal calibrateClicked
    signal cancelClicked
    signal alternateClicked

    readonly property bool      _inline:    row.inline === true
    readonly property bool      _busy:      inlineState === "running"
    readonly property bool      _failed:    inlineState === "failed"
    readonly property bool      _done:      inlineState === "done"
    readonly property string    _buttonName:    "sensorsSetup_calibrate" + row.id.charAt(0).toUpperCase() + row.id.slice(1)
    readonly property string    _hint: {
        if (!_inline) {
            return ""
        }
        if (_failed) {
            return inlineMessage
        }
        if (_busy) {
            return qsTr("Keep the vehicle still.")
        }
        if (row.gps) {
            return calibrationState !== "ok" || gps.level !== "good" ? gps.message : ""
        }
        return _done ? "" : row.hint
    }
    readonly property color     _hintColor: {
        if (_failed || (row.gps && gps.level === "none")) {
            return qgcPal.errorText
        }
        return row.gps && gps.level === "poor" ? qgcPal.warningText : qgcPal.textSecondary
    }

    objectName:     "sensors_row_" + row.id
    title:          row.title
    params:         row.params
    active:         calibrationState !== "optional"
    activeColor:    calibrationState === "needs" ? qgcPal.colorRed : qgcPal.colorGreen

    QGCPalette { id: qgcPal }

    ConfigNote {
        text: control.row.description
    }

    cellsData: [
        QGCCaptionCell {
            caption: qsTr("Calibration")

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth * 2

                QGCLabel {
                    id:                     statusLabel
                    objectName:             "sensors_status_" + control.row.id
                    Layout.fillWidth:       true
                    // Calibrated and Calibrating are the same width
                    Layout.minimumWidth:    Math.ceil(calibratingMetrics.advanceWidth)
                    text:                   control._busy ? qsTr("Calibrating…") :
                                                control._failed ? qsTr("Calibration failed") :
                                                control._done ? qsTr("Calibrated") :
                                                SensorsRows.calibrationStatusText(control.row, control.calibrationState)
                    font.bold:              true
                    color:                  control._failed || control.calibrationState === "needs" ? qgcPal.errorText :
                                                control._busy ? qgcPal.textPrimary :
                                                control._done || control.calibrationState === "ok" ? qgcPal.successText : qgcPal.textSecondary

                    TextMetrics {
                        id:     calibratingMetrics
                        font:   statusLabel.font
                        text:   qsTr("Calibrating…")
                    }
                }

                QGCButton {
                    objectName: control._buttonName
                    text:       control._busy ? qsTr("Cancel") : control._failed ? qsTr("Retry") : qsTr("Calibrate")
                    primary:    control._failed || control.calibrationState === "needs" ||
                                (control.row.gps === true && control.calibrationState !== "ok" && control.gps.ready)
                    enabled:    control._busy ? control.cancelEnabled :
                                    control.calibrateEnabled && (control.row.gps !== true || control.gps.ready)
                    onClicked:  control._busy ? control.cancelClicked() : control.calibrateClicked()
                }
            }

            // The width comes from the status line, so these fill it
            ProgressBar {
                objectName:             "sensors_progress_" + control.row.id
                Layout.fillWidth:       true
                Layout.preferredWidth:  0
                visible:                control._busy
                value:                  control.progress
                indeterminate:          control.row.progress !== true
            }

            ConfigNote {
                objectName:             "sensors_hint_" + control.row.id
                Layout.preferredWidth:  0
                text:                   control._hint
                color:                  control._hintColor
            }

            QGCButton {
                objectName: control._buttonName + "Alternate"
                text:       control.row.alternate ? control.row.alternate.label : ""
                visible:    control.row.alternate !== undefined
                enabled:    control.calibrateEnabled
                onClicked:  control.alternateClicked()
            }

            ConfigNote {
                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 28
                text:                   control.row.alternate ? control.row.alternate.note : ""
            }
        },

        QGCCaptionCell {
            objectName: "sensors_gpsCell"
            caption:    qsTr("GPS")
            visible:    control.row.gps === true

            SensorsGpsStatus {
                gps: control.gps
            }
        },

        Repeater {
            model: control.row.cells

            ConfigParamCell {
                cell:               modelData
                factFor:            control.factFor
                initialValues:      control.initialValues
                objectNamePrefix:   "sensors"
                enabled:            control.paramsEnabled

                required property var modelData
            }
        }
    ]
}
