VARYING float vHighlight;

void MAIN()
{
    vec3 linearColor = clamp(mix(pathColor.rgb, highlightColor.rgb, vHighlight), 0.0, 1.0);
    // Unshaded output skips the scene tonemap, so re-encode the linearized QML colors to sRGB here
    vec3 srgb = mix(linearColor * 12.92, 1.055 * pow(linearColor, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, linearColor));
    FRAGCOLOR = vec4(srgb, 1.0);
}
