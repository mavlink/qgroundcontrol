.pragma library

// Row definitions and sensor logic for SensorsSetup.qml, with the parameter rows in the PX4ConfigRows.js schema plus:
// Row:  advanced?                    puts the row on the page's Advanced tab instead of the Calibration tab
//       hiddenWhenFixedOrientation?  hides the row when the vehicle's sensors have a fixed orientation
// Cell: shownWhen?                   { param, value } shows the cell only while the parameter is above value
//
// Calibration rows are not parameter rows, see calibrationRows():
// { id, title, description, calibration, idParam?, offsetParam?, optional?, requires?, shownWhen?, vehicles?, params, cells, ... }
//   idParam       the device ID parameter the firmware sets once the sensor is calibrated
//   offsetParam   a parameter the firmware sets to a non-zero offset once the sensor is calibrated
//   optional      calibrating is not needed for flight
//   requires      ids of calibration rows which must be done before this one can run
//   shownWhen     { param, op: "gt" | "ne", value, showIfMissing? } shows the row only while the condition holds
//   alternate     { type, label, note } a second calibration offered under the main one
//   inline        runs in its row, with its progress and result shown there, instead of in the calibration panel
//   gps           shows the GPS state in the row, because the calibration is matched to the GPS
//   hint          what to do before starting an inline calibration
//   progress      true when the firmware reports the progress, otherwise the row shows a bar without a value

/// The firmware has this many instances of each CAL_<SENSOR>n_* parameter
var maxSensorInstances = 4

function cellShown(cell, rawValueOf) {
    return cell.shownWhen === undefined || rawValueOf(cell.shownWhen.param) > cell.shownWhen.value
}

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

function _existing(names, paramExists) {
    return names.filter(function (name) { return paramExists(name) })
}

function _sensorParam(sensor, instance, suffix) {
    return "CAL_" + sensor + instance + "_" + suffix
}

function calibrationRows(vehicle, paramExists) {
    var rows = [
        {
            id: "compass", title: qsTr("Compass"), calibration: "compass", idParam: "CAL_MAG0_ID",
            description: qsTr("Rotate the vehicle through the orientations the vehicle asks for."),
            params: ["CAL_MAG0_ID", "SENS_MAG_AUTOROT"],
            shownWhen: { param: "SYS_HAS_MAG", op: "ne", value: 0, showIfMissing: true },
            alternate: {
                type: "compassLargeVehicle", label: qsTr("Large vehicle calibration…"),
                note: qsTr("For vehicles too large or heavy to rotate. Needs the heading and a GPS position.")
            },
            cells: [
                {
                    param: "SENS_MAG_AUTOROT", label: qsTr("Auto-detect external compass rotation"),
                    note: qsTr("Sets the rotation of an external compass when you calibrate.")
                }
            ]
        },
        {
            id: "gyro", title: qsTr("Gyroscope"), calibration: "gyro", idParam: "CAL_GYRO0_ID",
            inline: true, progress: true,
            description: qsTr("Measures the gyroscope offset while the vehicle is still."),
            hint: qsTr("Keep the vehicle still on a flat surface."),
            params: ["CAL_GYRO0_ID"], cells: []
        },
        {
            id: "accel", title: qsTr("Accelerometer"), calibration: "accel", idParam: "CAL_ACC0_ID",
            description: qsTr("Hold the vehicle on each of its six sides."),
            params: ["CAL_ACC0_ID"],
            cells: []
        },
        {
            id: "level", title: qsTr("Level horizon"), calibration: "level", optional: true,
            inline: true, progress: true,
            hint: qsTr("Place the vehicle in its level flight position and keep it still."),
            requires: ["gyro", "accel"],
            description: qsTr("Sets the attitude the autopilot treats as level. Needs the gyroscope and accelerometer done first."),
            params: ["SENS_BOARD_X_OFF", "SENS_BOARD_Y_OFF"], cells: []
        },
        {
            id: "baro", title: qsTr("Barometer"), calibration: "baro", idParam: "CAL_BARO0_ID", optional: true,
            inline: true, gps: true,
            description: qsTr("Matches the barometer to the GPS altitude. The vehicle also does this itself after boot."),
            params: ["CAL_BARO0_ID"],
            cells: []
        },
        {
            id: "airspeed", title: qsTr("Airspeed"), calibration: "airspeed", offsetParam: "SENS_DPRES_OFF",
            vehicles: ["fixedWing", "vtol"],
            description: qsTr("Cover the pitot tube, then blow across it when asked."),
            params: ["SENS_DPRES_OFF", "SYS_HAS_NUM_ASPD"],
            cells: []
        }
    ]

    return rows.filter(function (row) {
        if (!_matchesVehicle(row.vehicles, vehicle)) {
            return false
        }
        var required = row.idParam || row.offsetParam
        if (required && !paramExists(required)) {
            return false
        }
        return true
    }).map(function (row) {
        return Object.assign({}, row, {
            params: _existing(row.params, paramExists),
            cells: row.cells.filter(function (cell) { return paramExists(cell.param) })
        })
    })
}

