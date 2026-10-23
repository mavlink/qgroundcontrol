.pragma library

// Shared row logic for the PX4 setup pages built from row definitions (Failsafes, Battery, Flight Modes).
// Cells name parameters only; the control, units and choices come from the vehicle's parameter metadata.
//
// Row:  { id, group, title, description, vehicles?, activeWhen?, activeWhenAny?, cells }
// Cell: { label, param? | thresholdBar? | fixed?, note?, vehicles?, onlyIfMissing? }
//   vehicles        "multiRotor" | "fixedWing" | "vtol" names, any one matching shows the row or cell
//   activeWhen      [{ param, op: "gt" | "ge" | "ne" | "eq", value, onlyIfMissing? }], all must hold. Omitted means always active.
//                   A condition with onlyIfMissing is ignored when that parameter is on the vehicle.
//   activeWhenAny   [[condition, ...], ...], at least one list must fully hold. Combines with activeWhen.
//                   A list only counts when at least one of its parameters is on the vehicle.
//   fixed           text for behavior the firmware does not make configurable
//   onlyIfMissing   shows the cell only when this parameter is not on the vehicle
//   thresholdBar    [emergency, critical, low] battery threshold parameters
// Pages add their own fields to rows and cells.

function _matchesVehicle(vehicles, vehicle) {
    if (!vehicles) {
        return true
    }
    for (var i = 0; i < vehicles.length; i++) {
        if (vehicle[vehicles[i]]) {
            return true
        }
    }
    return false
}

/// Parameter names a cell uses
function cellParams(cell) {
    if (cell.thresholdBar) {
        return cell.thresholdBar
    }
    return cell.param ? [cell.param] : []
}

function _allExist(names, paramExists) {
    for (var i = 0; i < names.length; i++) {
        if (!paramExists(names[i])) {
            return false
        }
    }
    return true
}

function _addUnique(names, added) {
    for (var i = 0; i < added.length; i++) {
        if (names.indexOf(added[i]) < 0) {
            names.push(added[i])
        }
    }
}

/// Returns the rows to show for a vehicle. Each returned row is a copy of the row holding only the cells the vehicle
/// supports (the cell objects are shared with `rows`), plus `params`, the parameter names those cells use.
/// Rows with no parameter on the vehicle are left out.
/// vehicle: { multiRotor, fixedWing, vtol } booleans. paramExists(name) is true when the vehicle has the parameter.
function visibleRows(rows, vehicle, paramExists) {
    var visible = []

    for (var r = 0; r < rows.length; r++) {
        var row = rows[r]
        if (!_matchesVehicle(row.vehicles, vehicle)) {
            continue
        }

        var cells = []
        var params = []
        var hasParamCell = false

        for (var c = 0; c < row.cells.length; c++) {
            var cell = row.cells[c]
            if (!_matchesVehicle(cell.vehicles, vehicle)) {
                continue
            }
            if (cell.onlyIfMissing && paramExists(cell.onlyIfMissing)) {
                continue
            }
            var names = cellParams(cell)
            if (!_allExist(names, paramExists)) {
                continue
            }
            cells.push(cell)
            hasParamCell = hasParamCell || names.length > 0
            _addUnique(params, names)
        }

        if (!hasParamCell) {
            continue
        }

        visible.push(Object.assign({}, row, { warnOnly: row.warnOnly === true, cells: cells, params: params }))
    }

    return visible
}

function _conditionsHold(conditions, rawValueOf, requireAvailable) {
    var available = 0

    for (var i = 0; i < conditions.length; i++) {
        var condition = conditions[i]
        if (condition.onlyIfMissing && rawValueOf(condition.onlyIfMissing) !== undefined) {
            continue
        }
        var value = rawValueOf(condition.param)
        // Nothing can turn the row off through a parameter the vehicle lacks
        if (value === undefined) {
            continue
        }
        available++
        if (condition.op === "gt" && !(value > condition.value)) {
            return false
        }
        if (condition.op === "ge" && !(value >= condition.value)) {
            return false
        }
        if (condition.op === "ne" && value === condition.value) {
            return false
        }
        if (condition.op === "eq" && value !== condition.value) {
            return false
        }
    }

    return !requireAvailable || available > 0
}

/// rawValueOf(name) returns a parameter's raw value, or undefined when the vehicle does not have it
function isRowActive(row, rawValueOf) {
    if (!_conditionsHold(row.activeWhen || [], rawValueOf)) {
        return false
    }

    var alternatives = row.activeWhenAny
    if (!alternatives) {
        return true
    }
    for (var i = 0; i < alternatives.length; i++) {
        if (_conditionsHold(alternatives[i], rawValueOf, true)) {
            return true
        }
    }
    return false
}

/// True when the row can be inactive, so a show-active filter can hide it. Rows without conditions are always shown.
function isOptionalRow(row) {
    return row.activeWhen !== undefined || row.activeWhenAny !== undefined
}

/// Every parameter name the rows use, each once. PX4 leaves parameters no running module looked up out of the
/// initial download, so a page can request these by name.
function allParams(rows) {
    var params = []
    for (var r = 0; r < rows.length; r++) {
        for (var c = 0; c < rows[r].cells.length; c++) {
            _addUnique(params, cellParams(rows[r].cells[c]))
        }
    }
    return params
}

/// Parameter names of the row's cells that cellUsed(cell) accepts, in cell order
function usedParams(row, cellUsed) {
    var params = []
    for (var c = 0; c < row.cells.length; c++) {
        if (cellUsed(row.cells[c])) {
            _addUnique(params, cellParams(row.cells[c]))
        }
    }
    return params
}

/// Splits rows into [{ name, rows }] runs of consecutive rows with the same group
function groupRows(rows) {
    var groups = []
    for (var r = 0; r < rows.length; r++) {
        var row = rows[r]
        if (groups.length === 0 || groups[groups.length - 1].name !== row.group) {
            groups.push({ "name": row.group, "rows": [] })
        }
        groups[groups.length - 1].rows.push(row)
    }
    return groups
}
