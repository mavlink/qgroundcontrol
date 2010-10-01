
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* addfog.c is a set of routines for adding OpenGL-style depth attenuated fog
   to a scene as a final rendering pass.  This may be useful if you are doing
   multipass algorithms where attempting to enable fog within the passes
   would screw up the rendering effect.

   The approach is to read back the depth buffer, then do an in-place draw
   pixels of the depth buffer values back into the frame buffer as alpha.
   OpenGL's standard pixel path is used to "blend in" fog.  The Red, Green,
   and Blue components are forced to the fog color; alpha is either scaled &
   biased (for GL_LINEAR style fog) or remapped with OpenGL's "map color"
   capability (for GL_EXP or GL_EXP style fog). The result is blended with
   the current frame buffer contents.  The result is almost identical to
   OpenGL's fog.

   With this fogging technique, the fog pass time is always proportional to
   the number of pixels in the window.  This is in contrast to fogging the
   entire scene standard OpenGL fog during rendering.  With standard OpenGL
   fog rendering, a scene with a depth complexity greater than 1 may end up
   fogging many more pixels than are visible.  If fog is not supported for
   free in hardware and you are rendering a scene with high enough depth
   complexity, this fogging technique could be faster than standard OpenGL
   fog.

   As mentioned earlier, the technique may also be appropriate in multi-pass
   rendering algorithms to avoid fog improperly interferring with the various
   passes.  Examples: reflections or shadows.

   A more sophisticated version of this technique could be used to simulate
   "eye distance" attenuated fog instead of OpenGL's "depth" attenuated fog.
   The technique could perform the depth to alpha read/write in tiles with a
   pixel map set up to attenuate based on eye distance instead of simply
   depth.

   This approach could be made more efficient with an extension to
   glCopyPixels to support a copy of one frame buffer type to another
   (specifically, depth to alpha).
   
   One side-effect of this approach is that it trashes your destination
   alpha buffer (if you even have one). */

/** Using the "addfog" routines:

   1)  Given your near and far ranges (generally from glOrtho, glFrustum, or
   gluPerspective), call:

   afEyeNearFar(near, far);

   Careful since "near" and "far" are reserved words on PC compilers.

   2)  Instead calling glFog to set fog parameters, use the corresponding
   "addfog" routines as shown below:

   Instead of:
   
       glFogf(GL_FOG_START, start); glFogf(GL_FOG_END, end);

   Use:

       afFogStartEnd(start, end);

   Instead of:

       glFogi(GL_FOG_MODE, mode);

   Use:

       afFogMode(mode);

   Instead of:

       glFogf(GL_FOG_DENSITY, density);

   Use:

       afFogDensity(density);

   Instead of:

       GLfloat fogcolor[4] = { red, green, blue, alpha };
       glFogfv(GL_FOG_COLOR, fog_color);

   Use:

       afFogColor(red, green, blue);

   3)  Draw you scene *without* OpenGL fog enabled.

   4)  Assuming you want to fog the entire window of size width by
       height, call:

       afDoFinalFogPass(0, 0, width, height);

       Note: x & y are OpenGL-style lower-left hand window coordinates.

   5)  Call glFinish or do a buffer swap.

   That's it.  View your fogged scene. */

#ifdef __sgi            /* SGI has a good alloca; many other machines don't. */
#define HAS_ALLOCA
#endif

#include <stdlib.h>
#ifdef HAS_ALLOCA
#include <alloca.h>
#endif
#ifdef _WIN32
#include <windows.h>
#pragma warning (disable:4244)          /* disable bogus conversion warnings */
#endif
#include <GL/gl.h>
#include <math.h>

static GLfloat eye_near = 0.0, eye_far = 1.0;
static GLfloat fog_start = 0.0, fog_end = 1.0;
static GLenum fog_mode = GL_EXP;
static GLfloat fog_density = 1.0;
static int valid = 0;
static GLfloat fog_red, fog_green, fog_blue;

void
afEyeNearFar(GLfloat fnear, GLfloat ffar)
{
  eye_near = fnear;
  eye_far = ffar;
  valid = 0;
}

void
afFogStartEnd(GLfloat start, GLfloat end)
{
  fog_start = start;
  fog_end = end;
  valid = 0;
}

void
afFogMode(GLenum mode)
{
  fog_mode = mode;
  valid = 0;
}

void
afFogDensity(GLfloat density)
{
  fog_density = density;
  valid = 0;
}