function calibrationRowShown(row, rawValueOf) {
    var condition = row.shownWhen
    if (!condition) {
        return true
    }
    var value = rawValueOf(condition.param)
    if (value === undefined) {
        return condition.showIfMissing === true
    }
    return condition.op === "gt" ? value > condition.value : value !== condition.value
}

/// "ok" when the firmware has the sensor calibrated, "needs" when it has to be before flight, "optional" otherwise
function calibrationState(row, rawValueOf) {
    var param = row.idParam || row.offsetParam
    if (!param) {
        return "optional"
    }
    var calibrated = rawValueOf(param) !== undefined && rawValueOf(param) !== 0
    if (calibrated) {
        return "ok"
    }
    return row.optional ? "optional" : "needs"
}

function calibrationStatusText(row, state) {
    if (state === "ok") {
        return qsTr("Calibrated")
    }
    if (state === "needs") {
        return qsTr("Needs calibration")
    }
    return row.idParam ? qsTr("Not calibrated") : qsTr("Optional")
}

/// True when every calibration the row requires is done
function calibrationEnabled(row, rows, rawValueOf) {
    var required = row.requires || []
    for (var i = 0; i < required.length; i++) {
        var other = rows.filter(function (candidate) { return candidate.id === required[i] })[0]
        if (!other || calibrationState(other, rawValueOf) !== "ok") {
            return false
        }
    }
    return true
}

/// The shown rows which must be calibrated before flight
function rowsNeedingCalibration(rows, rawValueOf) {
    return rows.filter(function (row) {
        return calibrationRowShown(row, rawValueOf) && calibrationState(row, rawValueOf) === "needs"
    })
}

/// Vertical GPS error in metres above which PX4's own automatic barometer calibration would not use the GPS height
var poorGpsHeightAccuracy = 8

/// What the GPS offers the barometer calibration, which matches the barometer to the GPS altitude.
/// lock is the MAVLink GPS fix type, verticalAccuracy is in metres and NaN when the GPS does not report it.
/// Returns { ready, level: "none" | "poor" | "good", summary, detail, message }.
function gpsReadiness(lock, satellites, verticalAccuracy) {
    var satelliteCount = satellites || 0
    var satelliteText = satelliteCount === 1 ? qsTr("1 satellite") : qsTr("%1 satellites").arg(satelliteCount)
    var accuracyKnown = isFinite(verticalAccuracy) && verticalAccuracy > 0
    var accuracy = accuracyKnown ? Math.round(verticalAccuracy * 10) / 10 : 0

    if (!(lock >= 3)) {
        var twoD = lock === 2
        return {
            ready: false, level: "none", summary: twoD ? qsTr("2D fix") : qsTr("No fix"), detail: satelliteText,
            message: twoD ? qsTr("A 2D fix has no reliable height. Wait for a 3D fix.") :
                            qsTr("Needs a GPS fix. Wait for the vehicle to get one.")
        }
    }

    var detail = accuracyKnown ? qsTr("±%1 m vertical").arg(accuracy) + " · " + satelliteText : satelliteText
    if (accuracyKnown && verticalAccuracy > poorGpsHeightAccuracy) {
        return {
            ready: true, level: "poor", summary: qsTr("3D fix"), detail: detail,
            message: qsTr("GPS height is only accurate to ±%1 m. Wait for a better fix if you can.").arg(accuracy)
        }
    }
    return {
        ready: true, level: "good", summary: qsTr("3D fix"), detail: detail,
        message: qsTr("Keep the vehicle still. It takes about 3 seconds.")
    }
}

