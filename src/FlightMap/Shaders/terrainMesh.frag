VARYING vec2 uv;
VARYING float heightNorm;

void MAIN()
{
    // Altitude-based color ramp:
    // Low (0.0)  -> dark green
    // Mid (0.5)  -> yellow-brown
    // High (1.0) -> white (snow)

    // Use vertex position Y as the height indicator through the custom property
    float h = clamp(heightFraction, 0.0, 1.0);

    vec3 color;
    if (h < 0.25) {
        // Green to yellow-green
        color = mix(vec3(0.18, 0.42, 0.14), vec3(0.45, 0.55, 0.20), h / 0.25);
    } else if (h < 0.5) {
        // Yellow-green to tan/brown
        color = mix(vec3(0.45, 0.55, 0.20), vec3(0.60, 0.48, 0.28), (h - 0.25) / 0.25);
    } else if (h < 0.75) {
        // Tan to gray rock
        color = mix(vec3(0.60, 0.48, 0.28), vec3(0.55, 0.52, 0.50), (h - 0.5) / 0.25);
    } else {
        // Gray rock to white snow
        color = mix(vec3(0.55, 0.52, 0.50), vec3(0.95, 0.95, 0.97), (h - 0.75) / 0.25);
    }

    BASE_COLOR = vec4(color, terrainOpacity);
}
