.pragma library
.import "PX4ConfigRows.js" as ConfigRows

// Row definitions and source rules for PX4BatteryComponent.qml. Rows and cells use the PX4ConfigRows.js schema,
// so filtering and the active rules are shared. Controls, units and choices come from the parameter metadata.
//
// Extra row fields:  instance (1-based battery number), sourceParam, shared?
//   shared   settings for every battery. The In Use filter leaves these rows shown.
// Extra cell fields: inputs?, calc?
//   inputs   battery inputs (see batteryInputs) the firmware uses the parameter with. Omitted means always shown.
//   calc     "voltageDivider" | "ampsPerVolt" adds the matching calculator, which needs live battery telemetry

var maxBatteries = 3

// PX4 runs its battery estimate (cells, voltages, capacity) on power modules, ESC telemetry and filtered DroneCAN data
var _estimated = ["powerModule", "escs", "canFiltered"]
var _analog = ["powerModule"]

function _batteryRow(instance) {
    var prefix = "BAT" + instance + "_"
    return {
        id: "battery" + instance, group: qsTr("Batteries"), title: qsTr("Battery %1").arg(instance),
        instance: instance, sourceParam: prefix + "SOURCE",
        // Filtered DroneCAN data runs every battery's estimate, whatever its source
        activeWhenAny: [
            [{ param: prefix + "SOURCE", op: "ne", value: -1 }],
            [{ param: "UAVCAN_ENABLE", op: "gt", value: 0 }, { param: "UAVCAN_SUB_BAT", op: "gt", value: 1 }]
        ],
        cells: [
            { param: prefix + "SOURCE", label: qsTr("Source") },
            { param: prefix + "N_CELLS", label: qsTr("Cells in series"), inputs: _estimated },
            { param: prefix + "V_EMPTY", label: qsTr("Empty voltage per cell"), inputs: _estimated,
              note: qsTr("Keep above the steep drop-off, about 3.5 V for LiPo.") },
            { param: prefix + "V_CHARGED", label: qsTr("Full voltage per cell"), inputs: _estimated,
              note: qsTr("Slightly below the true full voltage gives a better estimate.") },
            { param: prefix + "CAPACITY", label: qsTr("Capacity"), inputs: _estimated,
              note: qsTr("-1 disables. Needed for charge counting and remaining flight time.") },
            { param: prefix + "R_INTERNAL", label: qsTr("Internal resistance per cell"), inputs: _estimated,
              note: qsTr("Negative estimates it in flight.") },
            { param: prefix + "I_OVERWRITE", label: qsTr("Current while disarmed"), inputs: ["idleOverwrite"],
              note: qsTr("Replaces an inaccurate no-load reading. 0 disables.") },
            { param: prefix + "V_DIV", label: qsTr("Voltage divider"), inputs: _analog, calc: "voltageDivider",
              note: qsTr("-1 uses the board default. Calculate needs a set value.") },
            { param: prefix + "A_PER_V", label: qsTr("Amps per volt"), inputs: _analog, calc: "ampsPerVolt",
              note: qsTr("-1 uses the board default. Calculate needs a set value.") },
            { param: prefix + "V_CHANNEL", label: qsTr("Voltage ADC channel"), inputs: _analog,
              note: qsTr("-1 uses the board default. -2 turns off analog voltage.") },
            { param: prefix + "I_CHANNEL", label: qsTr("Current ADC channel"), inputs: _analog,
              note: qsTr("-1 uses the board default.") },
            { param: prefix + "V_FILT", label: qsTr("Voltage filter"), inputs: _analog, note: qsTr("0 disables.") },
            { param: prefix + "I_FILT", label: qsTr("Current filter"), inputs: _analog, note: qsTr("0 disables.") }
        ]
    }
}

