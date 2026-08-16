// Flight-path ribbon expansion: each centerline vertex carries the path's
// unit 3D tangent (NORMAL) and a side flag (UV0.x). Expand perpendicular to
// both the tangent and the view direction (billboarding) so the ribbon faces
// the camera for any segment orientation — including vertical climbs, which
// a fixed horizontal expansion would render edge-on and invisible. Half the
// line width is converted from pixels to scene units at this vertex's
// distance from the camera so the width stays constant on screen.
void MAIN()
{
    vec3 worldCenter = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
    vec3 viewVec = worldCenter - CAMERA_POSITION;
    float dist = length(viewVec);
    float unitsPerPixel = screenFactor * dist;
    // Model-space tangent transforms with the model matrix (this also
    // flattens vertical tangents along with the terrain in 2D mode)
    vec3 worldTangent = (MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz;
    vec3 perp = cross(worldTangent, viewVec / max(dist, 1e-6));
    float perpLen = length(perp);
    // Degenerate (segment seen end-on or flattened to a point): any
    // perpendicular works; use the camera's right vector
    vec3 sideDir = (perpLen > 1e-4) ? (perp / perpLen)
                                    : vec3(VIEW_MATRIX[0][0], VIEW_MATRIX[1][0], VIEW_MATRIX[2][0]);
    vec3 worldPos = worldCenter + (sideDir * (UV0.x * 0.5 * lineWidth * unitsPerPixel));
    // Scaling view-space position leaves the screen position unchanged but
    // shrinks depth: in 2D mode (depthPull > 0) this defeats the depth test
    // against the flattened terrain, which keeps a residual height above the
    // fully flattened path.
    vec4 viewPos = VIEW_MATRIX * vec4(worldPos, 1.0);
    viewPos.xyz *= 1.0 - depthPull;
    POSITION = PROJECTION_MATRIX * viewPos;
}
