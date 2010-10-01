
/* Copyright (c) Mark J. Kilgard, 1994.  */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This program uses 3D texture coordinates to introduce
   "sifting" effects to warp a static mesh of textured
   geometry.  The third texture coordinate encodes a shifting
   quantity through the mesh.  By updating the texture matrix,
   the texture coordinates can be shifted based on this
   third texture coordinate.  You'll notice the face seems
   to have local vortexes scattered over the image that
   warp the image.  While the texture coordinates look dynamic,
   they are indeed quite static (frozen in a display list) and
   it is just the texture matrix that is changing to shift
   the final 2D texture coordinates. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#include <GL/glut.h>

extern unsigned char mjk_image[];
extern int mjk_depth;
extern int mjk_height;
extern int mjk_width;

float tick1 = 0;
float tick2 = 0;
float angle;
float size = 0.4;
int set_timeout = 0;
int visible = 0;
int sifting = 1;
int scaling = 0;
int interval = 100;

void
animate(int value)
{
  if (visible) {
    if (sifting || scaling) {
      if (value) {
        if (sifting) {
          tick1 += 4 * (interval / 100.0);
          angle = ((int) tick1) % 360;
        }
        if (scaling) {
          tick2 += 2 * (interval / 100.0);
          size = .7 - .5 * sin(tick2 / 20.0);
        }
      }
      glutPostRedisplay();
      set_timeout = 1;
    }
  }
}

/* Setup display list with "frozen" 3D texture coordinates. */
void
generateTexturedSurface(void)
{
  static GLfloat data[8] =
  {0, 1, 0, -1};
  int i, j;

#define COLS 12
#define ROWS 12
#define TILE_TEX_W (1.0/COLS)
#define TILE_TEX_H (1.0/ROWS)

  glNewList(1, GL_COMPILE);
  glTranslatef(-COLS / 2.0 + .5, -ROWS / 2.0 + .5, 0);
  for (j = 0; j < ROWS; j++) {
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i < COLS; i++) {
      glTexCoord3f(i * TILE_TEX_W, j * TILE_TEX_H, data[(i + j) % 4]);
      glVertex2f(i - .5, j - .5);
      glTexCoord3f(i * TILE_TEX_W, (j + 1) * TILE_TEX_H, data[(i + j + 1) % 4]);
      glVertex2f(i - .5, j + .5);
    }
    glTexCoord3f((i + 1) * TILE_TEX_W, j * TILE_TEX_H, data[(i + j) % 4]);
    glVertex2f(i + .5, j - .5);
    glTexCoord3f((i + 1) * TILE_TEX_W, (j + 1) * TILE_TEX_H, data[(i + j + 1) % 4]);
    glVertex2f(i + .5, j + .5);
    glEnd();
  }
  glEndList();
}

/* Construct an identity matrix except that the third coordinate
   can be used to "sift" the X and Y coordinates. */
void
makeSift(GLfloat m[16], float xsift, float ysift)
{
  m[0 + 4 * 0] = 1;
  m[0 + 4 * 1] = 0;
  m[0 + 4 * 2] = xsift;
  m[0 + 4 * 3] = 0;

  m[1 + 4 * 0] = 0;
  m[1 + 4 * 1] = 1;
  m[1 + 4 * 2] = ysift;
  m[1 + 4 * 3] = 0;

  m[2 + 4 * 0] = 0;
  m[2 + 4 * 1] = 0;
  m[2 + 4 * 2] = 1;
  m[2 + 4 * 3] = 0;

  m[3 + 4 * 0] = 0;
  m[3 + 4 * 1] = 0;
  m[3 + 4 * 2] = 0;
  m[3 + 4 * 3] = 1;
}

void
redraw(void)
{
  int begin, end, elapsed;
  GLfloat matrix[16];

  if (set_timeout) {
    begin = glutGet(GLUT_ELAPSED_TIME);
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();

  glScalef(size, size, size);

  glMatrixMode(GL_TEXTURE);
  makeSift(matrix, 0.02 * cos(tick1 / 40.0), 0.02 * sin(tick1 / 15.0));
  glLoadMatrixf(matrix);
  glMatrixMode(GL_MODELVIEW);

  glCallList(1);

  glPopMatrix();
  glutSwapBuffers();
  if (set_timeout) {
    set_timeout = 0;
    end = glutGet(GLUT_ELAPSED_TIME);
    elapsed = end - begin;
    if (elapsed > interval) {
      glutTimerFunc(0, animate, 1);
    } else {
      glutTimerFunc(interval - elapsed, animate, 1);
    }
  }
}

int width;
int height;
int depth;
unsigned char *bits;

void
visibility(int state)
{
  if (state == GLUT_VISIBLE) {
    visible = 1;
    animate(0);
  } else {
    visible = 0;
  }
}

void
minify_select(int value)
{
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
  gluBuild2DMipmaps(GL_TEXTURE_2D, depth, width, height,
    GL_RGB, GL_UNSIGNED_BYTE, bits);
  glutPostRedisplay();
}

void
rate_select(int value)
{
  interval = value;
}

void
menu_select(int value)
{
  switch (value) {
  case 1:
    sifting = !sifting;
    if (sifting)
      animate(0);
    break;
  case 2:
    scaling = !scaling;
    if (scaling)
      animate(0);
    break;
  case 666:
    exit(0);
  }
}

int
main(int argc, char **argv)
{
  int minify_menu, rate_menu;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("mjksift");
  glutDisplayFunc(redraw);
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 70.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,30) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */
  depth = mjk_depth;
  width = mjk_width;
  height = mjk_height;
  bits = mjk_image;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  gluBuild2DMipmaps(GL_TEXTURE_2D, depth, width, height,
    GL_RGB, GL_UNSIGNED_BYTE, bits);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glEnable(GL_TEXTURE_2D);
  glutVisibilityFunc(visibility);
  minify_menu = glutCreateMenu(minify_select);
  glutAddMenuEntry("Nearest", GL_NEAREST);
  glutAddMenuEntry("Linear", GL_LINEAR);
  glutAddMenuEntry("Nearest mipmap nearest", GL_NEAREST_MIPMAP_NEAREST);
  glutAddMenuEntry("Linear mipmap nearest", GL_LINEAR_MIPMAP_NEAREST);
  glutAddMenuEntry("Nearest mipmap linear", GL_NEAREST_MIPMAP_LINEAR);
  glutAddMenuEntry("Linear mipmap linear", GL_LINEAR_MIPMAP_LINEAR);
  rate_menu = glutCreateMenu(rate_select);
  glutAddMenuEntry(" 2/sec", 500);
  glutAddMenuEntry(" 6/sec", 166);
  glutAddMenuEntry("10/sec", 100);
  glutAddMenuEntry("20/sec", 50);
  glutAddMenuEntry("30/sec", 33);
  glutAddMenuEntry("60/sec", 16);
  glutCreateMenu(menu_select);
  glutAddMenuEntry("Toggle sifting", 1);
  glutAddMenuEntry("Toggle scaling", 2);
  glutAddSubMenu("Minimum frame rate", rate_menu);
  glutAddSubMenu("Minify modes", minify_menu);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  menu_select(3);
  generateTexturedSurface();
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
