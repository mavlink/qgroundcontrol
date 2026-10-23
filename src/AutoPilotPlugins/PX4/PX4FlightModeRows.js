.pragma library

// Row definitions and RC math for PX4FlightModes.qml. Rows and cells use the PX4ConfigRows.js schema, plus:
// Row:  kind?          "method" (mode selection method), "slots" (mode slots) or "switch". These rows add their own
//                      controls; plain parameter rows have no kind.
//       channelParam   (switch) channel mapping parameter
//       thresholdParam (switch) on threshold. Omitted uses fixedThreshold.
//       fixedThreshold (switch) threshold the firmware uses when it has no parameter for it
//       midThresholdParam (switch) mid threshold of a three-position switch
// Cell: customUi?      the row's own controls show the parameter, so it gets no generic cell

var modeSlotCount = 6

var _selectionChannelParam = "RC_MAP_FLTMODE"
var _selectionButtonsParam = "RC_MAP_FLTM_BTN"

function _switchRow(id, title, description, channelParam, thresholdParam, extraCells, extra) {
    var cells = [{ param: channelParam, label: qsTr("Channel") }]
    if (thresholdParam) {
        cells.push({ param: thresholdParam, label: qsTr("Threshold"),
                     note: qsTr("0 to 1 across the channel range. On above it, or below it when negative.") })
    }
    var row = {
        id: id, group: qsTr("Switches"), kind: "switch", title: title, description: description,
        channelParam: channelParam, thresholdParam: thresholdParam,
        activeWhen: [{ param: channelParam, op: "gt", value: 0 }],
        cells: cells.concat(extraCells || [])
    }
    return Object.assign(row, extra || {})
}

