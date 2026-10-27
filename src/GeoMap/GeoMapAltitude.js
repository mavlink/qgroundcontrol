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
