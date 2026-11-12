import QtQuick
import QtQuick3D

import QGroundControl.GeoMap

/// Material of a FlightPathGeometry ribbon: flightpath.vert widens the centerline to ribbonWidth screen pixels and
/// flightpath.frag colors it. Every property but scene is a uniform of the two shaders.
CustomMaterial {
    property GeoScene scene
    property real ribbonWidth
    property color pathColor
    property color highlightColor: pathColor    ///< Color of the points that the geometry flags as highlighted
    property real pathOpacity: 1.0

    // Pixel-to-scene-unit factor at unit distance; the shader scales it by each vertex's own camera distance
    readonly property real screenFactor: scene && scene.camera ? scene.camera.unitsPerPixelAtUnitDistance : 0
    // Ramps in as the terrain flattens and defeats the depth test in 2D, where the flattened terrain keeps a
    // residual height above the flat path (see flightpath.vert)
    readonly property real depthPull: scene ? 0.5 * (1.0 - scene.terrainScale) : 0.0

    shadingMode: CustomMaterial.Unshaded
    cullMode: Material.NoCulling
    // Opaque unless the path is dimmed: blending would move it to the transparent pass
    sourceBlend: pathOpacity < 1 ? CustomMaterial.SrcAlpha : CustomMaterial.NoBlend
    destinationBlend: pathOpacity < 1 ? CustomMaterial.OneMinusSrcAlpha : CustomMaterial.NoBlend
    // Resolved here: Qt resolves a relative URL against the file that uses the material
    vertexShader: Qt.resolvedUrl("shaders/flightpath.vert")
    fragmentShader: Qt.resolvedUrl("shaders/flightpath.frag")
}
