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
