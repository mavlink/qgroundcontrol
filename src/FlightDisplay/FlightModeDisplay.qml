/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQml

QtObject {
    id: flightModeDisplay

    function _tr(sourceText) {
        return qsTranslate("FlyViewToolStripActionList", sourceText)
    }

    function _apmMode(sourceText) {
        return qsTranslate("APMPlaneMode", sourceText)
    }

    function quadPlaneText() {
        return qsTr("四旋翼")
    }

    function quadPlaneBaseMode(mode) {
        if (!mode || mode.length === 0) {
            return ""
        }

        var quadPlaneModes = [
            { source: "QuadPlane Stabilize", base: "Stabilize" },
            { source: "QuadPlane Hover",     base: "Hover" },
            { source: "QuadPlane Loiter",    base: "Loiter" },
            { source: "QuadPlane Land",      base: "Land" },
            { source: "QuadPlane RTL",       base: "RTL" },
            { source: "QuadPlane AutoTune",  base: "AutoTune" },
            { source: "QuadPlane Acro",      base: "Acro" }
        ]

        for (var i = 0; i < quadPlaneModes.length; i++) {
            if (mode === quadPlaneModes[i].source || mode === _apmMode(quadPlaneModes[i].source)) {
                return quadPlaneModes[i].base
            }
        }

        if (mode.toLowerCase().indexOf("quadplane ") === 0) {
            return mode.substr(10)
        }

        return ""
    }

    function displayModeSource(mode) {
        var quadPlaneBase = quadPlaneBaseMode(mode)
        return quadPlaneBase !== "" ? quadPlaneBase : mode
    }

    function baseDisplayText(mode, fallbackText) {
        var displayMode = displayModeSource(mode)
        if (!displayMode || displayMode.length === 0) {
            return fallbackText !== undefined ? fallbackText : _tr("Mode")
        }

        var normalized = displayMode.toLowerCase()
        if (normalized === "manual") {
            return _tr("Manual")
        }
        if (normalized === "stabilize" || normalized === "stabilized") {
            return _tr("Stabilize")
        }
        if (normalized === "alt hold" || normalized === "althold" || normalized === "altitude hold") {
            return _tr("Alt Hold")
        }
        if (normalized === "loiter") {
            return _tr("Loiter")
        }
        if (normalized.indexOf("follow") !== -1) {
            return _tr("Follow")
        }
        if (normalized.indexOf("guided") !== -1) {
            return _tr("Guided")
        }
        if (normalized.indexOf("auto") !== -1) {
            return _tr("Auto")
        }
        if (normalized === "rtl") {
            return _tr("RTL")
        }
        if (normalized === "land") {
            return _tr("Land")
        }
        if (normalized === "takeoff") {
            return _tr("Takeoff")
        }
        if (normalized.indexOf("hold") !== -1) {
            return _tr("Hold")
        }
        if (normalized.indexOf("position") !== -1 || normalized.indexOf("pos") !== -1) {
            return _tr("Position")
        }
        if (normalized === "acro") {
            return _tr("Acro")
        }
        if (normalized === "brake") {
            return _tr("Brake")
        }
        if (normalized === "circle") {
            return _tr("Circle")
        }
        return displayMode
    }

    function displayText(mode, fallbackText) {
        if (!mode || mode.length === 0) {
            return fallbackText !== undefined ? fallbackText : _tr("Mode")
        }

        if (isQuadPlaneMode(mode)) {
            return quadPlaneText() + baseDisplayText(mode, fallbackText)
        }

        return baseDisplayText(mode, fallbackText)
    }

    function shortText(mode, fallbackText) {
        var translated = displayText(mode, fallbackText)
        return translated.length > 10 ? translated.split(" ")[0] : translated
    }

    function modeText(vehicle, mode, fallbackText) {
        return displayText(mode, fallbackText)
    }

    function shortModeText(vehicle, mode, fallbackText) {
        return shortText(mode, fallbackText)
    }

    function badgeText(modeText) {
        if (!modeText || modeText.length === 0) {
            return ""
        }
        if (modeText.length >= 4 && modeText.substr(modeText.length - 4) === "(FW)") {
            return "FW"
        }
        if (modeText.length >= 6 && modeText.substr(modeText.length - 6) === "(VTOL)") {
            return "VTOL"
        }
        return ""
    }

    function labelText(modeText) {
        var badge = badgeText(modeText)
        return badge === "" ? modeText : modeText.substr(0, modeText.length - badge.length - 2)
    }

    function baseKey(mode) {
        if (!mode || mode.length === 0) {
            return ""
        }
        return baseDisplayText(mode, "").toLowerCase()
    }

    function isQuadPlaneMode(mode) {
        return quadPlaneBaseMode(mode) !== ""
    }

    function hasQuadPlaneModes(vehicle) {
        if (!vehicle || !vehicle.flightModes) {
            return false
        }

        for (var i = 0; i < vehicle.flightModes.length; i++) {
            if (isQuadPlaneMode(vehicle.flightModes[i])) {
                return true
            }
        }
        return false
    }

    function hasQuadPlanePair(vehicle, mode) {
        if (!vehicle || !vehicle.flightModes || !mode || mode.length === 0) {
            return false
        }

        var modeBaseKey = baseKey(mode)
        for (var i = 0; i < vehicle.flightModes.length; i++) {
            if (baseKey(vehicle.flightModes[i]) === modeBaseKey && isQuadPlaneMode(vehicle.flightModes[i])) {
                return true
            }
        }
        return false
    }

    function modeTypeText(vehicle, mode) {
        return ""
    }

    function modeGroupRank(vehicle, mode) {
        if (hasQuadPlaneModes(vehicle)) {
            return isQuadPlaneMode(mode) ? 2 : 1
        }

        return 0
    }

    function sortedModes(vehicle) {
        var modes = []
        if (!vehicle || !vehicle.flightModes) {
            return modes
        }

        for (var i = 0; i < vehicle.flightModes.length; i++) {
            var mode = vehicle.flightModes[i]
            modes.push({
                mode: mode,
                index: i,
                rank: modeGroupRank(vehicle, mode)
            })
        }

        modes.sort(function(left, right) {
            if (left.rank !== right.rank) {
                return left.rank - right.rank
            }
            return left.index - right.index
        })

        var sorted = []
        for (var j = 0; j < modes.length; j++) {
            sorted.push(modes[j].mode)
        }
        return sorted
    }
}