/// Plain language for why an inline calibration failed, from the firmware's failure text
function inlineFailureText(message) {
    var text = message.toLowerCase()
    if (text.indexOf("gps required") >= 0) {
        return qsTr("No GPS data arrived. Check the GPS has a fix and try again.")
    }
    if (text.indexOf("motion during calibration") >= 0) {
        return qsTr("The vehicle moved. Keep it still and try again.")
    }
    if (text.indexOf("excess roll angle") >= 0 || text.indexOf("excess pitch angle") >= 0) {
        return qsTr("The vehicle is too far from level. Place it level and try again.")
    }
    if (text.indexOf("attitude estimator not running") >= 0) {
        return qsTr("The attitude estimator is not running.")
    }
    if (text.indexOf("no gyros found") >= 0) {
        return qsTr("The vehicle reports no gyroscope.")
    }
    if (text.indexOf("save failed") >= 0 || text.indexOf("cannot open calibration storage") >= 0) {
        return qsTr("The calibration could not be saved. Try again.")
    }
    return message !== "" ? message : qsTr("The calibration did not finish. Try again.")
}

/// The heading typed in the large vehicle calibration form: a plain decimal number, NaN for anything else
function parseHeading(text) {
    var trimmed = text.trim()
    return /^\d+(\.\d+)?$/.test(trimmed) ? Number(trimmed) : NaN
}

/// A row for each compass the vehicle has, to set its priority and, for an external compass, its rotation.
/// A compass is present while its device ID is set.
function compassRows(rawValueOf, paramExists) {
    var rows = []
    for (var i = 0; i < maxSensorInstances; i++) {
        var idParam = _sensorParam("MAG", i, "ID")
        if (!paramExists(idParam) || !rawValueOf(idParam)) {
            continue
        }
        var rotationParam = _sensorParam("MAG", i, "ROT")
        var angleParams = ["ROLL", "PITCH", "YAW"].map(function (axis) { return _sensorParam("MAG", i, axis) })
        // The custom angles only apply when the rotation is Custom Euler Angle (100)
        var customAngle = function (param, label) {
            return { param: param, label: label, externalOnly: true, shownWhen: { param: rotationParam, value: 99 } }
        }
        rows.push({
            id: "compass" + i, group: qsTr("Compasses"), title: qsTr("Compass %1").arg(i),
            idParam: idParam, rotationParam: rotationParam, angleParams: _existing(angleParams, paramExists),
            params: _existing([idParam, _sensorParam("MAG", i, "PRIO"), rotationParam].concat(angleParams), paramExists),
            cells: [
                { param: _sensorParam("MAG", i, "PRIO"), label: qsTr("Priority"),
                  note: i === 0 ? qsTr("The highest priority compass is the primary one.") : "" },
                { param: rotationParam, label: qsTr("Rotation"), externalOnly: true },
                customAngle(angleParams[0], qsTr("Custom roll")),
                customAngle(angleParams[1], qsTr("Custom pitch")),
                customAngle(angleParams[2], qsTr("Custom yaw"))
            ].filter(function (cell) { return paramExists(cell.param) })
        })
    }
    return rows
}

/// The firmware uses a rotation of zero or more for an external compass and -1 for one built into the autopilot
function compassExternal(rotation) {
    return rotation !== undefined && rotation >= 0
}

/// The rotation which makes an external compass use its custom angles
var customCompassRotation = 100

/// The custom angle parameters of an external compass which has a standard rotation but angles which are not zero. PX4 sets
/// the rotation back to custom while any angle is not zero, so a standard rotation only stays once they are cleared.
function staleCustomAngles(row, rawValueOf) {
    var rotation = rawValueOf(row.rotationParam)
    if (!compassExternal(rotation) || rotation >= customCompassRotation) {
        return []
    }
    return row.angleParams.filter(function (param) {
        var value = rawValueOf(param)
        return value !== undefined && value !== 0
    })
}

/// The calibrated sensors, for the live card's hardware list
function hardwareDevices(rawValueOf, paramExists) {
    var sensors = [
        { key: "GYRO", label: qsTr("Gyroscope %1") },
        { key: "ACC", label: qsTr("Accelerometer %1") },
        { key: "MAG", label: qsTr("Compass %1") },
        { key: "BARO", label: qsTr("Barometer %1") }
    ]
    var devices = []
    for (var s = 0; s < sensors.length; s++) {
        for (var i = 0; i < maxSensorInstances; i++) {
            var param = _sensorParam(sensors[s].key, i, "ID")
            if (paramExists(param) && rawValueOf(param)) {
                devices.push({ label: sensors[s].label.arg(i), param: param })
            }
        }
    }
    return devices
}

