.pragma library

// Map a selected-instances array to the MAV_CMD_INJECT_FAILURE param3/param4 form:
//   All -> {p3:0, p4:0};  one instance n -> {p3:n, p4:0};  many -> {p3:NaN, p4:bitmask}.
function instanceSend(selectedInstances) {
    var sel = selectedInstances.slice().sort(function(a, b){ return a - b })
    if (sel.indexOf(0) >= 0) {
        return { param3: 0, param4: 0, label: "all" }
    }
    if (sel.length === 1) {
        return { param3: sel[0], param4: 0, label: "" + sel[0] }
    }
    var mask = 0
    for (var i = 0; i < sel.length; ++i) {
        mask |= (1 << (sel[i] - 1))
    }
    return { param3: NaN, param4: mask, label: sel.join(", ") }
}

// Toggle instance n in the selection. Picking a specific instance clears "All" (0); picking "All"
// is handled by the caller directly, this only covers the specific-instance toggle.
function toggleInstance(selectedInstances, n) {
    var arr = selectedInstances.slice()
    var zero = arr.indexOf(0)
    if (zero >= 0) { arr.splice(zero, 1) }
    var idx = arr.indexOf(n)
    if (idx >= 0) { arr.splice(idx, 1) } else { arr.push(n) }
    return arr
}

// Look up the display name for a FAILURE_UNIT value in a [{ name, unit }] catalog list.
// Falls back to the numeric value as a string if not found.
function unitName(units, unitEnum) {
    for (var i = 0; i < units.length; ++i) {
        if (units[i].unit === unitEnum) { return units[i].name }
    }
    return "" + unitEnum
}

// Look up the display name for a FAILURE_TYPE value in a [{ name, type }] catalog list.
// Falls back to the numeric value as a string if not found.
function typeName(types, typeEnum) {
    for (var i = 0; i < types.length; ++i) {
        if (types[i].type === typeEnum) { return types[i].name }
    }
    return "" + typeEnum
}
