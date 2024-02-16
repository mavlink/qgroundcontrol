#version 440

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
  mat4 qt_Matrix;
  ivec4 swizzle;
  mat4 color_matrix;
  float qt_Opacity;
} ubuf;

layout(binding = 1) uniform sampler2D Ytex;
layout(binding = 2) uniform sampler2D Utex;
layout(binding = 3) uniform sampler2D Vtex;

vec4 swizzle(in vec4 texel, in ivec4 swizzle) {
  return vec4(texel[swizzle[0]], texel[swizzle[1]], texel[swizzle[2]], texel[swizzle[3]]);
}

vec4 yuva_to_rgba(in vec4 yuva, in mat4 color_matrix) {
  return yuva * color_matrix;
}

void main()
{
  vec4 yuva;
  yuva.x = texture(Ytex, vTexCoord).r;
  yuva.y = texture(Utex, vTexCoord).r;
  yuva.z = texture(Vtex, vTexCoord).r;
  yuva.a = 1.0;
  yuva = swizzle(yuva, ubuf.swizzle);
  vec4 rgba = yuva_to_rgba (yuva, ubuf.color_matrix);
  fragColor = rgba * ubuf.qt_Opacity;
}