function sensorRows() {
    return [
        {
            id: "autopilot", group: qsTr("Orientation"), title: qsTr("Autopilot"), hiddenWhenFixedOrientation: true,
            description: qsTr("How the autopilot is mounted compared to the vehicle. Tilt the vehicle and check the live view follows."),
            cells: [
                { param: "SENS_BOARD_ROT", label: qsTr("Rotation") },
                { param: "SENS_BOARD_X_OFF", label: qsTr("Roll trim") },
                { param: "SENS_BOARD_Y_OFF", label: qsTr("Pitch trim") },
                { param: "SENS_BOARD_Z_OFF", label: qsTr("Yaw trim") }
            ]
        },
        {
            id: "required", group: qsTr("Sensors"), title: qsTr("Required sensors"),
            description: qsTr("Preflight checks fail when a required sensor is missing."),
            cells: [
                { param: "SYS_HAS_MAG", label: qsTr("Compasses required"), note: qsTr("Zero skips the compass check.") },
                { param: "SYS_HAS_BARO", label: qsTr("Barometer required") },
                { param: "SENS_INT_BARO_EN", label: qsTr("Internal barometers"), note: qsTr("Turn off to use an external barometer.") },
                { param: "SYS_HAS_NUM_ASPD", label: qsTr("Airspeed sensor required"), vehicles: ["fixedWing", "vtol"] }
            ]
        },
        {
            id: "autoCalibration", group: qsTr("Advanced"), title: qsTr("Automatic calibration"), advanced: true,
            description: qsTr("Lets the vehicle calibrate sensors itself. It sets an initial gyroscope, accelerometer and compass calibration from the estimator's bias estimates when the sensor is not calibrated, and calibrates the barometer against the GNSS height."),
            cells: [
                { param: "SENS_IMU_AUTOCAL", label: qsTr("Gyroscope and accelerometer") },
                { param: "SENS_MAG_AUTOCAL", label: qsTr("Compass") },
                { param: "SENS_BAR_AUTOCAL", label: qsTr("Barometer") }
            ]
        },
        {
            id: "compassCompensation", group: qsTr("Advanced"), title: qsTr("Compass compensation"), advanced: true,
            description: qsTr("Removes the magnetic interference of the motors from the compass."),
            cells: [{ param: "CAL_MAG_COMP_TYP", label: qsTr("Compensation") }]
        },
        {
            id: "clipping", group: qsTr("Advanced"), title: qsTr("Clipping notice"), advanced: true,
            cells: [{ param: "SENS_IMU_CLPNOTI", label: qsTr("Notify when the IMU clips") }]
        },
        {
            id: "airspeedTubing", group: qsTr("Advanced"), title: qsTr("Airspeed tubing"), advanced: true,
            vehicles: ["fixedWing", "vtol"],
            description: qsTr("Compensation for an SDP3x sensor. Reverse the readings when the static and dynamic ports are swapped."),
            cells: [
                { param: "CAL_AIR_CMODEL", label: qsTr("Compensation model") },
                { param: "CAL_AIR_TUBELEN", label: qsTr("Tube length") },
                // Only the tube pressure drop model (2) uses the diameter
                { param: "CAL_AIR_TUBED_MM", label: qsTr("Tube diameter"), shownWhen: { param: "CAL_AIR_CMODEL", value: 1 } },
                { param: "SENS_DPRES_REV", label: qsTr("Reverse readings") }
            ]
        }
    ]
}

function rowShown(row, fixedOrientation) {
    return !(row.hiddenWhenFixedOrientation && fixedOrientation)
}

/// The groups of one tab of the page: the groups from PX4ConfigRows.js groupRows() with only the rows on that tab,
/// and without the groups which have none. advanced selects the Advanced tab, otherwise the Calibration tab.
function tabGroups(groups, advanced) {
    return groups.map(function (group) {
        return {
            name: group.name,
            rows: group.rows.filter(function (row) { return (row.advanced === true) === advanced })
        }
    }).filter(function (group) { return group.rows.length > 0 })
}