function flightModeRows() {
    var slotCells = []
    for (var slot = 1; slot <= modeSlotCount; slot++) {
        slotCells.push({ param: "COM_FLTMODE" + slot, label: qsTr("Slot %1").arg(slot), customUi: true })
    }

    return [
        {
            id: "selection", group: qsTr("Mode Selection"), kind: "method", title: qsTr("Selection method"),
            description: qsTr("A switch channel picks one of six slots by its position. Momentary buttons pick a slot each, lowest channel first. Only one method can be used."),
            cells: [
                { param: _selectionChannelParam, label: qsTr("Mode channel"), customUi: true },
                { param: _selectionButtonsParam, label: qsTr("Mode buttons"), customUi: true }
            ]
        },
        {
            id: "slots", group: qsTr("Mode Selection"), kind: "slots", title: qsTr("Mode slots"),
            description: qsTr("The flight mode each slot selects."),
            cells: slotCells
        },
        {
            id: "bootMode", group: qsTr("Mode Selection"), title: qsTr("Mode on boot"),
            description: qsTr("Mode set at power-up, before any RC input or mode command arrives."),
            cells: [
                { param: "COM_FLTMODE_BOOT", label: qsTr("Mode") }
            ]
        },

        _switchRow("return", qsTr("Return"), qsTr("Switches to Return while on."), "RC_MAP_RETURN_SW", "RC_RETURN_TH"),
        _switchRow("loiter", qsTr("Hold"), qsTr("Switches to Hold while on."), "RC_MAP_LOITER_SW", "RC_LOITER_TH"),
        _switchRow("offboard", qsTr("Offboard"), qsTr("Switches to Offboard while on."), "RC_MAP_OFFB_SW", "RC_OFFB_TH"),
        _switchRow("arm", qsTr("Arm"),
                   qsTr("Arms and disarms from a switch, or from a momentary button held down."),
                   "RC_MAP_ARM_SW", "RC_ARMSWITCH_TH",
                   [{ param: "COM_ARM_SWISBTN", label: qsTr("Momentary button") },
                    { param: "COM_RC_ARM_HYST", label: qsTr("Button hold time"),
                      note: qsTr("Also the arm gesture's hold time.") }]),
        _switchRow("kill", qsTr("Kill"),
                   qsTr("Stops the motors at once. Turning the switch off restores them."),
                   "RC_MAP_KILL_SW", "RC_KILLSWITCH_TH",
                   [{ param: "COM_KILL_DISARM", label: qsTr("Disarm after"),
                      note: qsTr("Disarms if the switch stays on this long.") }]),
        _switchRow("termination", qsTr("Flight termination"),
                   qsTr("Turns off all outputs and triggers MAVLink parachutes. Unlike Kill, it cannot be undone until the vehicle reboots."),
                   "RC_MAP_TERM_SW", "", [], { fixedThreshold: 0.75, irreversible: true }),
        _switchRow("transition", qsTr("VTOL transition"), qsTr("On flies fixed-wing, off flies multicopter."),
                   "RC_MAP_TRANS_SW", "RC_TRANS_TH", [], { vehicles: ["vtol"] }),
        _switchRow("gear", qsTr("Landing gear"), qsTr("Retracts the landing gear while on."), "RC_MAP_GEAR_SW", "RC_GEAR_TH"),
        _switchRow("payload", qsTr("Payload power"), qsTr("Three positions: off, mid and on."),
                   "RC_MAP_PAY_SW", "RC_PAYLOAD_TH",
                   [{ param: "RC_PAYLOAD_MIDTH", label: qsTr("Mid threshold") }],
                   { midThresholdParam: "RC_PAYLOAD_MIDTH" }),
        _switchRow("flaps", qsTr("Flaps"), qsTr("Sets the flaps from the channel position."),
                   "RC_MAP_FLAPS", "", [], { vehicles: ["fixedWing", "vtol"] }),

        {
            id: "armGesture", group: qsTr("Stick Gestures"), title: qsTr("Arm / disarm gesture"),
            description: qsTr("Throttle low with yaw right arms, throttle low with yaw left disarms. Roll and pitch must stay centered."),
            activeWhen: [{ param: "MAN_ARM_GESTURE", op: "gt", value: 0 }],
            cells: [
                { param: "MAN_ARM_GESTURE", label: qsTr("Gesture") },
                { param: "COM_RC_ARM_HYST", label: qsTr("Hold time"),
                  note: qsTr("Also how long a momentary arm button is held.") },
                { param: "COM_DISARM_MAN", label: qsTr("Disarm in manual flight"), vehicles: ["multiRotor", "vtol"] }
            ]
        },
        {
            id: "killGesture", group: qsTr("Stick Gestures"), title: qsTr("Kill gesture"),
            description: qsTr("Throttle low, yaw left, pitch back and roll right, all held together, stop the motors. It cannot be undone in flight."),
            activeWhen: [{ param: "MAN_KILL_GEST_T", op: "gt", value: 0 }],
            cells: [
                { param: "MAN_KILL_GEST_T", label: qsTr("Hold time"), note: qsTr("Negative disables.") }
            ]
        },

        {
            id: "slowLimits", group: qsTr("Position Slow Mode"), title: qsTr("Speed limits"),
            description: qsTr("Limits in Position Slow mode when no knob or MAVLink limit sets them."),
            vehicles: ["multiRotor", "vtol"],
            cells: [
                { param: "MC_SLOW_DEF_HVEL", label: qsTr("Horizontal speed") },
                { param: "MC_SLOW_DEF_VVEL", label: qsTr("Vertical speed") },
                { param: "MC_SLOW_DEF_YAWR", label: qsTr("Yaw rate") }
            ]
        },
        {
            id: "auxInputs", group: qsTr("Position Slow Mode"), title: qsTr("AUX inputs"),
            description: qsTr("RC channel each AUX input reads, for the speed knobs below."),
            vehicles: ["multiRotor", "vtol"],
            activeWhenAny: [
                [{ param: "RC_MAP_AUX1", op: "gt", value: 0 }], [{ param: "RC_MAP_AUX2", op: "gt", value: 0 }],
                [{ param: "RC_MAP_AUX3", op: "gt", value: 0 }], [{ param: "RC_MAP_AUX4", op: "gt", value: 0 }],
                [{ param: "RC_MAP_AUX5", op: "gt", value: 0 }], [{ param: "RC_MAP_AUX6", op: "gt", value: 0 }]
            ],
            cells: [
                { param: "RC_MAP_AUX1", label: qsTr("AUX1") },
                { param: "RC_MAP_AUX2", label: qsTr("AUX2") },
                { param: "RC_MAP_AUX3", label: qsTr("AUX3") },
                { param: "RC_MAP_AUX4", label: qsTr("AUX4") },
                { param: "RC_MAP_AUX5", label: qsTr("AUX5") },
                { param: "RC_MAP_AUX6", label: qsTr("AUX6") }
            ]
        },
        {
            id: "slowKnobs", group: qsTr("Position Slow Mode"), title: qsTr("Speed knobs"),
            description: qsTr("AUX inputs that scale a speed in flight, from its minimum up to the normal manual limit. Speed limits only apply when no knob is assigned."),
            vehicles: ["multiRotor", "vtol"],
            activeWhenAny: [
                [{ param: "MC_SLOW_MAP_HVEL", op: "gt", value: 0 }],
                [{ param: "MC_SLOW_MAP_VVEL", op: "gt", value: 0 }],
                [{ param: "MC_SLOW_MAP_YAWR", op: "gt", value: 0 }],
                [{ param: "MC_SLOW_MAP_PTCH", op: "gt", value: 0 }]
            ],
            cells: [
                { param: "MC_SLOW_MAP_HVEL", label: qsTr("Horizontal speed from") },
                { param: "MC_SLOW_MIN_HVEL", label: qsTr("Min horizontal speed") },
                { param: "MC_SLOW_MAP_VVEL", label: qsTr("Vertical speed from") },
                { param: "MC_SLOW_MIN_VVEL", label: qsTr("Min vertical speed") },
                { param: "MC_SLOW_MAP_YAWR", label: qsTr("Yaw rate from") },
                { param: "MC_SLOW_MIN_YAWR", label: qsTr("Min yaw rate") },
                { param: "MC_SLOW_MAP_PTCH", label: qsTr("Gimbal pitch rate from") }
            ]
        },

        {
            id: "stickOverride", group: qsTr("Pilot Takeover"), title: qsTr("Stick override"),
            description: qsTr("Moving the sticks in an automatic mode switches to Position, or Altitude without a position estimate."),
            vehicles: ["multiRotor", "vtol"],
            activeWhen: [{ param: "COM_RC_OVERRIDE", op: "gt", value: 0 }, { param: "MAN_OVERRIDE_SPD", op: "ge", value: 0 }],
            cells: [
                { param: "COM_RC_OVERRIDE", label: qsTr("Override in") },
                { param: "COM_RC_STICK_OV", label: qsTr("Stick threshold") },
                { param: "MAN_OVERRIDE_SPD", label: qsTr("Stick speed"),
                  note: qsTr("Negative disables. At 1, moving a stick half its range in about half a second takes over.") }
            ]
        }
    ]
}

