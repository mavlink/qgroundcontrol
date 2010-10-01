/****************************************************************************
Copyright 1995 by Silicon Graphics Incorporated, Mountain View, California.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL SILICON GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

****************************************************************************/

/**
 * Derived from code written by Kurt Akeley, November 1992
 *
 *	Uses PolygonOffset to draw hidden-line images.  PolygonOffset
 *	    shifts the z values of polygons an amount that is
 *	    proportional to their slope in screen z.  This keeps
 *	    the lines, which are drawn without displacement, from
 *	    interacting with their respective polygons, and
 *	    thus eliminates line dropouts.
 *
 *	The left image shows an ordinary antialiased wireframe image.
 *	The center image shows an antialiased hidden-line image without
 *	    PolygonOffset.
 *	The right image shows an antialiased hidden-line image using
 *	    PolygonOffset to reduce artifacts.
 *
 *	Drag with a mouse button pressed to rotate the models.
 *	Press the escape key to exit.
 */

/*
 * Modified for OpenGL 1.1 glPolygonOffset() conventions
 */

/* Conversion to GLUT by Mark J. Kilgard */

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef EXIT_FAILURE
#define EXIT_FAILURE    1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS    0
#endif

#define MAXQUAD 6

typedef float Vertex[3];

typedef Vertex Quad[4];

/* data to define the six faces of a unit cube */
Quad quads[MAXQUAD] =
{
  { 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0 },
  { 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1 },
  { 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1 },
  { 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1 },
  { 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0 },
  { 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0 }
};
static int deltax = 90, deltay = 40;
static int prevx, prevy;

#define WIREFRAME	0
#define HIDDEN_LINE	1

static void error(const char *prog, const char *msg);
static void cubes(int mx, int my, int mode);
static void fill(Quad quad);
static void outline(Quad quad);
static void draw_hidden(Quad quad, int mode);
static void draw_scene(void);

static int dimension = 3;

#define MODE_11   0 /* OpenGL 1.1 */
#define MODE_EXT  1 /* Polygon offset extension */
#define MODE_NONE 2 /* No polygon offset support */
static int version;

int
supportsOneDotOne(void)
{
  const char *version;
  int major, minor;

  version = (char *) glGetString(GL_VERSION);
  if (sscanf(version, "%d.%d", &major, &minor) == 2)
    return major >= 1 && minor >= 1;
  return 0;            /* OpenGL version string malformed! */
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

  glutCreateWindow("offset");
  glutDisplayFunc(draw_scene);

#ifdef GL_VERSION_1_1
  if (supportsOneDotOne()) {
    glPolygonOffset(2.0, 1);
    version = MODE_11;
  } else
#endif
  {
#ifdef GL_EXT_polygon_offset
  /* check for the polygon offset extension */
  if (glutExtensionSupported("GL_EXT_polygon_offset")) {
    glPolygonOffsetEXT(0.75, 0.00);
    version = MODE_EXT;
  } else 
#endif
    {
      error(argv[0], "Warning: running with out the polygon offset extension.\n");
      version = MODE_NONE;
    }
  }

  /* set up viewing parameters */
  glMatrixMode(GL_PROJECTION);
  gluPerspective(20, 1, 0.1, 20);
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(0, 0, -15);

  /* set other relevant state information */
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_FLAT);
  glDisable(GL_DITHER);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}

static void
draw_scene(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();
  glTranslatef(-1.7, 0.0, 0.0);
  cubes(deltax, deltay, WIREFRAME);
  glPopMatrix();

  glPushMatrix();
  cubes(deltax, deltay, HIDDEN_LINE);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(1.7, 0.0, 0.0);

  switch(version) {
#ifdef GL_VERSION_1_1
  case MODE_11:
    glEnable(GL_POLYGON_OFFSET_FILL);
    break;
#endif
#ifdef GL_EXT_polygon_offset
  case MODE_EXT:
    glEnable(GL_POLYGON_OFFSET_EXT);
    break;
#endif
  }

  cubes(deltax, deltay, HIDDEN_LINE);

  switch(version) {
#ifdef GL_VERSION_1_1
  case MODE_11:
    glDisable(GL_POLYGON_OFFSET_FILL);
    break;
#endif
#ifdef GL_EXT_polygon_offset
  case MODE_EXT:
    glDisable(GL_POLYGON_OFFSET_EXT);
    break;
#endif
  }

  glPopMatrix();

  glutSwapBuffers();
}

static void
cubes(int mx, int my, int mode)
{
  int x, y, z, i;

  /* track the mouse */
  glRotatef(mx / 2.0, 0, 1, 0);
  glRotatef(my / 2.0, 1, 0, 0);

  /* draw the lines as hidden polygons */
  glTranslatef(-0.5, -0.5, -0.5);
  glScalef(1.0 / dimension, 1.0 / dimension, 1.0 / dimension);
  for (z = 0; z < dimension; z++) {
    for (y = 0; y < dimension; y++) {
      for (x = 0; x < dimension; x++) {
        glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(0.8, 0.8, 0.8);
        for (i = 0; i < MAXQUAD; i++)
          draw_hidden(quads[i], mode);
        glPopMatrix();
      }
    }
  }
}

static void
fill(Quad quad)
{
  /* draw a filled polygon */
  glBegin(GL_QUADS);
  glVertex3fv(quad[0]);
  glVertex3fv(quad[1]);
  glVertex3fv(quad[2]);
  glVertex3fv(quad[3]);
  glEnd();
}

static void
outline(Quad quad)
{
  /* draw an outlined polygon */
  glBegin(GL_LINE_LOOP);
  glVertex3fv(quad[0]);
  glVertex3fv(quad[1]);
  glVertex3fv(quad[2]);
  glVertex3fv(quad[3]);
  glEnd();
}

static void
draw_hidden(Quad quad, int mode)
{
  /* draw the outline using white, optionally fill the interior 
     with black */
  glColor3f(1, 1, 1);
  outline(quad);

  if (mode == HIDDEN_LINE) {
    glColor3f(0, 0, 0);
    fill(quad);
  }
}

/* ARGSUSED1 */
void
keyboard(unsigned char key, int x, int y)
{
  if (key == 27) exit(EXIT_SUCCESS);
}

/* ARGSUSED */
void
button(int which, int state, int x, int y)
{
  if(state == GLUT_DOWN) {
      prevx = x;
      prevy = y;
  }
}

void
motion(int x, int y)
{
  deltax += (x - prevx);
  prevx = x;
  deltay += (y - prevy);
  prevy = y;
  glutPostRedisplay();
}

static void
error(const char *prog, const char *msg)
{
  fprintf(stderr, "%s: %s\n", prog, msg);
}

