import QtQuick
import QtTest

import "../../../../src/GeoMap/GeoMapAltitude.js" as GeoMapAltitude

/// Tests the altitude GeoMap draws a mission item at while its terrain query is pending.
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
}
