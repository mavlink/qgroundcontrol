#ifndef CHEETAHGL_H_
#define CHEETAHGL_H_

#if (defined __APPLE__) & (defined __MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

GLint generatePixhawkCheetah(float red, float green, float blue);

#endif
