#version 440

layout(location = 0) in vec4 aVertex;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 vTexCoord;

layout(std140, binding = 0) uniform buf {
  mat4 qt_Matrix;
  ivec4 swizzle;
  mat4 color_matrix;
  float qt_Opacity;
} ubuf;

out gl_PerVertex {
  vec4 gl_Position;
};

void main()
{
  gl_Position = ubuf.qt_Matrix * aVertex;
  vTexCoord = aTexCoord;
}