void
afFogColor(GLfloat red, GLfloat green, GLfloat blue)
{
  fog_red = red;
  fog_green = green;
  fog_blue = blue;
}

#define LEN 256

void
afDoFinalFogPass(GLint x, GLint y, GLsizei width, GLsizei height)
{
  static GLfloat alpha_scale, alpha_bias;
  static GLfloat fog_map[LEN];
  int i;

#ifdef HAS_ALLOCA
  void *buffer = alloca((unsigned int) sizeof(GLushort) * width * height);
#else
  static void *buffer = NULL;
  static int last_width, last_height;

  if (width * height != last_width * last_height) {
    buffer = realloc(buffer, sizeof(GLushort) * width * height);
    last_width = width;
    last_height = height;
  }
#endif

  if (!valid) {
    switch (fog_mode) {
    case GL_LINEAR:
      /* Figure out linear fog blending from "f = (e-z)/(e-s)". */
      alpha_scale = (eye_far - eye_near) / (fog_end - fog_start);
      alpha_bias = (eye_near - fog_start) / (fog_end - fog_start);
      break;
    case GL_EXP:
      /* Setup fog_map to be "f = exp(-d*z)". */
      for (i = 0; i < LEN; i += 1) {
        float fi, z, dz;

        fi = i * 1.0 / (LEN - 1);
        z = eye_near + fi * (eye_far - eye_near);
        dz = fog_density * z;
        fog_map[i] = 1.0 - exp(-dz);
      }
      break;
    case GL_EXP2:
      /* Setup fog_map to be "f = exp(-(d*z)^2)". */
      for (i = 0; i < LEN; i += 1) {
        float fi, z, dz;

        fi = i * 1.0 / (LEN - 1);
        z = eye_near + fi * (eye_far - eye_near);
        dz = fog_density * z;
        fog_map[i] = 1.0 - exp(-dz * dz);
      }
      break;
    default:;
      /* Mesa makes GLenum an actual enumerant.  Have a default
         case to avoid all the gcc warnings from all the other
	 GLenum values that we are not handling. */
    }
  }

  /* XXX Careful, afDoFinalFogPass makes no attempt to preserve your
     pixel store state and assumes the initial pixel store state! */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  /* Preserve the current raster position, viewport, matrix mode,
     blend function, enable state, and pixel path state. */
  /* XXX This is pretty expensive.  A real application should just
     "know" to reload all the OpenGL state mucked with by afDoFinalFogPass
     and then you could get rid of all this glPushAttrib and glPopMatrix
     garbage. */
  glPushAttrib(GL_PIXEL_MODE_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT |
    GL_TRANSFORM_BIT | GL_VIEWPORT_BIT | GL_CURRENT_BIT);

  /* Reposition the current raster position as location (x,y). */
  glMatrixMode(GL_MODELVIEW);
  glViewport(x-1, y-1, 2, 2);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glRasterPos2i(0, 0);

  /* Definitely don't want fog or depth enabled. */
  glDisable(GL_FOG);
  glDisable(GL_DEPTH_TEST);

  /* The alpha on the glDrawPixels after the pixel path transformation
     will be "1 - f" where f is the blending factor described in Section
     3.9 of the OpenGL 1.1 specification. */
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  switch (fog_mode) {
  case GL_LINEAR:
    /* Force red, green, and blue to the fog color. */
    glPixelTransferf(GL_RED_SCALE, 0);
    glPixelTransferf(GL_GREEN_SCALE, 0);
    glPixelTransferf(GL_BLUE_SCALE, 0);
    glPixelTransferf(GL_RED_BIAS, fog_red);
    glPixelTransferf(GL_GREEN_BIAS, fog_green);
    glPixelTransferf(GL_BLUE_BIAS, fog_blue);

    glPixelTransferf(GL_ALPHA_SCALE, alpha_scale);
    glPixelTransferf(GL_ALPHA_BIAS, alpha_bias);
    break;
  case GL_EXP:
  case GL_EXP2:
    /* Force red, green, and blue to the fog color. */
    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 1, &fog_red);
    glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 1, &fog_green);
    glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 1, &fog_blue);

    glPixelMapfv(GL_PIXEL_MAP_A_TO_A, LEN, fog_map);
    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
    break;
  }

  /* Read out the depth buffer... */
  glReadPixels(x, y, width, height,
    GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, buffer);

  /* ... and write it back as alpha. */
  glDrawPixels(width, height, GL_ALPHA,
    GL_UNSIGNED_SHORT, buffer);

  /* Restore state saved earlier. */
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
}
