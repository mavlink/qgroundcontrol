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
        return qsTranslate("FlightModeDisplay", sourceText)
    }

    property var _modeContexts: [ "APMPlaneMode", "APMCopterMode", "APMCustomMode" ]

    property var _quadPlaneModes: [
        { source: "QuadPlane Stabilize", base: "Stabilize" },
        { source: "QuadPlane Hover",     base: "Hover" },
        { source: "QuadPlane Loiter",    base: "Loiter" },
        { source: "QuadPlane Land",      base: "Land" },
        { source: "QuadPlane RTL",       base: "RTL" },
        { source: "QuadPlane AutoTune",  base: "Autotune", aliases: [ "QuadPlane Autotune" ] },
        { source: "QuadPlane Acro",      base: "Acro" }
    ]

    property var _modeEntries: [
        { source: "Manual",             label: "Manual" },
        { source: "Circle",             label: "Circle" },
        { source: "Stabilize",          label: "Stabilize", aliases: [ "Stabilized" ] },
        { source: "Training",           label: "Training" },
        { source: "Acro",               label: "Acro" },
        { source: "Altitude Hold",      label: "Alt Hold", aliases: [ "Alt Hold", "AltHold" ] },
        { source: "FBW A",              label: "FBW A" },
        { source: "FBW B",              label: "FBW B" },
        { source: "Cruise",             label: "Cruise" },
        { source: "Auto",               label: "Auto" },
        { source: "Autotune",           label: "Autotune", aliases: [ "AutoTune", "Auto Tune" ] },
        { source: "RTL",                label: "RTL" },
        { source: "Loiter",             label: "Loiter" },
        { source: "Takeoff",            label: "Takeoff" },
        { source: "Avoid ADSB",         label: "Avoid ADSB" },
        { source: "Guided",             label: "Guided" },
        { source: "Guided No GPS",      label: "Guided No GPS" },
        { source: "Initializing",       label: "Initializing" },
        { source: "Land",               label: "Land" },
        { source: "Hover",              label: "Hover" },
        { source: "Drift",              label: "Drift" },
        { source: "Sport",              label: "Sport" },
        { source: "Flip",               label: "Flip" },
        { source: "Position Hold",      label: "Position Hold", aliases: [ "Pos Hold" ] },
        { source: "Brake",              label: "Brake" },
        { source: "Throw",              label: "Throw" },
        { source: "Smart RTL",          label: "Smart RTL" },
        { source: "Flow Hold",          label: "Flow Hold" },
        { source: "Follow",             label: "Follow" },
        { source: "Follow Target",      label: "Follow Target" },
        { source: "ZigZag",             label: "ZigZag" },
        { source: "SystemID",           label: "SystemID" },
        { source: "AutoRotate",         label: "AutoRotate", aliases: [ "Auto Rotate" ] },
        { source: "AutoRTL",            label: "AutoRTL", aliases: [ "Auto RTL" ] },
        { source: "Turtle",             label: "Turtle" },
        { source: "Thermal",            label: "Thermal" },
        { source: "Loiter to QLand",    label: "Loiter to QLand" },
        { source: "Autoland",           label: "Autoland" },
        { source: "Custom Landing",     label: "Custom Landing" },
        { source: "QuadPlane Stabilize", label: "QuadPlane Stabilize" },
        { source: "QuadPlane Hover",     label: "QuadPlane Hover" },
        { source: "QuadPlane Loiter",    label: "QuadPlane Loiter" },
        { source: "QuadPlane Land",      label: "QuadPlane Land" },
        { source: "QuadPlane RTL",       label: "QuadPlane RTL" },
        { source: "QuadPlane AutoTune",  label: "QuadPlane AutoTune", aliases: [ "QuadPlane Autotune" ] },
        { source: "QuadPlane Acro",      label: "QuadPlane Acro" }
    ]

    function _matchesMode(mode, sourceText) {
        if (mode === sourceText) {
            return true
        }

        for (var i = 0; i < _modeContexts.length; i++) {
            if (mode === qsTranslate(_modeContexts[i], sourceText)) {
                return true
            }
        }

        return false
    }

    function _matchesEntry(mode, entry) {
        if (_matchesMode(mode, entry.source)) {
            return true
        }

        var aliases = entry.aliases || []
        for (var i = 0; i < aliases.length; i++) {
            if (_matchesMode(mode, aliases[i])) {
                return true
            }
        }

        return false
    }

    function _modeEntry(mode) {
        if (!mode || mode.length === 0) {
            return null
        }

        for (var i = 0; i < _modeEntries.length; i++) {
            if (_matchesEntry(mode, _modeEntries[i])) {
                return _modeEntries[i]
            }
        }

        return null
    }

    function quadPlaneBaseMode(mode) {
        if (!mode || mode.length === 0) {
            return ""
        }

        for (var i = 0; i < _quadPlaneModes.length; i++) {
            if (_matchesEntry(mode, _quadPlaneModes[i])) {
                return _quadPlaneModes[i].base
            }
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

        var entry = _modeEntry(displayMode)
        if (entry) {
            return _tr(entry.label)
        }

        return displayMode
    }

    function displayText(mode, fallbackText) {
        if (!mode || mode.length === 0) {
            return fallbackText !== undefined ? fallbackText : _tr("Mode")
        }

        var entry = _modeEntry(mode)
        if (entry) {
            return _tr(entry.label)
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

        var entry = _modeEntry(mode)
        if (!entry) {
            return ""
        }

        var quadPlaneBase = quadPlaneBaseMode(entry.source)
        return (quadPlaneBase !== "" ? quadPlaneBase : entry.source).toLowerCase()
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