/// RC channel value scaled the way PX4 does: -1 at RC#_MIN, 0 at RC#_TRIM, 1 at RC#_MAX, clamped, and negated when
/// the channel is reversed
function normalizedChannel(pwm, min, trim, max, reversed) {
    var value
    if (pwm <= min) {
        value = -1
    } else if (pwm >= max) {
        value = 1
    } else if (pwm < trim) {
        value = (pwm - trim) / (trim - min)
    } else if (pwm > trim) {
        value = (pwm - trim) / (max - trim)
    } else {
        value = 0
    }
    if (!isFinite(value)) {
        value = 0
    }
    return reversed ? -value : value
}

// PX4's mode slot mapping. The scale runs a little past -1 and 1, so the slots are not quite evenly spread.
var _slotMin = -1.05
var _slotMax = 1.05
var _slotWidthHalf = 1 / modeSlotCount

/// Mode slot (1-6) a normalized mode channel value selects
function modeSlot(value) {
    var slot = Math.floor((((value - _slotMin) * modeSlotCount) + _slotWidthHalf) / (_slotMax - _slotMin) + _slotWidthHalf) + 1
    return Math.max(1, Math.min(modeSlotCount, slot))
}

/// Normalized values where each slot after the first starts, ascending
function slotBoundaries() {
    var boundaries = []
    for (var n = 1; n < modeSlotCount; n++) {
        boundaries.push((((n - _slotWidthHalf) * (_slotMax - _slotMin)) - _slotWidthHalf) / modeSlotCount + _slotMin)
    }
    return boundaries
}

/// Position (0-1) of a normalized mode channel value along a strip of equal-width slots. The value is placed
/// proportionally within its own slot's range.
function slotStripPosition(value) {
    var bounds = [-1].concat(slotBoundaries()).concat([1])
    var clamped = Math.max(-1, Math.min(1, value))
    var slot = modeSlot(clamped)
    var low = bounds[slot - 1]
    var high = bounds[slot]
    var fraction = high > low ? (clamped - low) / (high - low) : 0
    return (slot - 1 + Math.max(0, Math.min(1, fraction))) / modeSlotCount
}

