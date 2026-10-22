import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4BatteryRows.js" as BatteryRows

SetupPage {
    id:             batteryPage
    pageComponent:  pageComponent

    property real _cellPadding:     ScreenTools.defaultFontPixelWidth * 1.5
    property bool _showInUseOnly:   false
    property bool _filterChosen:    false
    // Parameter values when the page first showed them, so a reboot is only flagged after a change
    property var _initialValues:    Object.create(null)

    readonly property var _vehicle: controller.vehicle
    readonly property bool _estimatorOverwrite: BatteryRows.overwriteInEstimator(_vehicle ? _vehicle.firmwareMajorVersion : -1,
                                                                                 _vehicle ? _vehicle.firmwareMinorVersion : -1)
    readonly property bool _loading: controller.pendingParams.length > 0
    readonly property var _allRows: BatteryRows.batteryRows()
    readonly property var _rows: {
        // Rebuilt only when the vehicle adds a parameter. Each rebuild recreates the row controls, and a read the
        // firmware cannot answer changes nothing visible.
        controller.addedParamCount
        return BatteryRows.visibleRows(_allRows, name => controller.parameterReady(name))
    }
    readonly property var _groups: {
        const groups = []
        for (const row of _rows) {
            if (groups.length === 0 || groups[groups.length - 1].name !== row.group) {
                groups.push({ "name": row.group, "rows": [] })
            }
            groups[groups.length - 1].rows.push(row)
        }
        return groups
    }
    readonly property var _reportedTelemetry: {
        const reported = []
        const batteries = _vehicle ? _vehicle.batteries : null
        for (let i = 0; batteries && i < batteries.count; i++) {
            reported.push(batteries.get(i))
        }
        return reported
    }
    readonly property var _otherTelemetry: {
        const batteries = _rows.filter(row => row.instance !== undefined)
                               .map(row => ({ "instance": row.instance, "inputs": _rowInputs(row) }))
        const unclaimed = BatteryRows.unclaimedTelemetryIds(_reportedTelemetry.map(battery => battery.id.rawValue), batteries)
        return _reportedTelemetry.filter(battery => unclaimed.indexOf(battery.id.rawValue) >= 0)
    }
    readonly property var _filterableRows: _rows.filter(row => !row.shared)
    readonly property int _inUseCount: _filterableRows.filter(row => _rowActive(row)).length

    function _fact(name) {
        return controller.getParameterFact(-1, name, false)
    }

    function _rawValue(name) {
        const fact = _fact(name)
        return fact ? fact.rawValue : undefined
    }

    function _rowActive(row) {
        return BatteryRows.isRowActive(row, _rawValue)
    }

    function _rowShown(row) {
        return !_showInUseOnly || row.shared === true || _rowActive(row)
    }

    function _groupShown(group) {
        return group.rows.some(row => _rowShown(row))
    }

    function _rowInputs(row) {
        return row.sourceParam ? BatteryRows.batteryInputs(_rawValue(row.sourceParam), _rawValue("UAVCAN_SUB_BAT"),
                                                           _rawValue("UAVCAN_ENABLE"), _estimatorOverwrite) : []
    }

    function _sourceDescription(inputs) {
        if (inputs.indexOf("canFiltered") >= 0) {
            return qsTr("DroneCAN data filtered through the battery estimate below.")
        }
        if (inputs.indexOf("external") >= 0) {
            return qsTr("Reported by MAVLink, DroneCAN or a driver, which estimates the charge itself.")
        }
        if (inputs.indexOf("escs") >= 0) {
            return qsTr("Voltage and current from ESC telemetry.")
        }
        if (inputs.indexOf("powerModule") >= 0) {
            return qsTr("Analog power module or I2C power monitor. The analog settings only apply to the board's ADC inputs.")
        }
        return qsTr("Not monitored.")
    }

    function _batteryTelemetry(telemetryId) {
        if (telemetryId < 0) {
            return null
        }
        for (const battery of _reportedTelemetry) {
            if (battery.id.rawValue === telemetryId) {
                return battery
            }
        }
        return null
    }

    function _telemetryText(battery) {
        return [battery.voltage, battery.percentRemaining, battery.current]
            .map(fact => fact.valueString + " " + fact.units)
            .join("   ")
    }

    // Reads pendingParams from the controller, not _loading, which may not have updated yet when pendingParams changes
    function _chooseFilter() {
        if (!_filterChosen && controller.pendingParams.length === 0) {
            _filterChosen = true
            _showInUseOnly = _inUseCount > 0
        }
    }

    PX4BatteryController {
        id:                     controller
        // Reboot-required settings are marked on the page instead
        suppressRebootMessage:  true

        // Opening on what is in use hides the spare battery slots and unused power monitor drivers
        onPendingParamsChanged: _chooseFilter()
    }

    QGCPalette {
        id: qgcPal
    }

    QGCPopupDialogFactory {
        id:                 voltageDividerDialogFactory
        dialogComponent:    voltageDividerDialogComponent
    }

    Component {
        id: voltageDividerDialogComponent

        CalcVoltageDividerDialog { }
    }

    QGCPopupDialogFactory {
        id:                 ampsPerVoltDialogFactory
        dialogComponent:    ampsPerVoltDialogComponent
    }

    Component {
        id: ampsPerVoltDialogComponent

        CalcAmpsPerVoltDialog { }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, implicitWidth)
            spacing: _margins

            Rectangle {
                Layout.fillWidth:   true
                implicitWidth:      panelLayout.implicitWidth + _margins * 2
                implicitHeight:     panelLayout.implicitHeight + _margins * 2
                color:              qgcPal.windowPanel
                border.color:       qgcPal.borderSubtle
                radius:             ScreenTools.defaultBorderRadius * 2

                ColumnLayout {
                    id:                 panelLayout
                    anchors.fill:       parent
                    anchors.margins:    _margins
                    spacing:            _margins

                    RowLayout {
                        Layout.fillWidth:   true
                        spacing:            _margins

                        QGCRadioButtonBar {
                            model:          [ qsTr("All"), qsTr("In Use") ]
                            currentIndex:   _showInUseOnly ? 1 : 0
                            onActivated:    (index) => {
                                _filterChosen = true
                                _showInUseOnly = index === 1
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            color:              qgcPal.textSecondary
                            text:               {
                                const total = _filterableRows.length
                                if (_showInUseOnly) {
                                    return total === 1 ?
                                               qsTr("Showing %1 of 1 battery or power monitor.").arg(_inUseCount) :
                                               qsTr("Showing %1 of %2 batteries and power monitors.").arg(_inUseCount).arg(total)
                                }
                                return total === 1 ?
                                           qsTr("Showing the only battery or power monitor.") :
                                           qsTr("Showing all %1 batteries and power monitors.").arg(total)
                            }
                        }

                        QGCLabel {
                            objectName: "battery_loading"
                            color:      qgcPal.textSecondary
                            text:       qsTr("Loading settings from the vehicle...")
                            visible:    _loading
                        }
                    }

                    Repeater {
                        model: _groups

                        ColumnLayout {
                            Layout.fillWidth:   true
                            spacing:            _margins
                            visible:            _groupShown(group)

                            required property var modelData

                            readonly property var group: modelData

                            RowLayout {
                                Layout.fillWidth:   true
                                spacing:            _margins

                                QGCCaptionLabel {
                                    text:           group.name
                                    color:          qgcPal.textPrimary
                                }

                                Rectangle {
                                    Layout.fillWidth:   true
                                    implicitHeight:     1
                                    color:              qgcPal.borderSubtle
                                }
                            }

                            Repeater {
                                model: group.rows

                                ColumnLayout {
                                    id:                 rowLayout
                                    objectName:         "battery_row_" + row.id
                                    Layout.fillWidth:   true
                                    spacing:            _margins
                                    visible:            _rowShown(row)

                                    required property var modelData
                                    required property int index

                                    readonly property var row:          modelData
                                    readonly property bool _active:     _rowActive(row)
                                    readonly property bool _isBattery:  row.instance !== undefined
                                    readonly property var _inputs:      _rowInputs(row)
                                    readonly property var _telemetry:   _isBattery ? _batteryTelemetry(BatteryRows.telemetryId(row.instance, _inputs)) : null
                                    readonly property string _warning:  _isBattery ? BatteryRows.sourceWarning(row.instance, _rawValue(row.sourceParam)) : ""

                                    Rectangle {
                                        Layout.fillWidth:   true
                                        implicitHeight:     1
                                        color:              qgcPal.borderSubtle
                                        visible:            rowLayout.index > 0
                                    }

                                    RowLayout {
                                        Layout.fillWidth:   true
                                        spacing:            _margins

                                        ColumnLayout {
                                            Layout.alignment:       Qt.AlignTop
                                            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 30
                                            Layout.maximumWidth:    Layout.preferredWidth
                                            Layout.topMargin:       _cellPadding
                                            spacing:                ScreenTools.defaultFontPixelHeight * 0.25

                                            RowLayout {
                                                spacing: ScreenTools.defaultFontPixelWidth

                                                Rectangle {
                                                    implicitWidth:  ScreenTools.defaultFontPixelHeight * 0.5
                                                    implicitHeight: implicitWidth
                                                    radius:         implicitWidth / 2
                                                    color:          rowLayout._active ? qgcPal.colorGreen : "transparent"
                                                    border.color:   rowLayout._active ? color : qgcPal.textSecondary
                                                }

                                                QGCLabel {
                                                    Layout.fillWidth:       true
                                                    text:                   rowLayout.row.title
                                                    wrapMode:               Text.WordWrap
                                                    font.bold:              true
                                                    font.capitalization:    Font.AllUppercase
                                                    font.letterSpacing:     ScreenTools.defaultFontPixelWidth * 0.1
                                                }
                                            }

                                            Flow {
                                                Layout.fillWidth:   true
                                                spacing:            ScreenTools.defaultFontPixelWidth

                                                Repeater {
                                                    model: BatteryRows.usedParams(rowLayout.row, rowLayout._inputs)

                                                    QGCLabel {
                                                        text:           modelData
                                                        font.family:    ScreenTools.fixedFontFamily
                                                        font.pointSize: ScreenTools.smallFontPointSize
                                                        color:          qgcPal.textSecondary

                                                        required property string modelData
                                                    }
                                                }
                                            }

                                            QGCLabel {
                                                objectName:     "battery_telemetry_" + rowLayout.row.id
                                                text:           rowLayout._telemetry ? _telemetryText(rowLayout._telemetry) : ""
                                                font.family:    ScreenTools.fixedFontFamily
                                                visible:        rowLayout._telemetry !== null
                                            }

                                            QGCLabel {
                                                Layout.fillWidth:   true
                                                wrapMode:           Text.WordWrap
                                                font.pointSize:     ScreenTools.smallFontPointSize
                                                color:              qgcPal.textSecondary
                                                text:               rowLayout._isBattery ? _sourceDescription(rowLayout._inputs) : rowLayout.row.description
                                            }

                                            QGCLabel {
                                                Layout.fillWidth:   true
                                                wrapMode:           Text.WordWrap
                                                font.pointSize:     ScreenTools.smallFontPointSize
                                                color:              qgcPal.warningText
                                                text:               rowLayout._warning
                                                visible:            text !== ""
                                            }
                                        }

                                        // Cells wrap instead of widening the page, so the column only needs its widest single cell
                                        Flow {
                                            Layout.alignment:       Qt.AlignTop
                                            Layout.fillWidth:       true
                                            Layout.minimumWidth:    _widestChildWidth
                                            Layout.preferredWidth:  _widestChildWidth
                                            spacing:                _margins

                                            readonly property real _widestChildWidth: {
                                                let widest = 0
                                                for (let i = 0; i < children.length; i++) {
                                                    if (children[i].visible) {
                                                        widest = Math.max(widest, children[i].implicitWidth)
                                                    }
                                                }
                                                return widest
                                            }

                                            Repeater {
                                                model: rowLayout.row.cells

                                                QGCCaptionCell {
                                                    id:         cell
                                                    caption:    modelData.label
                                                    visible:    BatteryRows.cellUsed(modelData, rowLayout._inputs)

                                                    required property var modelData

                                                    readonly property var _fact:        modelData.param ? batteryPage._fact(modelData.param) : null
                                                    readonly property bool _isEnum:     _fact ? _fact.enumStrings.length > 0 : false
                                                    readonly property bool _rebootPending: _fact !== null && _fact.vehicleRebootRequired &&
                                                                                           _initialValue !== undefined && _fact.rawValue !== _initialValue

                                                    property var _initialValue: undefined

                                                    Component.onCompleted: {
                                                        if (!_fact) {
                                                            return
                                                        }
                                                        if (!(modelData.param in batteryPage._initialValues)) {
                                                            batteryPage._initialValues[modelData.param] = _fact.rawValue
                                                        }
                                                        _initialValue = batteryPage._initialValues[modelData.param]
                                                    }

                                                    FactComboBox {
                                                        Layout.fillWidth:       true
                                                        Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 50
                                                        fact:                   cell._fact
                                                        indexModel:             false
                                                        sizeToContents:         true
                                                        visible:                cell._isEnum
                                                    }

                                                    RowLayout {
                                                        spacing:    ScreenTools.defaultFontPixelWidth
                                                        visible:    cell._fact !== null && !cell._isEnum

                                                        FactTextField {
                                                            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 14
                                                            fact:                   cell._fact
                                                        }

                                                        QGCButton {
                                                            objectName: "battery_calc_" + (modelData.param !== undefined ? modelData.param : "")
                                                            text:       qsTr("Calculate")
                                                            // Calculating scales the current value, so it needs a set value rather than the board default
                                                            enabled:    rowLayout._telemetry !== null && cell._fact !== null && cell._fact.rawValue > 0
                                                            visible:    modelData.calc !== undefined
                                                            onClicked: {
                                                                const factory = modelData.calc === "voltageDivider" ?
                                                                                    voltageDividerDialogFactory : ampsPerVoltDialogFactory
                                                                factory.open({ "batteryIndex": rowLayout.row.instance })
                                                            }
                                                        }
                                                    }

                                                    BatteryThresholdBar {
                                                        // visibleRows only keeps this cell when all three parameters exist
                                                        visible:    modelData.thresholdBar !== undefined
                                                        emergency:  modelData.thresholdBar ? batteryPage._fact(modelData.thresholdBar[0]).rawValue : 0
                                                        critical:   modelData.thresholdBar ? batteryPage._fact(modelData.thresholdBar[1]).rawValue : 0
                                                        low:        modelData.thresholdBar ? batteryPage._fact(modelData.thresholdBar[2]).rawValue : 0
                                                    }

                                                    QGCChip {
                                                        objectName: "battery_rebootChip_" + (modelData.param !== undefined ? modelData.param : "")
                                                        text:       cell._rebootPending ? qsTr("Reboot required") : qsTr("Applies after reboot")
                                                        textColor:  cell._rebootPending ? qgcPal.warningText : qgcPal.textSecondary
                                                        visible:    cell._fact ? cell._fact.vehicleRebootRequired : false
                                                    }

                                                    QGCLabel {
                                                        Layout.fillWidth:       true
                                                        Layout.preferredWidth:  Math.min(implicitWidth, ScreenTools.defaultFontPixelWidth * 20)
                                                        wrapMode:               Text.WordWrap
                                                        font.pointSize:         ScreenTools.smallFontPointSize
                                                        color:                  qgcPal.textSecondary
                                                        text:                   modelData.note !== undefined ? modelData.note : ""
                                                        visible:                modelData.note !== undefined
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Reported under an id that does not identify a battery row above
                    ColumnLayout {
                        Layout.fillWidth:   true
                        spacing:            _margins
                        visible:            _otherTelemetry.length > 0

                        RowLayout {
                            Layout.fillWidth:   true
                            spacing:            _margins

                            QGCCaptionLabel {
                                text:   qsTr("Other Reported Batteries")
                                color:  qgcPal.textPrimary
                            }

                            Rectangle {
                                Layout.fillWidth:   true
                                implicitHeight:     1
                                color:              qgcPal.borderSubtle
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            wrapMode:           Text.WordWrap
                            font.pointSize:     ScreenTools.smallFontPointSize
                            color:              qgcPal.textSecondary
                            text:               qsTr("Batteries the vehicle reports that are not linked to a battery above, such as External sources. DroneCAN batteries report their node ID minus one, and MAVLink batteries 255.")
                        }

                        Repeater {
                            model: _otherTelemetry

                            RowLayout {
                                objectName: "battery_other_" + modelData.id.rawValue
                                spacing:    ScreenTools.defaultFontPixelWidth * 2

                                required property var modelData

                                QGCLabel {
                                    text:       qsTr("ID %1").arg(modelData.id.rawValue)
                                    font.bold:  true
                                }

                                QGCLabel {
                                    text:           _telemetryText(modelData)
                                    font.family:    ScreenTools.fixedFontFamily
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        controller.requestParameters(BatteryRows.allParams(_allRows))
        _chooseFilter()
    }
}
