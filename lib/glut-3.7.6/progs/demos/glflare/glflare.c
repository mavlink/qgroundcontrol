
/* This source code is based on Direct3D-based code written by Stephen
   Coy of Microsoft.  All credit for the original implementation of the
   idea should go to Stephen.  While I referenced Stephen's code during
   the writing of my code, my code was completely written by me from
   scratch.  The algorithms are basically the same to ease critical
   comparison of the OpenGL and Direct3D versions.  */

/* The image files used by this program are derived from images
   developed by Microsoft. */

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if !defined(GL_VERSION_1_1)
/* Assume the texture object extension is supported. */
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#endif

#include "loadlum.h"

GLfloat red[3] = {1.0, 0.0, 0.0};
GLfloat green[3] = {0.0, 1.0, 0.0};
GLfloat blue[3] = {0.0, 0.0, 1.0};
GLfloat from[3] = {0.0, 0.0, 20.0};
GLfloat at[3] = {0.0, 0.0, 0.0};
GLfloat near_clip = 1.0;
int useMipmaps = 0, verbose = 0;
GLuint logoTex, flareTex[6], shineTex[10];

typedef struct t_flare {
  int type;             /* flare texture index, 0..5 */
  float scale;
  float loc;            /* postion on axis */
  GLfloat color[3];
} Flare;

Flare
set_flare(int type, float location, float scale, GLfloat color[3], float colorScale)
{
  Flare ret;

  ret.type = type;
  ret.loc = location;
  ret.scale = scale;
  ret.color[0] = color[0] * colorScale;
  ret.color[1] = color[1] * colorScale;
  ret.color[2] = color[2] * colorScale;
  return ret;
}

Flare flare[20];
int num_flares = 0;
static float tic = 0.0;
float position[3];
int shine_tic = 0;

void
init_flares(void)
{
  /* Shines */
  flare[0] = set_flare(-1, 1.0, 0.3, blue, 1.0);
  flare[1] = set_flare(-1, 1.0, 0.2, green, 1.0);
  flare[2] = set_flare(-1, 1.0, 0.25, red, 1.0);

  /* Flares, ordered to eliminate redundant texture binds */
  flare[3] = set_flare(1, 0.5, 0.2f, red, 0.3);
  flare[4] = set_flare(2, 1.3, 0.04f, red, 0.6);
  flare[5] = set_flare(3, 1.0, 0.1f, red, 0.4);
  flare[6] = set_flare(3, 0.2, 0.05f, red, 0.3);
  flare[7] = set_flare(0, 0.0, 0.04f, red, 0.3);
  flare[8] = set_flare(5, -0.25, 0.07f, red, 0.5);
  flare[9] = set_flare(5, -0.4, 0.02f, red, 0.6);
  flare[10] = set_flare(5, -0.6, 0.04f, red, 0.4);
  flare[11] = set_flare(5, -1.0, 0.03f, red, 0.2);
  num_flares = 12;
}

#include "vec3d.c"  /* Simple 3D vector math routines. */

void
DoFlares(GLfloat from[3], GLfloat at[3], GLfloat light[3], GLfloat near_clip)
{
  GLfloat view_dir[3], tmp[3], light_dir[3], position[3], dx[3], dy[3],
    center[3], axis[3], sx[3], sy[3], dot, global_scale = 1.5;
  GLuint bound_to = 0;
  int i;

  /* view_dir = normalize(at-from) */
  vdiff(view_dir, at, from);
  vnorm(view_dir);

  /* center = from + near_clip * view_dir */
  vscale(tmp, view_dir, near_clip);
  vadd(center, from, tmp);

  /* light_dir = normalize(light-from) */
  vdiff(light_dir, light, from);
  vnorm(light_dir);

  /* light = from + dot(light,view_dir)*near_clip*light_dir */
  dot = vdot(light_dir, view_dir);
  vscale(tmp, light_dir, near_clip / dot);
  vadd(light, from, light_dir);

  /* axis = light - center */
  vdiff(axis, light, center);
  vcopy(dx, axis);

  /* dx = normalize(axis) */
  vnorm(dx);

  /* dy = cross(dx,view_dir) */
  vcross(dy, dx, view_dir);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  for (i = 0; i < num_flares; i++) {
    vscale(sx, dx, flare[i].scale * global_scale);
    vscale(sy, dy, flare[i].scale * global_scale);

    glColor3fv(flare[i].color);
    /* Note logic below to eliminate duplicate texture binds. */
    if (flare[i].type < 0) {
      if (bound_to)
	glEnd();
      glBindTexture(GL_TEXTURE_2D, shineTex[shine_tic]);
      bound_to = shineTex[shine_tic];
      shine_tic = (shine_tic + 1) % 10;
      glBegin(GL_QUADS);
    } else {
      if (bound_to != flareTex[flare[i].type]) {
	glEnd();
        glBindTexture(GL_TEXTURE_2D, flareTex[flare[i].type]);
        bound_to = flareTex[flare[i].type];
	glBegin(GL_QUADS);
      }
    }

    /* position = center + flare[i].loc * axis */
    vscale(tmp, axis, flare[i].loc);
    vadd(position, center, tmp);

    glTexCoord2f(0.0, 0.0);
    vadd(tmp, position, sx);
    vadd(tmp, tmp, sy);
    glVertex3fv(tmp);

    glTexCoord2f(1.0, 0.0);
    vdiff(tmp, position, sx);
    vadd(tmp, tmp, sy);
    glVertex3fv(tmp);

    glTexCoord2f(1.0, 1.0);
    vdiff(tmp, position, sx);
    vdiff(tmp, tmp, sy);
    glVertex3fv(tmp);

    glTexCoord2f(0.0, 1.0);
    vadd(tmp, position, sx);
    vdiff(tmp, tmp, sy);
    glVertex3fv(tmp);
  }
  glEnd();
}