function _inaRow(model) {
    return {
        id: model.toLowerCase(), group: qsTr("Power Monitors"), title: model,
        description: qsTr("I2C power monitor. Reports as a Power Module battery source."),
        // Some boards start the monitors on their own when no driver is enabled, so a Power Module battery counts too
        activeWhenAny: [
            [{ param: "SENS_EN_" + model, op: "gt", value: 0 }],
            [{ param: "BAT1_SOURCE", op: "eq", value: 0 }],
            [{ param: "BAT2_SOURCE", op: "eq", value: 0 }],
            [{ param: "BAT3_SOURCE", op: "eq", value: 0 }]
        ],
        cells: [
            { param: "SENS_EN_" + model, label: qsTr("Driver") },
            { param: model + "_CURRENT", label: qsTr("Max current") },
            { param: model + "_SHUNT", label: qsTr("Shunt resistance") }
        ]
    }
}

function batteryRows() {
    var rows = []
    for (var instance = 1; instance <= maxBatteries; instance++) {
        rows.push(_batteryRow(instance))
    }

    rows.push(
        {
            id: "droneCan", group: qsTr("DroneCAN"), title: qsTr("DroneCAN battery"),
            description: qsTr("Smart batteries and power monitors on the CAN bus. Use the External battery source."),
            activeWhen: [{ param: "UAVCAN_ENABLE", op: "gt", value: 0 }, { param: "UAVCAN_SUB_BAT", op: "gt", value: 0 }],
            cells: [
                { param: "UAVCAN_ENABLE", label: qsTr("DroneCAN") },
                { param: "UAVCAN_SUB_BAT", label: qsTr("Battery data") }
            ]
        },
        {
            id: "levels", group: qsTr("All Batteries"), shared: true, title: qsTr("Charge levels"),
            description: qsTr("Remaining charge at which each battery is reported low, critical and emergency. The action is set on the Failsafes page."),
            cells: [
                { thresholdBar: ["BAT_EMERGEN_THR", "BAT_CRIT_THR", "BAT_LOW_THR"], label: qsTr("Levels") },
                { param: "BAT_LOW_THR", label: qsTr("Low") },
                { param: "BAT_CRIT_THR", label: qsTr("Critical") },
                { param: "BAT_EMERGEN_THR", label: qsTr("Emergency") }
            ]
        },
        {
            id: "flightTime", group: qsTr("All Batteries"), shared: true, title: qsTr("Flight time estimate"),
            description: qsTr("Starting point for the remaining flight time until the average current is measured in flight. Needs the battery capacity."),
            cells: [
                { param: "BAT_AVRG_CURRENT", label: qsTr("Expected current in flight") }
            ]
        },
        {
            id: "analogOffset", group: qsTr("All Batteries"), shared: true, title: qsTr("Analog current offset"),
            description: qsTr("Analog power modules only. Subtracted from the current sensor voltage before scaling."),
            cells: [
                { param: "BAT_V_OFFS_CURR", label: qsTr("Current sensor offset") }
            ]
        },

        _inaRow("INA226"),
        _inaRow("INA228"),
        _inaRow("INA238"),
        {
            id: "ina220", group: qsTr("Power Monitors"), title: "INA220",
            description: qsTr("I2C power monitor for a battery and a regulator. Reports as a Power Module battery source."),
            activeWhen: [{ param: "SENS_EN_INA220", op: "gt", value: 0 }],
            cells: [
                { param: "SENS_EN_INA220", label: qsTr("Driver") },
                { param: "INA220_CUR_BAT", label: qsTr("Battery max current") },
                { param: "INA220_SHUNT_BAT", label: qsTr("Battery shunt resistance") },
                { param: "INA220_CUR_REG", label: qsTr("Regulator max current") },
                { param: "INA220_SHUNT_REG", label: qsTr("Regulator shunt resistance") }
            ]
        },
        {
            id: "voxlpm", group: qsTr("Power Monitors"), title: qsTr("VOXL Power Module"),
            description: qsTr("ModalAI power module. Reports as a Power Module battery source."),
            cells: [
                { param: "VOXLPM_SHUNT_BAT", label: qsTr("Battery shunt resistance") },
                { param: "VOXLPM_SHUNT_REG", label: qsTr("Regulator shunt resistance") }
            ]
        }
    )

    return rows
}

