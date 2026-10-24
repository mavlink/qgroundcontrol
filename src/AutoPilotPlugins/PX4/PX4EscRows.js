.pragma library

// Row definitions for PX4EscComponent.qml, in the PX4ConfigRows.js schema plus:
// Row:  outputs?    "dshot" shows the row only while an output group is set to DShot
// Cell: shownWhen?  { param, value } shows the cell only while the parameter is above value

var _timerGroupCount = 8

/// Names of the output protocol parameters of every PWM output group, whether or not the vehicle has them
function timerParams() {
    var names = []
    for (var i = 0; i < _timerGroupCount; i++) {
        names.push("PWM_MAIN_TIM" + i)
        names.push("PWM_AUX_TIM" + i)
    }
    return names
}

/// protocols are the raw values of the output protocol parameters the vehicle has. Values of -1 and above are PWM or
/// OneShot, below -1 are DShot or bidirectional DShot.
function outputKinds(protocols) {
    var kinds = { known: protocols.length > 0, pwm: false, dshot: false }
    for (var i = 0; i < protocols.length; i++) {
        if (protocols[i] < -1) {
            kinds.dshot = true
        } else {
            kinds.pwm = true
        }
    }
    return kinds
}

/// PWM calibration only means something to PWM outputs. A vehicle which reports no output protocols keeps it.
function calibrationShown(kinds) {
    return !kinds.known || kinds.pwm
}

function rowShown(row, kinds) {
    return row.outputs !== "dshot" || kinds.dshot
}

function cellShown(cell, rawValueOf) {
    return cell.shownWhen === undefined || rawValueOf(cell.shownWhen.param) > cell.shownWhen.value
}

/// motorCount is how many motor outputs the vehicle has, which limits the per-motor pole count cells
function escRows(motorCount) {
    var poleCells = []
    for (var motor = 1; motor <= motorCount; motor++) {
        poleCells.push({ param: "DSHOT_MOT_POL" + motor, label: qsTr("Motor %1 poles").arg(motor) })
    }

    return [
        {
            id: "dshotOutput", group: qsTr("DShot"), title: qsTr("Output"), outputs: "dshot",
            description: qsTr("Applies to every output group set to DShot."),
            cells: [
                { param: "DSHOT_MIN", label: qsTr("Minimum output"),
                  note: qsTr("Set high enough that the motors keep spinning while armed.") },
                { param: "DSHOT_3D_ENABLE", label: qsTr("3D mode"),
                  note: qsTr("The ESC must be set to 3D mode and the minimum output to 0.") },
                { param: "DSHOT_3D_DEAD_L", label: qsTr("3D deadband low"),
                  shownWhen: { param: "DSHOT_3D_ENABLE", value: 0 } },
                { param: "DSHOT_3D_DEAD_H", label: qsTr("3D deadband high"),
                  shownWhen: { param: "DSHOT_3D_ENABLE", value: 0 } }
            ]
        },
        {
            id: "dshotTelemetry", group: qsTr("DShot"), title: qsTr("Telemetry"), outputs: "dshot",
            description: qsTr("Data the ESCs return over the DShot signal wire. Telemetry over a separate serial wire is set on the Ports page."),
            cells: [
                { param: "DSHOT_BIDIR_EN", label: qsTr("Bidirectional DShot"),
                  note: qsTr("Needs ESCs which support it, such as BLHeli32.") },
                { param: "DSHOT_BIDIR_EDT", label: qsTr("Extended DShot telemetry"),
                  note: qsTr("Reports ESC temperature, voltage and current over DShot. Needs ESCs that support it.") },
                { param: "DSHOT_ESC_TYPE", label: qsTr("ESC firmware"),
                  note: qsTr("Lets the flight controller read ESC settings over the telemetry wire.") },
                { param: "MOT_POLE_COUNT", label: qsTr("Motor poles"),
                  note: qsTr("Needed to turn the ESC's electrical RPM into motor RPM. Typical 5 inch motors have 14.") }
            ].concat(poleCells)
        },

        {
            id: "dronecanEsc", group: qsTr("DroneCAN"), title: qsTr("ESC messages"),
            description: qsTr("ESC messages are sent often at high priority, so use only the CAN interfaces which have ESCs on them."),
            cells: [
                { param: "UAVCAN_ESC_IFACE", label: qsTr("CAN interfaces") }
            ]
        }
    ]
}