void
DoBackground(void)
{
  glEnable(GL_DITHER);
  glDisable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, logoTex);

  glBegin(GL_QUADS);
  glColor3f(0.0, 0.0, 1.0);
  glTexCoord2f(0.075, 0.1);
  glVertex3f(-11.0, -7.0, 0.0);

  glColor3f(0.8, 0.8, 1.0);
  glTexCoord2f(1.0, 0.1);
  glVertex3f(11.0, -7.0, 0.0);

  glColor3f(0.0, 0.0, 1.0);
  glTexCoord2f(1.0, 0.9);
  glVertex3f(11.0, 7.0, 0.0);

  glColor3f(0.0, 0.5, 1.0);
  glTexCoord2f(0.075, 0.9);
  glVertex3f(-11.0, 7.0, 0.0);
  glEnd();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  position[0] = sin(tic * 0.73) * 6;
  position[1] = 4.0 + 8.0 * sin(tic * 0.678);
  position[2] = sin(tic * 0.895) * 6;
  DoBackground();
  DoFlares(from, at, position, near_clip);

  glutSwapBuffers();
}

void
idle(void)
{
  tic += 0.08f;
  glutPostRedisplay();
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(idle);
  else
    glutIdleFunc(NULL);
}

void
setup_texture(char *filename, GLuint texobj,
  GLenum minFilter, GLenum maxFilter)
{
  unsigned char *buf;
  int width, height, components;

  glBindTexture(GL_TEXTURE_2D, texobj);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, maxFilter);
  if (verbose)
    printf("Loading %s:", filename);
  buf = load_luminance(filename, &width, &height, &components);
  if (verbose)
    printf(" %dx%dx%d\n", width, height, components);
  if (useMipmaps)
    gluBuild2DMipmaps(GL_TEXTURE_2D, 1, width, height,
      GL_LUMINANCE, GL_UNSIGNED_BYTE, buf);
  else
    glTexImage2D(GL_TEXTURE_2D, 0, 1, width, height, 0,
      GL_LUMINANCE, GL_UNSIGNED_BYTE, buf);
  free(buf);
}

void
load_textures(void)
{
  char filename[256];
  GLenum minFilter, maxFilter;
  int id = 1, i;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (useMipmaps) {
    minFilter = GL_LINEAR_MIPMAP_LINEAR;
    maxFilter = GL_LINEAR;
  } else {
    minFilter = GL_LINEAR;
    maxFilter = GL_LINEAR;
  }
  logoTex = id;
  setup_texture("OpenGL.bw", logoTex, minFilter, maxFilter);
  id++;

  if (!useMipmaps) {
    minFilter = GL_NEAREST;
    maxFilter = GL_NEAREST;
  }
  for (i = 0; i < 10; i++) {
    shineTex[i] = id;
    sprintf(filename, "Shine%d.bw", i);
    setup_texture(filename, shineTex[i], minFilter, maxFilter);
    id++;
  }
  for (i = 0; i < 6; i++) {
    flareTex[i] = id;
    sprintf(filename, "Flare%d.bw", i + 1);
    setup_texture(filename, flareTex[i], minFilter, maxFilter);
    id++;
  }
}

int
main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);
  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i], "-mipmap"))
      useMipmaps = 1;
    else if (!strcmp(argv[i], "-v"))
      verbose = 1;
  }
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("glflare");
  glutDisplayFunc(display);
  glutVisibilityFunc(visible);

  init_flares();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(
    60.0,               /* field of view in degree */
    1.0,                /* aspect ratio */
    0.5, 30.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(
    from[0], from[1], from[2],
    at[0], at[1], at[2],
    0.0, 1.0, 0.);      /* up is in positive Y direction */

  load_textures();

  glEnable(GL_TEXTURE_2D);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
