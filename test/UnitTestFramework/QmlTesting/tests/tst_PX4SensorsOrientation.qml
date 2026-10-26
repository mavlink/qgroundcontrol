import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/PX4SensorsOrientation.js" as Orientation
import "../../../../src/AutoPilotPlugins/PX4/PX4SensorsPose.js" as Pose

/// Tests the route of sides which SensorsCalibrationPanel.qml suggests during an accelerometer or compass calibration.
TestCase {
    name: "PX4SensorsOrientationTest"

    function _followSuggestions(start) {
        var done = start === "" ? [] : [start]
        var route = []
        var current = start
        for (var i = 0; i < Orientation.sideIds.length; i++) {
            var next = Orientation.suggestNext(current, done)
            if (next === "") {
                break
            }
            route.push(next)
            done.push(next)
            current = next
        }
        return route
    }

    function test_route_fromLevelIsTheSuggestedOrder() {
        compare(_followSuggestions(""), ["level", "nose", "left", "tail", "right", "flipped"])
        compare(_followSuggestions("level").length, 5)
    }

    function test_route_everyStepAfterTheFirstIsAOneNinetyDegreeTip() {
        var route = _followSuggestions("")
        for (var i = 1; i < route.length; i++) {
            verify(Orientation.adjacent(route[i - 1], route[i]), route[i - 1] + " to " + route[i])
        }
    }

    function test_route_upsideDownIsLast() {
        var route = _followSuggestions("")
        compare(route[route.length - 1], "flipped")
    }

    function test_suggestNext_doneWhenEverySideIsDone() {
        compare(Orientation.suggestNext("flipped", Orientation.sideIds.slice()), "")
    }

    function test_suggestNext_neverSuggestsTheCurrentOrADoneSide() {
        var sides = Orientation.sideIds
        for (var c = 0; c < sides.length; c++) {
            for (var mask = 0; mask < (1 << sides.length); mask++) {
                var done = sides.filter(function(side, i) { return (mask & (1 << i)) !== 0 })
                var next = Orientation.suggestNext(sides[c], done)
                if (next !== "") {
                    verify(done.indexOf(next) < 0 && next !== sides[c], sides[c] + " " + done + " -> " + next)
                }
            }
        }
    }

    // Wherever the vehicle starts and whichever sides are done, a tip is suggested before a flip and upside down waits
    function test_suggestNext_prefersTipsAndLeavesUpsideDownForLast() {
        var sides = Orientation.sideIds
        for (var c = 0; c < sides.length; c++) {
            for (var mask = 0; mask < (1 << sides.length); mask++) {
                var done = sides.filter(function(side, i) { return (mask & (1 << i)) !== 0 })
                var remaining = sides.filter(function(side) { return done.indexOf(side) < 0 && side !== sides[c] })
                var next = Orientation.suggestNext(sides[c], done)
                if (remaining.length === 0) {
                    compare(next, "")
                    continue
                }
                var tips = remaining.filter(function(side) { return Orientation.adjacent(sides[c], side) })
                if (tips.length > 0) {
                    verify(Orientation.adjacent(sides[c], next), "tip expected: " + sides[c] + " " + done + " -> " + next)
                }
                var others = remaining.filter(function(side) { return side !== "flipped" && Orientation.adjacent(sides[c], side) })
                if (others.length > 0) {
                    verify(next !== "flipped", "upside down too early: " + sides[c] + " " + done)
                }
            }
        }
    }

    function test_adjacent_oppositeSidesNeedAFlip_data() {
        return [
            { tag: "levelUpsideDown", a: "level", b: "flipped", adjacent: false },
            { tag: "noseTail",        a: "nose",  b: "tail",    adjacent: false },
            { tag: "leftRight",       a: "left",  b: "right",   adjacent: false },
            { tag: "levelNose",       a: "level", b: "nose",    adjacent: true },
            { tag: "rightUpsideDown", a: "right", b: "flipped", adjacent: true },
            { tag: "sameSide",        a: "left",  b: "left",    adjacent: false },
        ]
    }

    function test_adjacent_oppositeSidesNeedAFlip(data) {
        compare(Orientation.adjacent(data.a, data.b), data.adjacent)
    }

    function test_everySideHasANameAndControllerProperty() {
        Orientation.sideIds.forEach(function(side) {
            verify(Orientation.sideName(side) !== "", side)
            verify(Orientation.stateProperty(side).indexOf("orientationCal") === 0, side)
        })
    }

    function _nearly(actual, expected) {
        for (var i = 0; i < 3; i++) {
            verify(Math.abs(actual[i] - expected[i]) < 1e-6, actual + " is not " + expected)
        }
    }

    function test_pose_everySidePutsItsOwnSideDown() {
        Orientation.sideIds.forEach(function(side) {
            _nearly(Pose.rotate(Pose.sidePose(side), Pose.bodyDown(side)), [0, -1, 0])
        })
    }

    function test_pose_tipTargetKeepsTheSideDownAndTurnsTheLeast() {
        Orientation.sideIds.forEach(function(from) {
            Orientation.sideIds.forEach(function(to) {
                if (from === to) {
                    return
                }
                var current = Pose.sidePose(from)
                var target = Pose.tipTarget(current, to)
                _nearly(Pose.rotate(target, Pose.bodyDown(to)), [0, -1, 0])
                var expected = Orientation.adjacent(from, to) ? 90 : 180
                verify(Math.abs(Pose.angleBetween(current, target) - expected) < 1e-3,
                       from + " to " + to + " turns " + Pose.angleBetween(current, target))
            })
        })
    }

    function test_pose_slerpGoesTheShortWayAndEndsOnTheTarget() {
        var from = Pose.sidePose("right")
        var to = Pose.tipTarget(from, "flipped")
        var half = Pose.slerp(from, to, 0.5)
        verify(Math.abs(Pose.angleBetween(from, half) - 45) < 1e-3)
        verify(Pose.angleBetween(half, to) < 45 + 1e-3)
        verify(Pose.angleBetween(Pose.slerp(from, to, 1), to) < 1e-3)
    }

    function test_attitudePose_turnsTheVehicleTheWayItMoves() {
        var nose = [0, 0, -1]
        var rightWing = [1, 0, 0]
        _nearly(Pose.rotate(Pose.attitudePose(0, 0, 0), nose), nose)
        _nearly(Pose.rotate(Pose.attitudePose(0, 90, 0), nose), [0, 1, 0])
        _nearly(Pose.rotate(Pose.attitudePose(90, 0, 0), rightWing), [0, -1, 0])
        _nearly(Pose.rotate(Pose.attitudePose(0, 0, 90), nose), [1, 0, 0])
    }

    function test_attitudePose_matchesTheCalibrationSides() {
        verify(Pose.angleBetween(Pose.attitudePose(0, -90, 0), Pose.sidePose("nose")) < 1e-3)
        verify(Pose.angleBetween(Pose.attitudePose(0, 90, 0), Pose.sidePose("tail")) < 1e-3)
        verify(Pose.angleBetween(Pose.attitudePose(-90, 0, 0), Pose.sidePose("left")) < 1e-3)
        verify(Pose.angleBetween(Pose.attitudePose(90, 0, 0), Pose.sidePose("right")) < 1e-3)
    }

    function test_attitudePose_unknownValuesAreLevel() {
        verify(Pose.angleBetween(Pose.attitudePose(NaN, NaN, NaN), Pose.identity) < 1e-3)
    }

    function test_pose_restHeightIsTheHalfSizeOfWhateverFacesDown() {
        var half = [100, 20, 50]
        compare(Pose.restHeight(Pose.identity, half), 20)
        compare(Math.round(Pose.restHeight(Pose.sidePose("left"), half)), 100)
        compare(Math.round(Pose.restHeight(Pose.sidePose("nose"), half)), 50)
    }
}
