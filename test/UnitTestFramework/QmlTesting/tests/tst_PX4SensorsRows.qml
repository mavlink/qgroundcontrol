import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/PX4ConfigRows.js" as ConfigRows
import "../../../../src/AutoPilotPlugins/PX4/PX4SensorsRows.js" as Rows

/// Tests the row definitions and sensor logic behind SensorsSetup.qml.
TestCase {
    id: testCase
    name: "PX4SensorsRowsTest"

    readonly property var _multiRotor: ({ multiRotor: true, fixedWing: false, vtol: false })
    readonly property var _fixedWing:  ({ multiRotor: false, fixedWing: true, vtol: false })
    readonly property var _vtol:       ({ multiRotor: false, fixedWing: false, vtol: true })

    readonly property var _allParams: ConfigRows.allParams(Rows.sensorRows()).concat([
        "CAL_MAG0_ID", "CAL_MAG1_ID", "CAL_MAG0_ROT", "CAL_MAG1_ROT", "CAL_MAG0_PRIO", "CAL_MAG1_PRIO",
        "CAL_GYRO0_ID", "CAL_ACC0_ID", "CAL_BARO0_ID", "SENS_DPRES_OFF", "SENS_MAG_AUTOROT"])

    function _exists(names) {
        return function(name) { return names.indexOf(name) >= 0 }
    }

    function _values(values) {
        return function(name) { return values[name] }
    }

    function _rowById(rows, id) {
        for (var i = 0; i < rows.length; i++) {
            if (rows[i].id === id) {
                return rows[i]
            }
        }
        return null
    }

    function test_rowDefinitions_wellFormed() {
        var rows = Rows.sensorRows()
        var seen = {}
        verify(rows.length > 0)
        for (var i = 0; i < rows.length; i++) {
            var row = rows[i]
            verify(!seen[row.id], "duplicate row id " + row.id)
            seen[row.id] = true
            verify(row.title !== "", row.id + " has a title")
            verify(row.group !== "", row.id + " has a group")
            verify(row.cells.some(function(cell) { return cell.param }), row.id + " has a parameter cell")
        }
    }

    function test_calibrationRows_vehicleRows_data() {
        return [
            { tag: "compassOnMultiRotor",     row: "compass",  vehicle: _multiRotor, shown: true },
            { tag: "baroOnMultiRotor",        row: "baro",     vehicle: _multiRotor, shown: true },
            { tag: "airspeedNotOnMultiRotor", row: "airspeed", vehicle: _multiRotor, shown: false },
            { tag: "airspeedOnFixedWing",     row: "airspeed", vehicle: _fixedWing,  shown: true },
            { tag: "airspeedOnVtol",          row: "airspeed", vehicle: _vtol,       shown: true },
        ]
    }

    function test_calibrationRows_vehicleRows(data) {
        var rows = Rows.calibrationRows(data.vehicle, _exists(_allParams))
        compare(_rowById(rows, data.row) !== null, data.shown)
    }

    // A row whose sensor ID parameter is not on the vehicle cannot be calibrated, others keep their available parameters
    function test_calibrationRows_dropsMissingParameters() {
        var rows = Rows.calibrationRows(_multiRotor, _exists(["CAL_MAG0_ID", "SENS_MAG_AUTOROT", "CAL_ACC0_ID"]))
        compare(rows.map(function(row) { return row.id }), ["compass", "accel", "level"])
        compare(_rowById(rows, "compass").params, ["CAL_MAG0_ID", "SENS_MAG_AUTOROT"])
        compare(_rowById(rows, "compass").cells.map(function(cell) { return cell.param }), ["SENS_MAG_AUTOROT"])
    }

    function test_calibrationRowShown_data() {
        return [
            { tag: "compassWithCompass",      row: "compass",  values: { SYS_HAS_MAG: 1 },         shown: true },
            { tag: "compassWithoutCompass",   row: "compass",  values: { SYS_HAS_MAG: 0 },         shown: false },
            { tag: "compassParamMissing",     row: "compass",  values: {},                         shown: true },
            { tag: "gyroAlways",              row: "gyro",     values: {},                         shown: true },
            { tag: "airspeedAlways",           row: "airspeed", values: {},                         shown: true },
        ]
    }

    function test_calibrationRowShown(data) {
        var rows = Rows.calibrationRows(_fixedWing, _exists(_allParams))
        compare(Rows.calibrationRowShown(_rowById(rows, data.row), _values(data.values)), data.shown)
    }

    function test_calibrationState_data() {
        return [
            { tag: "gyroCalibrated",       row: "gyro",     values: { CAL_GYRO0_ID: 1 },       state: "ok" },
            { tag: "gyroNeeds",            row: "gyro",     values: { CAL_GYRO0_ID: 0 },       state: "needs" },
            { tag: "accelNeeds",           row: "accel",    values: { CAL_ACC0_ID: 0 },        state: "needs" },
            { tag: "compassCalibrated",    row: "compass",  values: { CAL_MAG0_ID: 197388 },   state: "ok" },
            { tag: "compassNeeds",         row: "compass",  values: { CAL_MAG0_ID: 0 },        state: "needs" },
            { tag: "airspeedCalibrated",   row: "airspeed", values: { SENS_DPRES_OFF: 1.5 },   state: "ok" },
            { tag: "airspeedNeeds",        row: "airspeed", values: { SENS_DPRES_OFF: 0 },     state: "needs" },
            { tag: "baroCalibrated",       row: "baro",     values: { CAL_BARO0_ID: 7 },       state: "ok" },
            { tag: "baroOptional",         row: "baro",     values: { CAL_BARO0_ID: 0 },       state: "optional" },
            { tag: "levelAlwaysOptional",  row: "level",    values: {},                        state: "optional" },
        ]
    }

    function test_calibrationState(data) {
        var rows = Rows.calibrationRows(_fixedWing, _exists(_allParams))
        compare(Rows.calibrationState(_rowById(rows, data.row), _values(data.values)), data.state)
    }

    function test_calibrationEnabled_levelNeedsGyroAndAccel_data() {
        return [
            { tag: "both",       values: { CAL_GYRO0_ID: 1, CAL_ACC0_ID: 1 }, enabled: true },
            { tag: "gyroOnly",   values: { CAL_GYRO0_ID: 1, CAL_ACC0_ID: 0 }, enabled: false },
            { tag: "accelOnly",  values: { CAL_GYRO0_ID: 0, CAL_ACC0_ID: 1 }, enabled: false },
            { tag: "neither",    values: { CAL_GYRO0_ID: 0, CAL_ACC0_ID: 0 }, enabled: false },
        ]
    }

    function test_calibrationEnabled_levelNeedsGyroAndAccel(data) {
        var rows = Rows.calibrationRows(_multiRotor, _exists(_allParams))
        compare(Rows.calibrationEnabled(_rowById(rows, "level"), rows, _values(data.values)), data.enabled)
        verify(Rows.calibrationEnabled(_rowById(rows, "gyro"), rows, _values(data.values)), "rows without requirements are always enabled")
    }

    function test_rowsNeedingCalibration() {
        var rows = Rows.calibrationRows(_fixedWing, _exists(_allParams))
        var values = { SYS_HAS_MAG: 1, CAL_MAG0_ID: 0, CAL_GYRO0_ID: 1, CAL_ACC0_ID: 1, CAL_BARO0_ID: 0, SENS_DPRES_OFF: 0 }
        compare(Rows.rowsNeedingCalibration(rows, _values(values)).map(function(row) { return row.id }), ["compass", "airspeed"])

        // A compass that is not required does not need calibrating
        values.SYS_HAS_MAG = 0
        compare(Rows.rowsNeedingCalibration(rows, _values(values)).map(function(row) { return row.id }), ["airspeed"])
    }

    function test_compassRows_oneRowPerCalibratedCompass() {
        var rows = Rows.compassRows(_values({ CAL_MAG0_ID: 5, CAL_MAG1_ID: 0 }), _exists(_allParams))
        compare(rows.map(function(row) { return row.id }), ["compass0"])

        rows = Rows.compassRows(_values({ CAL_MAG0_ID: 5, CAL_MAG1_ID: 9 }), _exists(_allParams))
        compare(rows.map(function(row) { return row.id }), ["compass0", "compass1"])
        compare(rows[1].params, ["CAL_MAG1_ID", "CAL_MAG1_PRIO", "CAL_MAG1_ROT"])
    }

    // The custom roll, pitch and yaw only apply when the rotation is Custom Euler Angle (100)
    function test_compassRows_customAnglesOnlyForCustomRotation_data() {
        return [
            { tag: "standardRotation", rotation: 2,   shown: false },
            { tag: "noRotation",       rotation: 0,   shown: false },
            { tag: "customAngle",      rotation: 100, shown: true },
        ]
    }

    function test_compassRows_customAnglesOnlyForCustomRotation(data) {
        var names = ["CAL_MAG1_ID", "CAL_MAG1_PRIO", "CAL_MAG1_ROT", "CAL_MAG1_ROLL", "CAL_MAG1_PITCH", "CAL_MAG1_YAW"]
        var values = { CAL_MAG1_ID: 9, CAL_MAG1_ROT: data.rotation }
        var row = Rows.compassRows(_values(values), _exists(names))[0]
        compare(row.params, names)
        var angleCells = row.cells.filter(function(cell) { return cell.label.indexOf("Custom") === 0 })
        compare(angleCells.length, 3)
        for (var i = 0; i < angleCells.length; i++) {
            compare(Rows.cellShown(angleCells[i], _values(values)), data.shown)
        }
    }

    function test_staleCustomAngles_data() {
        return [
            // PX4 sets the rotation back to custom while an angle is not zero, so those angles have to be cleared
            { tag: "standardWithAngles",   rotation: 0,   angles: [0, 10, 5], stale: ["CAL_MAG1_PITCH", "CAL_MAG1_YAW"] },
            { tag: "standardNoAngles",     rotation: 2,   angles: [0, 0, 0],  stale: [] },
            { tag: "customWithAngles",     rotation: 100, angles: [0, 10, 5], stale: [] },
            { tag: "internalCompass",      rotation: -1,  angles: [0, 10, 5], stale: [] },
        ]
    }

    function test_staleCustomAngles(data) {
        var names = ["CAL_MAG1_ID", "CAL_MAG1_PRIO", "CAL_MAG1_ROT", "CAL_MAG1_ROLL", "CAL_MAG1_PITCH", "CAL_MAG1_YAW"]
        var values = { CAL_MAG1_ID: 9, CAL_MAG1_ROT: data.rotation,
                       CAL_MAG1_ROLL: data.angles[0], CAL_MAG1_PITCH: data.angles[1], CAL_MAG1_YAW: data.angles[2] }
        var row = Rows.compassRows(_values(values), _exists(names))[0]
        compare(Rows.staleCustomAngles(row, _values(values)), data.stale)
    }

    function test_compassRows_missingParametersAreLeftOut() {
        var rows = Rows.compassRows(_values({ CAL_MAG0_ID: 5 }), _exists(["CAL_MAG0_ID", "CAL_MAG0_PRIO"]))
        compare(rows[0].params, ["CAL_MAG0_ID", "CAL_MAG0_PRIO"])
        compare(rows[0].cells.map(function(cell) { return cell.param }), ["CAL_MAG0_PRIO"])
    }

    // The firmware reports a compass built into the autopilot with a rotation of -1
    function test_compassExternal_data() {
        return [
            { tag: "internal",   rotation: -1,        external: false },
            { tag: "none",       rotation: 0,         external: true },
            { tag: "yaw90",      rotation: 2,         external: true },
            { tag: "missing",    rotation: undefined, external: false },
        ]
    }

    function test_compassExternal(data) {
        compare(Rows.compassExternal(data.rotation), data.external)
    }

    function test_hardwareDevices() {
        var values = { CAL_GYRO0_ID: 1, CAL_ACC0_ID: 1, CAL_MAG0_ID: 1, CAL_MAG1_ID: 1, CAL_BARO0_ID: 0 }
        var devices = Rows.hardwareDevices(_values(values), _exists(_allParams))
        compare(devices.map(function(device) { return device.param }),
                ["CAL_GYRO0_ID", "CAL_ACC0_ID", "CAL_MAG0_ID", "CAL_MAG1_ID"])
    }

    function test_visibleRows_airspeedRowsOnlyForFixedWingAndVtol_data() {
        return [
            { tag: "multiRotor", vehicle: _multiRotor, tubing: false, required: false },
            { tag: "fixedWing",  vehicle: _fixedWing,  tubing: true,  required: true },
            { tag: "vtol",       vehicle: _vtol,       tubing: true,  required: true },
        ]
    }

    function test_visibleRows_airspeedRowsOnlyForFixedWingAndVtol(data) {
        var rows = ConfigRows.visibleRows(Rows.sensorRows(), data.vehicle, _exists(_allParams))
        compare(_rowById(rows, "airspeedTubing") !== null, data.tubing)
        compare(_rowById(rows, "required").params.indexOf("SYS_HAS_NUM_ASPD") >= 0, data.required)
    }

    // Only the tube pressure drop model (2) uses the tube diameter
    function test_cellShown_tubeDiameter_data() {
        return [
            { tag: "modelWithPitot",    model: 0, shown: false },
            { tag: "modelWithoutPitot", model: 1, shown: false },
            { tag: "tubePressureDrop",  model: 2, shown: true },
        ]
    }

    function test_cellShown_tubeDiameter(data) {
        var cell = _rowById(Rows.sensorRows(), "airspeedTubing").cells.filter(function(cell) { return cell.param === "CAL_AIR_TUBED_MM" })[0]
        compare(Rows.cellShown(cell, _values({ CAL_AIR_CMODEL: data.model })), data.shown)
    }

    function test_rowShown_data() {
        return [
            { tag: "plainRow",                  row: "required",        fixed: false, shown: true },
            { tag: "autopilotOnFixedOrientation", row: "autopilot",     fixed: true,  shown: false },
            { tag: "autopilotEditable",         row: "autopilot",       fixed: false, shown: true },
        ]
    }

    function test_rowShown(data) {
        compare(Rows.rowShown(_rowById(Rows.sensorRows(), data.row), data.fixed), data.shown)
    }

    // Every row is on exactly one tab, and only the Advanced rows are on the Advanced tab
    function test_tabGroups_splitRowsBetweenTabs() {
        var groups = ConfigRows.groupRows(Rows.sensorRows())
        var calibration = Rows.tabGroups(groups, false)
        var advanced = Rows.tabGroups(groups, true)
        var ids = function(tabGroups) {
            return tabGroups.reduce(function(all, group) { return all.concat(group.rows.map(function(row) { return row.id })) }, [])
        }
        compare(ids(calibration).length + ids(advanced).length, Rows.sensorRows().length)
        verify(ids(advanced).length > 0)
        verify(ids(advanced).every(function(id) { return _rowById(Rows.sensorRows(), id).advanced === true }))
        verify(ids(calibration).every(function(id) { return _rowById(Rows.sensorRows(), id).advanced !== true }))
        verify(calibration.concat(advanced).every(function(group) { return group.rows.length > 0 }))
    }

    function test_gpsReadiness_data() {
        return [
            { tag: "noFix",             lock: 0, satellites: 0, accuracy: NaN, ready: false, level: "none", summary: "No fix" },
            { tag: "twoDFix",           lock: 2, satellites: 5, accuracy: NaN, ready: false, level: "none", summary: "2D fix" },
            { tag: "goodAccuracy",      lock: 3, satellites: 12, accuracy: 1.4, ready: true, level: "good", summary: "3D fix" },
            { tag: "unknownAccuracy",   lock: 3, satellites: 8, accuracy: NaN, ready: true,  level: "good", summary: "3D fix" },
            { tag: "zeroAccuracy",      lock: 3, satellites: 8, accuracy: 0,   ready: true,  level: "good", summary: "3D fix" },
            { tag: "limitIsStillGood",  lock: 3, satellites: 8, accuracy: Rows.poorGpsHeightAccuracy, ready: true, level: "good", summary: "3D fix" },
            { tag: "poorAccuracy",      lock: 4, satellites: 6, accuracy: 15.2, ready: true, level: "poor", summary: "3D fix" },
        ]
    }

    function test_gpsReadiness(data) {
        var readiness = Rows.gpsReadiness(data.lock, data.satellites, data.accuracy)
        compare(readiness.ready, data.ready)
        compare(readiness.level, data.level)
        compare(readiness.summary, data.summary)
        verify(readiness.message !== "")
    }

    function test_gpsReadiness_detail() {
        compare(Rows.gpsReadiness(3, 12, 1.44).detail, "±1.4 m vertical · 12 satellites")
        compare(Rows.gpsReadiness(3, 1, NaN).detail, "1 satellite")
    }

    function test_inlineFailureText_data() {
        return [
            { tag: "baroNoGps",       message: "calibration failed: GPS required for baro cal", expected: "No GPS data arrived. Check the GPS has a fix and try again." },
            { tag: "levelMotion",     message: "motion during calibration: calibration failed: level", expected: "The vehicle moved. Keep it still and try again." },
            { tag: "gyroMotion",      message: "ERROR: Motion during calibration: calibration failed: gyro", expected: "The vehicle moved. Keep it still and try again." },
            { tag: "gyroNone",        message: "No gyros found: calibration failed: gyro", expected: "The vehicle reports no gyroscope." },
            { tag: "gyroSaveFailed",  message: "calibration save failed: calibration failed: gyro", expected: "The calibration could not be saved. Try again." },
            { tag: "gyroStorage",     message: "ERROR: cannot open calibration storage: calibration failed: gyro", expected: "The calibration could not be saved. Try again." },
            { tag: "levelRoll",       message: "excess roll angle: calibration failed: level", expected: "The vehicle is too far from level. Place it level and try again." },
            { tag: "levelPitch",      message: "excess pitch angle: calibration failed: level", expected: "The vehicle is too far from level. Place it level and try again." },
            { tag: "levelNoEstimator", message: "attitude estimator not running - check system boot: calibration failed: level", expected: "The attitude estimator is not running." },
            { tag: "empty",           message: "", expected: "The calibration did not finish. Try again." },
            { tag: "unknown",         message: "calibration failed: baro", expected: "calibration failed: baro" },
        ]
    }

    function test_inlineFailureText(data) {
        compare(Rows.inlineFailureText(data.message), data.expected)
    }

    function test_parseHeading_data() {
        return [
            { tag: "whole",          text: "90",      expected: 90 },
            { tag: "decimal",        text: "90.5",    expected: 90.5 },
            { tag: "zero",           text: "0",       expected: 0 },
            { tag: "padded",         text: " 270 ",   expected: 270 },
            { tag: "trailingText",   text: "90abc",   expected: NaN },
            { tag: "decimalComma",   text: "90,5",    expected: NaN },
            { tag: "empty",          text: "",        expected: NaN },
            { tag: "blank",          text: "  ",      expected: NaN },
            { tag: "negative",       text: "-5",      expected: NaN },
            { tag: "hex",            text: "0x10",    expected: NaN },
        ]
    }

    function test_parseHeading(data) {
        var heading = Rows.parseHeading(data.text)
        if (isNaN(data.expected)) {
            verify(isNaN(heading), data.text)
        } else {
            compare(heading, data.expected)
        }
    }

    function test_calibrationRows_inlineRowsHaveAHintOrGps() {
        var rows = Rows.calibrationRows(_multiRotor, _exists(_allParams)).filter(function(row) { return row.inline })
        compare(rows.map(function(row) { return row.id }).sort(), ["baro", "gyro", "level"])
        rows.forEach(function(row) { verify(row.gps === true || row.hint, row.id) })
    }
}