/// PWM range of each mode slot: [{ low, high }] in slot order, both ends inclusive. low or high is null where the slot
/// runs to the end of the channel. Found by stepping through each µs, so the ends always match modeSlot. Empty when the
/// calibration leaves a slot unreachable.
function slotPwmRanges(min, trim, max, reversed) {
    var first = []
    var last = []
    var lowestSlot = 0
    var highestSlot = 0
    for (var pwm = Math.ceil(min); pwm <= Math.floor(max); pwm++) {
        var slot = modeSlot(normalizedChannel(pwm, min, trim, max, reversed))
        if (first[slot] === undefined) {
            first[slot] = pwm
        }
        last[slot] = pwm
        if (lowestSlot === 0) {
            lowestSlot = slot
        }
        highestSlot = slot
    }
    var ranges = []
    for (var n = 1; n <= modeSlotCount; n++) {
        if (first[n] === undefined) {
            return []
        }
        ranges.push({ low: n === lowestSlot ? null : first[n], high: n === highestSlot ? null : last[n] })
    }
    return ranges
}

/// Position of a switch from its normalized channel value: "on", "mid" or "off". A negative threshold turns on below
/// it. midThreshold is only given for three-position switches.
function switchPosition(value, onThreshold, midThreshold) {
    var position = 0.5 * value + 0.5
    if (midThreshold === undefined) {
        if (onThreshold < 0) {
            position = -position
        }
        return position > onThreshold ? "on" : "off"
    }

    var onInverted = onThreshold < 0
    var midInverted = midThreshold < 0
    if (onInverted ? position < -midThreshold : position > onThreshold) {
        return "on"
    }
    if (midInverted ? position < -onThreshold : position > midThreshold) {
        return "mid"
    }
    return "off"
}

/// RC channels (1-based) of the mode buttons in RC_MAP_FLTM_BTN, lowest first. Channel N selects slot N in the list.
/// PX4 uses at most six.
function buttonChannels(mask) {
    return _maskChannels(mask).slice(0, modeSlotCount)
}

/// Channels set in RC_MAP_FLTM_BTN that PX4 ignores, because only the lowest six get a slot
function ignoredButtonChannels(mask) {
    return _maskChannels(mask).slice(modeSlotCount)
}

function _maskChannels(mask) {
    var channels = []
    for (var bit = 0; bit < 18; bit++) {
        if (mask & (1 << bit)) {
            channels.push(bit + 1)
        }
    }
    return channels
}

/// Mode slot (1-6) of the first mode button held down, 0 for none. valueOf(channel) returns the normalized channel
/// value, or undefined without one.
function buttonSlot(mask, valueOf) {
    var channels = buttonChannels(mask)
    for (var i = 0; i < channels.length; i++) {
        var value = valueOf(channels[i])
        if (value !== undefined && value > 0.5) {
            return i + 1
        }
    }
    return 0
}

/// Mode slot to show after an RC update with mode buttons: the button held down, else the last one pressed, since a
/// released button leaves its mode selected. 0 once no button channel has input.
function nextButtonSlot(previous, mask, valueOf) {
    var channels = buttonChannels(mask)
    var hasInput = channels.some(function (channel) { return valueOf(channel) !== undefined })
    if (!hasInput) {
        return 0
    }
    var pressed = buttonSlot(mask, valueOf)
    return pressed > 0 ? pressed : previous
}

/// "switch", "buttons" or "none" from RC_MAP_FLTMODE and RC_MAP_FLTM_BTN. PX4 ignores the buttons while a mode channel
/// is set.
function selectionMethod(modeChannel, buttonMask) {
    if (modeChannel > 0) {
        return "switch"
    }
    return buttonMask > 0 ? "buttons" : "none"
}

/// Parameter writes [{ param, value }] that turn off whatever the other methods use
function selectionWrites(method, modeChannel, buttonMask) {
    var writes = []
    if (method !== "switch" && modeChannel > 0) {
        writes.push({ param: _selectionChannelParam, value: 0 })
    }
    if (method !== "buttons" && buttonMask > 0) {
        writes.push({ param: _selectionButtonsParam, value: 0 })
    }
    return writes
}

