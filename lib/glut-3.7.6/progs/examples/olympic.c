
/* Copyright (c) Mark J. Kilgard, 1994. */

/**
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
#define drand48() (((float) rand())/((float) RAND_MAX))
#define srand48(x) (srand((x)))
#else
extern double drand48(void);
extern void srand48(long seedval);
#endif

#define XSIZE   100
#define YSIZE   75

#define RINGS 5
#define BLUERING 0
#define BLACKRING 1
#define REDRING 2
#define YELLOWRING 3
#define GREENRING 4

#define BACKGROUND 8

enum {
  BLACK = 0,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
  CYAN,
  WHITE
};

typedef short Point[2];

GLenum rgb, doubleBuffer, directRender;

unsigned char rgb_colors[RINGS][3];
int mapped_colors[RINGS];
float dests[RINGS][3];
float offsets[RINGS][3];
float angs[RINGS];
float rotAxis[RINGS][3];
int iters[RINGS];
GLuint theTorus;

void
FillTorus(float rc, int numc, float rt, int numt)
{
  int i, j, k;
  double s, t;
  double x, y, z;
  double pi, twopi;

  pi = M_PI;
  twopi = 2 * pi;

  for (i = 0; i < numc; i++) {
    glBegin(GL_QUAD_STRIP);
    for (j = 0; j <= numt; j++) {
      for (k = 1; k >= 0; k--) {
        s = (i + k) % numc + 0.5;
        t = j % numt;

        x = cos(t * twopi / numt) * cos(s * twopi / numc);
        y = sin(t * twopi / numt) * cos(s * twopi / numc);
        z = sin(s * twopi / numc);
        glNormal3f(x, y, z);

        x = (rt + rc * cos(s * twopi / numc)) * cos(t * twopi / numt);
        y = (rt + rc * cos(s * twopi / numc)) * sin(t * twopi / numt);
        z = rc * sin(s * twopi / numc);
        glVertex3f(x, y, z);
      }
    }
    glEnd();
  }
}

float
Clamp(int iters_left, float t)
{

  if (iters_left < 3) {
    return 0.0;
  }
  return (iters_left - 2) * t / iters_left;
}

void
Idle(void)
{
  int i, j;
  int more = GL_FALSE;

  for (i = 0; i < RINGS; i++) {
    if (iters[i]) {
      for (j = 0; j < 3; j++) {
        offsets[i][j] = Clamp(iters[i], offsets[i][j]);
      }
      angs[i] = Clamp(iters[i], angs[i]);
      iters[i]--;
      more = GL_TRUE;
    }
  }
  if (more) {
    glutPostRedisplay();
  } else {
    glutIdleFunc(NULL);
  }
}

void
DrawScene(void)
{
  int i;

  glPushMatrix();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0);

  for (i = 0; i < RINGS; i++) {
    if (rgb) {
      glColor3ubv(rgb_colors[i]);
    } else {
      glIndexi(mapped_colors[i]);
    }
    glPushMatrix();
    glTranslatef(dests[i][0] + offsets[i][0], dests[i][1] + offsets[i][1],
      dests[i][2] + offsets[i][2]);
    glRotatef(angs[i], rotAxis[i][0], rotAxis[i][1], rotAxis[i][2]);
    glCallList(theTorus);
    glPopMatrix();
  }

  glPopMatrix();
  if (doubleBuffer) {
    glutSwapBuffers();
  } else {
    glFlush();
  }
}

float
MyRand(void)
{
  return 10.0 * (drand48() - 0.5);
}

void
ReInit(void)
{
  int i;
  float deviation;

  deviation = MyRand() / 2;
  deviation = deviation * deviation;
  for (i = 0; i < RINGS; i++) {
    offsets[i][0] = MyRand();
    offsets[i][1] = MyRand();
    offsets[i][2] = MyRand();
    angs[i] = 260.0 * MyRand();
    rotAxis[i][0] = MyRand();
    rotAxis[i][1] = MyRand();
    rotAxis[i][2] = MyRand();
    iters[i] = (deviation * MyRand() + 60.0);
  }
}

void
Init(void)
{
  int i;
  float top_y = 1.0;
  float bottom_y = 0.0;
  float top_z = 0.15;
  float bottom_z = 0.69;
  float spacing = 2.5;
  static float lmodel_ambient[] =
  {0.0, 0.0, 0.0, 0.0};
  static float lmodel_twoside[] =
  {GL_FALSE};
  static float lmodel_local[] =
  {GL_FALSE};
  static float light0_ambient[] =
  {0.1, 0.1, 0.1, 1.0};
  static float light0_diffuse[] =
  {1.0, 1.0, 1.0, 0.0};
  static float light0_position[] =
  {0.8660254, 0.5, 1, 0};
  static float light0_specular[] =
  {1.0, 1.0, 1.0, 0.0};
  static float bevel_mat_ambient[] =
  {0.0, 0.0, 0.0, 1.0};
  static float bevel_mat_shininess[] =
  {40.0};
  static float bevel_mat_specular[] =
  {1.0, 1.0, 1.0, 0.0};
  static float bevel_mat_diffuse[] =
  {1.0, 0.0, 0.0, 0.0};

  srand48(0x102342);
  ReInit();
  for (i = 0; i < RINGS; i++) {
    rgb_colors[i][0] = rgb_colors[i][1] = rgb_colors[i][2] = 0;
  }
  rgb_colors[BLUERING][2] = 255;
  rgb_colors[REDRING][0] = 255;
  rgb_colors[GREENRING][1] = 255;
  rgb_colors[YELLOWRING][0] = 255;
  rgb_colors[YELLOWRING][1] = 255;
  mapped_colors[BLUERING] = BLUE;
  mapped_colors[REDRING] = RED;
  mapped_colors[GREENRING] = GREEN;
  mapped_colors[YELLOWRING] = YELLOW;
  mapped_colors[BLACKRING] = BLACK;

  dests[BLUERING][0] = -spacing;
  dests[BLUERING][1] = top_y;
  dests[BLUERING][2] = top_z;

  dests[BLACKRING][0] = 0.0;
  dests[BLACKRING][1] = top_y;
  dests[BLACKRING][2] = top_z;

  dests[REDRING][0] = spacing;
  dests[REDRING][1] = top_y;
  dests[REDRING][2] = top_z;

  dests[YELLOWRING][0] = -spacing / 2.0;
  dests[YELLOWRING][1] = bottom_y;
  dests[YELLOWRING][2] = bottom_z;

  dests[GREENRING][0] = spacing / 2.0;
  dests[GREENRING][1] = bottom_y;
  dests[GREENRING][2] = bottom_z;

  theTorus = glGenLists(1);
  glNewList(theTorus, GL_COMPILE);
  FillTorus(0.1, 8, 1.0, 25);
  glEndList();

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.0);

  if (rgb) {
    glClearColor(0.5, 0.5, 0.5, 0.0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glEnable(GL_LIGHT0);

    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_local);
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glEnable(GL_LIGHTING);

    glMaterialfv(GL_FRONT, GL_AMBIENT, bevel_mat_ambient);
    glMaterialfv(GL_FRONT, GL_SHININESS, bevel_mat_shininess);
    glMaterialfv(GL_FRONT, GL_SPECULAR, bevel_mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, bevel_mat_diffuse);

    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
  } else {
    glClearIndex(BACKGROUND);
    glShadeModel(GL_FLAT);
  }

  glMatrixMode(GL_PROJECTION);
  gluPerspective(45, 1.33, 0.1, 100.0);
  glMatrixMode(GL_MODELVIEW);
}

void
Reshape(int width, int height)
{
  glViewport(0, 0, width, height);
}

/* ARGSUSED1 */
void
Key(unsigned char key, int x, int y)
{

  switch (key) {
  case 27:
    exit(0);
    break;
  case ' ':
    ReInit();
    glutIdleFunc(Idle);
    break;
  }
}

