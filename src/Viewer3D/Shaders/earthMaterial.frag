///     @author Omid Esrafilian <esrafilian.omid@gmail.com>


VARYING vec2 uv;

// Ref:https://github.com/minus34/cesium1/blob/master/Cesium/Shaders/Builtin/Functions/saturation.glsl
vec3 czm_saturation(vec3 rgb, float adjustment)
{
    // Algorithm from Chapter 16 of OpenGL Shading Language
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}

void MAIN()
{
    vec4 textureColor = texture(someTextureMap, uv);
    BASE_COLOR = vec4(czm_saturation(textureColor.xyz, 1.5), 1.0);
}