/// Part of the channel range, as a position from 0 (RC#_MIN) to 1 (RC#_MAX), where a switch is not off. Matches
/// switchPosition. midThreshold is only given for three-position switches.
function switchOnRange(onThreshold, midThreshold) {
    function above(threshold) {
        return threshold < 0 ? { low: 0, high: -threshold } : { low: threshold, high: 1 }
    }
    if (midThreshold === undefined) {
        return above(onThreshold)
    }
    // With mixed-sign thresholds one of the two ranges is empty
    var ranges = [
        onThreshold < 0 ? { low: 0, high: -midThreshold } : { low: onThreshold, high: 1 },
        midThreshold < 0 ? { low: 0, high: -onThreshold } : { low: midThreshold, high: 1 }
    ].filter(function (range) { return range.high > range.low })
    if (ranges.length === 0) {
        return { low: 1, high: 0 }
    }
    return {
        low: Math.min.apply(null, ranges.map(function (range) { return range.low })),
        high: Math.max.apply(null, ranges.map(function (range) { return range.high }))
    }
}

/// Channels where functions can be on at the same time: [{ channel, users }] in channel order, users in assignment
/// order. assignments: [{ name, channel, range? }], range from switchOnRange; without one the function uses the whole
/// channel, like a stick. A channel of 0 or less is unassigned. Switches sharing a channel are fine when their ranges
/// do not overlap, such as one on when the switch is low and the other when it is high.
function channelConflicts(assignments) {
    var byChannel = {}
    for (var i = 0; i < assignments.length; i++) {
        var channel = assignments[i].channel
        if (!(channel > 0)) {
            continue
        }
        if (!byChannel[channel]) {
            byChannel[channel] = []
        }
        byChannel[channel].push(assignments[i])
    }

    var conflicts = []
    Object.keys(byChannel).map(Number).sort(function (a, b) { return a - b }).forEach(function (channel) {
        var shared = byChannel[channel]
        var conflicting = shared.map(function () { return false })
        for (var a = 0; a < shared.length; a++) {
            for (var b = a + 1; b < shared.length; b++) {
                var rangeA = shared[a].range || { low: 0, high: 1 }
                var rangeB = shared[b].range || { low: 0, high: 1 }
                if (Math.max(rangeA.low, rangeB.low) < Math.min(rangeA.high, rangeB.high)) {
                    conflicting[a] = true
                    conflicting[b] = true
                }
            }
        }
        var users = []
        shared.forEach(function (assignment, index) {
            if (conflicting[index] && users.indexOf(assignment.name) < 0) {
                users.push(assignment.name)
            }
        })
        if (users.length > 1) {
            conflicts.push({ channel: channel, users: users })
        }
    })
    return conflicts
}

/// External mode number (1-8) a mode parameter value selects, 0 for a built-in mode. COM_FLTMODE_BOOT uses PX4's
/// navigation state numbers, the mode slots their own.
function externalModeIndex(paramName, value) {
    var first = paramName === "COM_FLTMODE_BOOT" ? 23 : 100
    var index = value - first + 1
    return (index >= 1 && index <= 8) ? index : 0
}

/// Choices [{ label, value }] for a mode parameter. An external mode is offered under the name the vehicle reports
/// for it, and left out when nothing is registered in that slot, unless currentValue already selects it.
/// externalNames[n - 1] is External Mode n's name, empty if unknown.
function modeChoices(paramName, enumStrings, enumValues, externalNames, currentValue) {
    var choices = []
    for (var i = 0; i < enumStrings.length; i++) {
        var value = enumValues[i]
        var external = externalModeIndex(paramName, value)
        if (external === 0) {
            choices.push({ label: enumStrings[i], value: value })
            continue
        }
        var name = externalNames ? externalNames[external - 1] : ""
        if (name) {
            choices.push({ label: name, value: value })
        } else if (Number(value) === Number(currentValue)) {
            choices.push({ label: qsTr("%1 (not running)").arg(enumStrings[i]), value: value })
        }
    }
    return choices
}

/// Label of the mode a parameter value selects, as modeChoices shows it. Empty for a value with no choice.
function modeName(paramName, enumStrings, enumValues, externalNames, value) {
    var choices = modeChoices(paramName, enumStrings, enumValues, externalNames, value)
    for (var i = 0; i < choices.length; i++) {
        if (Number(choices[i].value) === Number(value)) {
            return choices[i].label
        }
    }
    return ""
}