/// Rows the vehicle has parameters for. See PX4ConfigRows.visibleRows.
function visibleRows(rows, paramExists) {
    return ConfigRows.visibleRows(rows, {}, paramExists)
}

/// rawValueOf(name) returns a parameter's raw value, or undefined when the vehicle does not have it
function isRowActive(row, rawValueOf) {
    return ConfigRows.isRowActive(row, rawValueOf)
}

/// PX4 1.18 applies BAT#_I_OVERWRITE in the shared battery estimate. Before that only the analog power module did.
/// An unknown firmware version assumes the newer behavior.
function overwriteInEstimator(firmwareMajor, firmwareMinor) {
    if (firmwareMajor === undefined || firmwareMajor < 0) {
        return true
    }
    return firmwareMajor > 1 || (firmwareMajor === 1 && firmwareMinor >= 18)
}

/// Inputs the firmware reads a battery from, given the raw BAT#_SOURCE, UAVCAN_SUB_BAT and UAVCAN_ENABLE values:
/// powerModule (analog or I2C), escs, external (MAVLink, DroneCAN or a driver), plus canFiltered when DroneCAN data
/// runs through the battery estimate. The DroneCAN bridge does not check BAT#_SOURCE, so a disabled battery still
/// has the filtered estimate. idleOverwrite marks a battery whose current BAT#_I_OVERWRITE replaces while disarmed.
function batteryInputs(source, canSubscription, canEnabled, estimatorOverwrite) {
    var canFiltered = canEnabled > 0 && canSubscription === 2
    var inputs = []
    switch (source) {
    case -1:
        inputs = canFiltered ? ["canFiltered"] : []
        break
    case 0:
        inputs = ["powerModule"]
        break
    case 1:
        inputs = canFiltered ? ["external", "canFiltered"] : ["external"]
        break
    case 2:
        inputs = ["escs"]
        break
    }

    var estimated = inputs.indexOf("escs") >= 0 || inputs.indexOf("canFiltered") >= 0
    if (inputs.indexOf("powerModule") >= 0 || (estimatorOverwrite && estimated)) {
        inputs.push("idleOverwrite")
    }
    return inputs
}

function cellUsed(cell, inputs) {
    if (!cell.inputs) {
        return true
    }
    return cell.inputs.some(function (input) { return inputs.indexOf(input) >= 0 })
}

/// MAVLink battery id of a battery row's telemetry, or -1 when it cannot be known. PX4 sends a battery it estimates
/// itself as id instance - 1: a power module on any battery, ESCs only on battery 1. A DroneCAN battery uses its node
/// id - 1 and a MAVLink battery 255, whichever BAT instance it is set up on.
function telemetryId(instance, inputs) {
    if (inputs.indexOf("powerModule") >= 0 || (inputs.indexOf("escs") >= 0 && instance === 1)) {
        return instance - 1
    }
    return -1
}

/// The reported telemetry ids no battery claims, in reported order. batteries: [{ instance, inputs }]
function unclaimedTelemetryIds(reportedIds, batteries) {
    var claimed = batteries.map(function (battery) { return telemetryId(battery.instance, battery.inputs) })
    return reportedIds.filter(function (id) { return claimed.indexOf(id) < 0 })
}

/// See PX4ConfigRows.allParams
function allParams(rows) {
    return ConfigRows.allParams(rows)
}

/// Parameter names of the row's cells that are used with these inputs, in cell order
function usedParams(row, inputs) {
    return ConfigRows.usedParams(row, function (cell) { return cellUsed(cell, inputs) })
}

/// Text for a source the firmware does not support on this battery, otherwise ""
function sourceWarning(instance, source) {
    if (source === 2 && instance !== 1) {
        return qsTr("PX4 only reads ESC telemetry into battery 1.")
    }
    return ""
}
