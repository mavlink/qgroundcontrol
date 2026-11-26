.pragma library

// Altitude (AMSL) to draw a mission item at. Items in a terrain frame report NaN until the terrain query for their
// coordinate answers, and a NaN altitude would be drawn at sea level, below the terrain surface. Until the query
// answers the item stands at its height above the rendered terrain instead.
//
// terrainHeight is the rendered terrain surface height at the item (NaN if unknown) and terrainBias the offset between
// the rendered surface and AMSL, which callers add back to the result. aglAltitude is the item's height above terrain.
function displayAmsl(amsl, aglAltitude, terrainHeight, terrainBias) {
    if (!isNaN(amsl)) {
        return amsl
    }
    const ground = isNaN(terrainHeight) ? 0 : terrainHeight
    return ground - terrainBias + (isNaN(aglAltitude) ? 0 : aglAltitude)
}

// Coordinate the screen point target is over, on the horizontal plane at the height of a dragged marker. startZ is the
// marker's scene height. heightAt(coordinate) is the height in meters the marker has at a position, or null when its
// height does not depend on the position. Such a marker moves to the height it has at the picked position, so the pick
// is repeated at that height.
function coordinateUnderCursor(camera, target, startZ, sceneScale, heightAt) {
    let coordinate = camera.coordinateAtScreenPoint(target, startZ)
    if (!heightAt) {
        return coordinate
    }
    for (let round = 0; round < 3 && coordinate.isValid; ++round) {
        const refined = camera.coordinateAtScreenPoint(target, heightAt(coordinate) * sceneScale)
        if (!refined.isValid) {
            break
        }
        coordinate = refined
    }
    return coordinate
}

// Height of spline sample `index` above the straight altitude ramp between the first and last samples: the curve's own
// vertical shape (FlightPathSegment::_hermiteSplinePath), 0 while an altitude is unknown. A terrain-frame spline adds
// this to its height-above-terrain ramp.
function splineVerticalShape(samples, index) {
    const first = samples[0].altitude
    const last = samples[samples.length - 1].altitude
    const shape = samples[index].altitude - (first + ((last - first) * index) / (samples.length - 1))
    return isNaN(shape) ? 0 : shape
}

// Altitude of terrain-frame spline sample `index` draped over the terrain: the height above terrain ramps between the
// leg's ends as on a straight terrain leg, with the curve's own vertical shape on top. startAltitude and endAltitude
// are the altitudes the leg's ends are drawn at, and the terrain heights are those under the two ends and under the
// sample. The spline starts at its first sample, which is above startAltitude after the vertical climb from home, so
// the ramp starts there once that altitude is known.
function drapedSplineAltitude(samples, index, startAltitude, endAltitude, startTerrain, endTerrain, sampleTerrain) {
    const first = samples[0].altitude
    const startAgl = (isNaN(first) ? startAltitude : first) - startTerrain
    const endAgl = endAltitude - endTerrain
    const fraction = index / (samples.length - 1)
    return sampleTerrain + startAgl + ((endAgl - startAgl) * fraction) + splineVerticalShape(samples, index)
}