GLenum
Args(int argc, char **argv)
{
  GLint i;

  rgb = GL_TRUE;
  doubleBuffer = GL_TRUE;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-ci") == 0) {
      rgb = GL_FALSE;
    } else if (strcmp(argv[i], "-rgb") == 0) {
      rgb = GL_TRUE;
    } else if (strcmp(argv[i], "-sb") == 0) {
      doubleBuffer = GL_FALSE;
    } else if (strcmp(argv[i], "-db") == 0) {
      doubleBuffer = GL_TRUE;
    } else {
      printf("%s (Bad option).\n", argv[i]);
      return GL_FALSE;
    }
  }
  return GL_TRUE;
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    glutIdleFunc(Idle);
  } else {
    glutIdleFunc(NULL);
  }
}

int
main(int argc, char **argv)
{
  GLenum type;

  glutInitWindowSize(400, 300);
  glutInit(&argc, argv);
  if (Args(argc, argv) == GL_FALSE) {
    exit(1);
  }
  type = (rgb) ? GLUT_RGB : GLUT_INDEX;
  type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
  glutInitDisplayMode(type);

  glutCreateWindow("Olympic");

  Init();

  glutReshapeFunc(Reshape);
  glutKeyboardFunc(Key);
  glutDisplayFunc(DrawScene);

  glutVisibilityFunc(visible);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
