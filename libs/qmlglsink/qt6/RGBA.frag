#version 440

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
  mat4 qt_Matrix;
  ivec4 swizzle;
  mat4 color_matrix;
  float qt_Opacity;
} ubuf;

layout(binding = 1) uniform sampler2D tex;

vec4 swizzle(in vec4 texel, in ivec4 swizzle) {
  return vec4(texel[swizzle[0]], texel[swizzle[1]], texel[swizzle[2]], texel[swizzle[3]]);
}

void main()
{
  vec4 texel = swizzle(texture(tex, vTexCoord), ubuf.swizzle);
  fragColor = texel * ubuf.qt_Opacity;
}
