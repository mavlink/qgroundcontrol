VARYING vec2 uv;
VARYING float heightNorm;

void MAIN()
{
    uv = UV0;
    // Extract Y position as relative height for coloring
    // The mesh is built with Y = terrain height in local coords
    heightNorm = UV0.y;
}
