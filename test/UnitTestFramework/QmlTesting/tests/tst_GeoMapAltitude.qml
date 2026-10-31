import QtQuick
import QtTest

import "../../../../src/GeoMap/GeoMapAltitude.js" as GeoMapAltitude

/// Tests where GeoMap draws a mission item while its terrain query is pending, and where a dragged marker lands.
TestCase {
    name: "GeoMapAltitudeTest"

    function test_displayAmsl_data() {
        return [
            { tag: "knownAltitudeIsKept", amsl: 250, agl: 50, terrainHeight: 100, terrainBias: 0, expected: 250 },
            { tag: "knownAltitudeIgnoresBias", amsl: 250, agl: 50, terrainHeight: 100, terrainBias: 10, expected: 250 },
            { tag: "pendingStandsAboveTerrain", amsl: NaN, agl: 50, terrainHeight: 100, terrainBias: 0, expected: 150 },
            { tag: "pendingRemovesRenderBias", amsl: NaN, agl: 50, terrainHeight: 100, terrainBias: 10, expected: 140 },
            { tag: "pendingWithoutAglStandsOnTerrain", amsl: NaN, agl: NaN, terrainHeight: 100, terrainBias: 0, expected: 100 },
            { tag: "pendingWithoutTerrainUsesAgl", amsl: NaN, agl: 50, terrainHeight: NaN, terrainBias: 0, expected: 50 },
            { tag: "pendingWithoutAnythingIsSeaLevel", amsl: NaN, agl: NaN, terrainHeight: NaN, terrainBias: 0, expected: 0 },
        ]
    }

    function test_displayAmsl(data) {
        compare(GeoMapAltitude.displayAmsl(data.amsl, data.agl, data.terrainHeight, data.terrainBias), data.expected)
    }

    // Camera looking down at 45 degrees: the cursor at screen x is over ground position x - z at height z
    function _camera(maxValidZ) {
        return {
            coordinateAtScreenPoint: function (target, z) {
                return { isValid: z <= maxValidZ, x: target.x - z, z: z }
            }
        }
    }

    // A step from low ground (x < 50) up to 100 m
    function _stepHeight(coordinate) {
        return coordinate.x < 50 ? 0 : 100
    }

    function test_coordinateUnderCursor_data() {
        return [
            { tag: "constantHeightPicksAtTheMarkerHeight", heightAt: null, maxValidZ: 1000, x: 200, z: 0 },
            { tag: "markerMovesToTheHeightUnderTheCursor", heightAt: _stepHeight, maxValidZ: 1000, x: 100, z: 100 },
            { tag: "unpickableHeightKeepsTheLastPick", heightAt: _stepHeight, maxValidZ: 50, x: 200, z: 0 },
        ]
    }

    function test_coordinateUnderCursor(data) {
        const coordinate = GeoMapAltitude.coordinateUnderCursor(_camera(data.maxValidZ), Qt.point(200, 0), 0, 1, data.heightAt)
        compare(coordinate.x, data.x)
        compare(coordinate.z, data.z)
    }
}
