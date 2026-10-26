.pragma library

// The sides of an orientation calibration and the order they are suggested in. Every step of the route is a single
// 90 degree tip onto an adjacent side, and upside down comes last because it is the hardest to reach. The firmware
// accepts the sides in any order, so this only decides what the page suggests next.

/// Side ids in the suggested route
var sideIds = ["level", "nose", "left", "tail", "right", "flipped"]

var _opposite = { level: "flipped", flipped: "level", nose: "tail", tail: "nose", left: "right", right: "left" }

/// The SensorsComponentController property which holds the SideCalState of each side
var _stateProperty = {
    level:   "orientationCalDownSideState",
    flipped: "orientationCalUpsideDownSideState",
    nose:    "orientationCalNoseDownSideState",
    tail:    "orientationCalTailDownSideState",
    left:    "orientationCalLeftSideState",
    right:   "orientationCalRightSideState"
}

function stateProperty(side) {
    return _stateProperty[side]
}

/// The SensorsComponentController property which says whether the calibration uses the side (a compass calibration may
/// use only some of them)
var _visibleProperty = {
    level:   "orientationCalDownSideVisible",
    flipped: "orientationCalUpsideDownSideVisible",
    nose:    "orientationCalNoseDownSideVisible",
    tail:    "orientationCalTailDownSideVisible",
    left:    "orientationCalLeftSideVisible",
    right:   "orientationCalRightSideVisible"
}

function visibleProperty(side) {
    return _visibleProperty[side]
}

var _image = {
    level:   "VehicleDown",
    flipped: "VehicleUpsideDown",
    nose:    "VehicleNoseDown",
    tail:    "VehicleTailDown",
    left:    "VehicleLeft",
    right:   "VehicleRight"
}

/// The picture of the vehicle on the side, with the turn arrow when rotating is true
function imageSource(side, rotating) {
    return "qrc:///qmlimages/" + _image[side] + (rotating ? "Rotate" : "") + ".png"
}

function sideName(side) {
    switch (side) {
    case "level":
        return qsTr("Level")
    case "nose":
        return qsTr("Nose down")
    case "left":
        return qsTr("Left side")
    case "tail":
        return qsTr("Tail down")
    case "right":
        return qsTr("Right side")
    case "flipped":
        return qsTr("Upside down")
    }
    return ""
}

function opposite(side) {
    return _opposite[side]
}

/// True when the vehicle gets from one side to the other with a single 90 degree tip
function adjacent(from, to) {
    return from !== to && _opposite[from] !== to
}

/// The side to suggest next, or "" when every side is done. current is the side the vehicle is on, or "" when it is not
/// on a side yet, and done lists the finished sides. A 90 degree tip is preferred to a 180 degree flip, and upside down is
/// left to the end where another side offers the same kind of move.
function suggestNext(current, done) {
    var best = ""
    var bestCost = Infinity
    for (var i = 0; i < sideIds.length; i++) {
        var side = sideIds[i]
        if (side === current || done.indexOf(side) >= 0) {
            continue
        }
        var cost = i
        if (current && current !== "" && !adjacent(current, side)) {
            cost += 100
        }
        if (side === "flipped") {
            cost += 10
        }
        if (cost < bestCost) {
            best = side
            bestCost = cost
        }
    }
    return best
}
