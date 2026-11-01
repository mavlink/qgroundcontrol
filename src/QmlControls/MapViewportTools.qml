pragma Singleton

import QtQuick

/// Helpers for the part of a map that Plan View tools leave free: the map's centerViewport, or the whole map when it
/// has none. They work with any map that has toCoordinate(), QtLocation or GeoMap.
QtObject {
    /// The part of the map in view, in map coordinates
    function viewportRect(map) {
        return map.centerViewport ?? Qt.rect(0, 0, map.width, map.height)
    }

    // A GeoMap viewport corner can point into the sky in a 3D tilt: moves it toward the center until it meets the
    // ground, null if no point does
    function _groundCoordinateInViewport(map, corner, center) {
        for (let step = 0; step < 10; step++) {
            const fraction = step / 10
            const coordinate = map.toCoordinate(
                Qt.point(corner.x + (center.x - corner.x) * fraction, corner.y + (center.y - corner.y) * fraction),
                false /* clipToViewPort */)
            if (coordinate.isValid) {
                return coordinate
            }
        }
        return null
    }

    function _normalizedLongitude(longitude) {
        let normalized = longitude
        while (normalized > 180) {
            normalized -= 360
        }
        while (normalized < -180) {
            normalized += 360
        }
        return normalized
    }

    // Whether point is inside the convex quadrilateral quad, edges included; a point sits on an edge within a tolerance
    // of about a centimeter (the points are {x: longitude offset, y: latitude} in degrees)
    function _insideQuadrilateral(point, quad) {
        const tolerance = 1e-7
        let side = 0
        for (let i = 0; i < quad.length; i++) {
            const from = quad[i]
            const to = quad[(i + 1) % quad.length]
            const edgeX = to.x - from.x
            const edgeY = to.y - from.y
            const edgeLength = Math.hypot(edgeX, edgeY)
            if (edgeLength === 0) {
                continue
            }
            const distance = (edgeX * (point.y - from.y) - edgeY * (point.x - from.x)) / edgeLength
            if (distance > tolerance) {
                if (side < 0) {
                    return false
                }
                side = 1
            } else if (distance < -tolerance) {
                if (side > 0) {
                    return false
                }
                side = -1
            }
        }
        return true
    }

    /// Geographic bounds {north, south, west, east} of a rectangle, centered in the map's centerViewport, that lies
    /// within the ground the viewport shows: where a new shape goes so it is in view. The bounds of the viewport corners
    /// are not that once the map is rotated or tilted: they enclose the ground shown instead of lying within it. The
    /// rectangle is shrunk from them until its corners are inside the quadrilateral of the corners' ground. This only
    /// compares ground picks with each other, so it does not depend on the map projecting back to the screen.
    /// Longitudes are taken relative to the viewport center so a viewport over the dateline works. Null if the viewport
    /// shows no ground.
    function groundBounds(map) {
        const viewport = viewportRect(map)
        const centerPoint = Qt.point(viewport.x + viewport.width / 2, viewport.y + viewport.height / 2)
        const corners = [
            Qt.point(viewport.x, viewport.y),
            Qt.point(viewport.x + viewport.width, viewport.y),
            Qt.point(viewport.x + viewport.width, viewport.y + viewport.height),
            Qt.point(viewport.x, viewport.y + viewport.height)
        ]
        const cornerCoordinates = []
        for (const corner of corners) {
            const coordinate = _groundCoordinateInViewport(map, corner, centerPoint)
            if (!coordinate) {
                return null
            }
            cornerCoordinates.push(coordinate)
        }
        // Relative to the center, a viewport of any width up to the whole world keeps its corners within a half
        // turn of it on either side
        const centerCoordinate = map.toCoordinate(centerPoint, false /* clipToViewPort */)
        const referenceLongitude = centerCoordinate.isValid ? centerCoordinate.longitude : cornerCoordinates[0].longitude
        const quad = cornerCoordinates.map(
            coordinate => Qt.point(_normalizedLongitude(coordinate.longitude - referenceLongitude), coordinate.latitude))
        const minX = Math.min(...quad.map(point => point.x))
        const maxX = Math.max(...quad.map(point => point.x))
        const minY = Math.min(...quad.map(point => point.y))
        const maxY = Math.max(...quad.map(point => point.y))
        if (maxX - minX < 1e-9 || maxY - minY < 1e-9) {
            return null
        }

        const center = centerCoordinate.isValid ? Qt.point(0, centerCoordinate.latitude)
                                                : Qt.point((minX + maxX) / 2, (minY + maxY) / 2)
        const halfX = Math.max(maxX - center.x, center.x - minX)
        const halfY = Math.max(maxY - center.y, center.y - minY)

        for (let scale = 1; scale > 0.01; scale *= 0.9) {
            const west = center.x - halfX * scale
            const east = center.x + halfX * scale
            const south = center.y - halfY * scale
            const north = center.y + halfY * scale
            if ([Qt.point(west, south), Qt.point(east, south), Qt.point(east, north), Qt.point(west, north)]
                    .every(corner => _insideQuadrilateral(corner, quad))) {
                return {
                    north: Math.min(north, 90),
                    south: Math.max(south, -90),
                    west: _normalizedLongitude(referenceLongitude + west),
                    east: _normalizedLongitude(referenceLongitude + east)
                }
            }
        }
        return null
    }
}
