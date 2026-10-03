VARYING float vHighlight;

void MAIN()
{
    FRAGCOLOR = vec4(mix(pathColor.rgb, highlightColor.rgb, vHighlight), 1.0);
}
