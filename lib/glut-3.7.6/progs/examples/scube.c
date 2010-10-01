
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

/*
 * 1992 David G Yu -- Silicon Graphics Computer Systems
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

static int useRGB = 1;
static int useLighting = 1;
static int useFog = 0;
static int useDB = 1;
static int useLogo = 0;
static int useQuads = 1;

static int tick = -1;
static int moving = 1;

#define GREY	0
#define RED	1
#define GREEN	2
#define BLUE	3
#define CYAN	4
#define MAGENTA	5
#define YELLOW	6
#define BLACK	7

static float materialColor[8][4] =
{
  {0.8, 0.8, 0.8, 1.0},
  {0.8, 0.0, 0.0, 1.0},
  {0.0, 0.8, 0.0, 1.0},
  {0.0, 0.0, 0.8, 1.0},
  {0.0, 0.8, 0.8, 1.0},
  {0.8, 0.0, 0.8, 1.0},
  {0.8, 0.8, 0.0, 1.0},
  {0.0, 0.0, 0.0, 0.6},
};

static float lightPos[4] =
{2.0, 4.0, 2.0, 1.0};
#if 0
static float lightDir[4] =
{-2.0, -4.0, -2.0, 1.0};
#endif
static float lightAmb[4] =
{0.2, 0.2, 0.2, 1.0};
static float lightDiff[4] =
{0.8, 0.8, 0.8, 1.0};
static float lightSpec[4] =
{0.4, 0.4, 0.4, 1.0};

static float groundPlane[4] =
{0.0, 1.0, 0.0, 1.499};
static float backPlane[4] =
{0.0, 0.0, 1.0, 0.899};

static float fogColor[4] =
{0.0, 0.0, 0.0, 0.0};
static float fogIndex[1] =
{0.0};

static unsigned char shadowPattern[128] =
{
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,  /* 50% Grey */
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55
};

static unsigned char sgiPattern[128] =
{
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  /* SGI Logo */
  0xff, 0xbd, 0xff, 0x83, 0xff, 0x5a, 0xff, 0xef,
  0xfe, 0xdb, 0x7f, 0xef, 0xfd, 0xdb, 0xbf, 0xef,
  0xfb, 0xdb, 0xdf, 0xef, 0xf7, 0xdb, 0xef, 0xef,
  0xfb, 0xdb, 0xdf, 0xef, 0xfd, 0xdb, 0xbf, 0x83,
  0xce, 0xdb, 0x73, 0xff, 0xb7, 0x5a, 0xed, 0xff,
  0xbb, 0xdb, 0xdd, 0xc7, 0xbd, 0xdb, 0xbd, 0xbb,
  0xbe, 0xbd, 0x7d, 0xbb, 0xbf, 0x7e, 0xfd, 0xb3,
  0xbe, 0xe7, 0x7d, 0xbf, 0xbd, 0xdb, 0xbd, 0xbf,
  0xbb, 0xbd, 0xdd, 0xbb, 0xb7, 0x7e, 0xed, 0xc7,
  0xce, 0xdb, 0x73, 0xff, 0xfd, 0xdb, 0xbf, 0xff,
  0xfb, 0xdb, 0xdf, 0x87, 0xf7, 0xdb, 0xef, 0xfb,
  0xf7, 0xdb, 0xef, 0xfb, 0xfb, 0xdb, 0xdf, 0xfb,
  0xfd, 0xdb, 0xbf, 0xc7, 0xfe, 0xdb, 0x7f, 0xbf,
  0xff, 0x5a, 0xff, 0xbf, 0xff, 0xbd, 0xff, 0xc3,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static float cube_vertexes[6][4][4] =
{
  {
    {-1.0, -1.0, -1.0, 1.0},
    {-1.0, -1.0, 1.0, 1.0},
    {-1.0, 1.0, 1.0, 1.0},
    {-1.0, 1.0, -1.0, 1.0}},

  {
    {1.0, 1.0, 1.0, 1.0},
    {1.0, -1.0, 1.0, 1.0},
    {1.0, -1.0, -1.0, 1.0},
    {1.0, 1.0, -1.0, 1.0}},

  {
    {-1.0, -1.0, -1.0, 1.0},
    {1.0, -1.0, -1.0, 1.0},
    {1.0, -1.0, 1.0, 1.0},
    {-1.0, -1.0, 1.0, 1.0}},

  {
    {1.0, 1.0, 1.0, 1.0},
    {1.0, 1.0, -1.0, 1.0},
    {-1.0, 1.0, -1.0, 1.0},
    {-1.0, 1.0, 1.0, 1.0}},

  {
    {-1.0, -1.0, -1.0, 1.0},
    {-1.0, 1.0, -1.0, 1.0},
    {1.0, 1.0, -1.0, 1.0},
    {1.0, -1.0, -1.0, 1.0}},

  {
    {1.0, 1.0, 1.0, 1.0},
    {-1.0, 1.0, 1.0, 1.0},
    {-1.0, -1.0, 1.0, 1.0},
    {1.0, -1.0, 1.0, 1.0}}
};

static float cube_normals[6][4] =
{
  {-1.0, 0.0, 0.0, 0.0},
  {1.0, 0.0, 0.0, 0.0},
  {0.0, -1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0, 0.0},
  {0.0, 0.0, -1.0, 0.0},
  {0.0, 0.0, 1.0, 0.0}
};

static void
usage(void)
{
  printf("\n");
  printf("usage: scube [options]\n");
  printf("\n");
  printf("    display a spinning cube and its shadow\n");
  printf("\n");
  printf("  Options:\n");
  printf("    -geometry  window size and location\n");
  printf("    -c         toggle color index mode\n");
  printf("    -l         toggle lighting\n");
  printf("    -f         toggle fog\n");
  printf("    -db        toggle double buffering\n");
  printf("    -logo      toggle sgi logo for the shadow pattern\n");
  printf("    -quads     toggle use of GL_QUADS to draw the checkerboard\n");
  printf("\n");
#ifndef EXIT_FAILURE    /* should be defined by ANSI C
                           <stdlib.h> */
#define EXIT_FAILURE 1
#endif
  exit(EXIT_FAILURE);
}

void
buildColormap(void)
{
  if (useRGB) {
    return;
  } else {
    int mapSize = 1 << glutGet(GLUT_WINDOW_BUFFER_SIZE);
    int rampSize = mapSize / 8;
    int entry;
    int i;

    for (entry = 0; entry < mapSize; ++entry) {
      int hue = entry / rampSize;
      GLfloat val = (entry % rampSize) * (1.0 / (rampSize - 1));
      GLfloat red, green, blue;

      red = (hue == 0 || hue == 1 || hue == 5 || hue == 6) ? val : 0;
      green = (hue == 0 || hue == 2 || hue == 4 || hue == 6) ? val : 0;
      blue = (hue == 0 || hue == 3 || hue == 4 || hue == 5) ? val : 0;

      glutSetColor(entry, red, green, blue);
    }

    for (i = 0; i < 8; ++i) {
      materialColor[i][0] = i * rampSize + 0.2 * (rampSize - 1);
      materialColor[i][1] = i * rampSize + 0.8 * (rampSize - 1);
      materialColor[i][2] = i * rampSize + 1.0 * (rampSize - 1);
      materialColor[i][3] = 0.0;
    }

    fogIndex[0] = -0.2 * (rampSize - 1);
  }
}

static void
setColor(int c)
{
  if (useLighting) {
    if (useRGB) {
      glMaterialfv(GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE, &materialColor[c][0]);
    } else {
      glMaterialfv(GL_FRONT_AND_BACK,
        GL_COLOR_INDEXES, &materialColor[c][0]);
    }
  } else {
    if (useRGB) {
      glColor4fv(&materialColor[c][0]);
    } else {
      glIndexf(materialColor[c][1]);
    }
  }
}

static void
drawCube(int color)
{
  int i;

  setColor(color);

  for (i = 0; i < 6; ++i) {
    glNormal3fv(&cube_normals[i][0]);
    glBegin(GL_POLYGON);
    glVertex4fv(&cube_vertexes[i][0][0]);
    glVertex4fv(&cube_vertexes[i][1][0]);
    glVertex4fv(&cube_vertexes[i][2][0]);
    glVertex4fv(&cube_vertexes[i][3][0]);
    glEnd();
  }
}

static void
drawCheck(int w, int h, int evenColor, int oddColor)
{
  static int initialized = 0;
  static int usedLighting = 0;
  static GLuint checklist = 0;

  if (!initialized || (usedLighting != useLighting)) {
    static float square_normal[4] =
    {0.0, 0.0, 1.0, 0.0};
    static float square[4][4];
    int i, j;

    if (!checklist) {
      checklist = glGenLists(1);
    }
    glNewList(checklist, GL_COMPILE_AND_EXECUTE);

    if (useQuads) {
      glNormal3fv(square_normal);
      glBegin(GL_QUADS);
    }
    for (j = 0; j < h; ++j) {
      for (i = 0; i < w; ++i) {
        square[0][0] = -1.0 + 2.0 / w * i;
        square[0][1] = -1.0 + 2.0 / h * (j + 1);
        square[0][2] = 0.0;
        square[0][3] = 1.0;

        square[1][0] = -1.0 + 2.0 / w * i;
        square[1][1] = -1.0 + 2.0 / h * j;
        square[1][2] = 0.0;
        square[1][3] = 1.0;

        square[2][0] = -1.0 + 2.0 / w * (i + 1);
        square[2][1] = -1.0 + 2.0 / h * j;
        square[2][2] = 0.0;
        square[2][3] = 1.0;

        square[3][0] = -1.0 + 2.0 / w * (i + 1);
        square[3][1] = -1.0 + 2.0 / h * (j + 1);
        square[3][2] = 0.0;
        square[3][3] = 1.0;

        if ((i & 1) ^ (j & 1)) {
          setColor(oddColor);
        } else {
          setColor(evenColor);
        }

        if (!useQuads) {
          glBegin(GL_POLYGON);
        }
        glVertex4fv(&square[0][0]);
        glVertex4fv(&square[1][0]);
        glVertex4fv(&square[2][0]);
        glVertex4fv(&square[3][0]);
        if (!useQuads) {
          glEnd();
        }
      }
    }

    if (useQuads) {
      glEnd();
    }
    glEndList();

    initialized = 1;
    usedLighting = useLighting;
  } else {
    glCallList(checklist);
  }
}

static void
myShadowMatrix(float ground[4], float light[4])
{
  float dot;
  float shadowMat[4][4];

  dot = ground[0] * light[0] +
    ground[1] * light[1] +
    ground[2] * light[2] +
    ground[3] * light[3];

  shadowMat[0][0] = dot - light[0] * ground[0];
  shadowMat[1][0] = 0.0 - light[0] * ground[1];
  shadowMat[2][0] = 0.0 - light[0] * ground[2];
  shadowMat[3][0] = 0.0 - light[0] * ground[3];

  shadowMat[0][1] = 0.0 - light[1] * ground[0];
  shadowMat[1][1] = dot - light[1] * ground[1];
  shadowMat[2][1] = 0.0 - light[1] * ground[2];
  shadowMat[3][1] = 0.0 - light[1] * ground[3];

  shadowMat[0][2] = 0.0 - light[2] * ground[0];
  shadowMat[1][2] = 0.0 - light[2] * ground[1];
  shadowMat[2][2] = dot - light[2] * ground[2];
  shadowMat[3][2] = 0.0 - light[2] * ground[3];

  shadowMat[0][3] = 0.0 - light[3] * ground[0];
  shadowMat[1][3] = 0.0 - light[3] * ground[1];
  shadowMat[2][3] = 0.0 - light[3] * ground[2];
  shadowMat[3][3] = dot - light[3] * ground[3];

  glMultMatrixf((const GLfloat *) shadowMat);
}

static char *windowNameRGBDB = "shadow cube (OpenGL RGB DB)";
static char *windowNameRGB = "shadow cube (OpenGL RGB)";
static char *windowNameIndexDB = "shadow cube (OpenGL Index DB)";
static char *windowNameIndex = "shadow cube (OpenGL Index)";

void
idle(void)
{
  tick++;
  if (tick >= 120) {
    tick = 0;
  }
  glutPostRedisplay();
}

/* ARGSUSED1 */
void
keyboard(unsigned char ch, int x, int y)
{
  switch (ch) {
  case 27:             /* escape */
    exit(0);
    break;
  case 'L':
  case 'l':
    useLighting = !useLighting;
    useLighting ? glEnable(GL_LIGHTING) :
      glDisable(GL_LIGHTING);
    glutPostRedisplay();
    break;
  case 'F':
  case 'f':
    useFog = !useFog;
    useFog ? glEnable(GL_FOG) : glDisable(GL_FOG);
    glutPostRedisplay();
    break;
  case '1':
    glFogf(GL_FOG_MODE, GL_LINEAR);
    glutPostRedisplay();
    break;
  case '2':
    glFogf(GL_FOG_MODE, GL_EXP);
    glutPostRedisplay();
    break;
  case '3':
    glFogf(GL_FOG_MODE, GL_EXP2);
    glutPostRedisplay();
    break;
  case ' ':
    if (!moving) {
      idle();
      glutPostRedisplay();
    }
  }
}

void
display(void)
{
  GLfloat cubeXform[4][4];

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();
  glTranslatef(0.0, -1.5, 0.0);
  glRotatef(-90.0, 1, 0, 0);
  glScalef(2.0, 2.0, 2.0);

  drawCheck(6, 6, BLUE, YELLOW);  /* draw ground */
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0.0, 0.0, -0.9);
  glScalef(2.0, 2.0, 2.0);

  drawCheck(6, 6, BLUE, YELLOW);  /* draw back */
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0.0, 0.2, 0.0);
  glScalef(0.3, 0.3, 0.3);
  glRotatef((360.0 / (30 * 1)) * tick, 1, 0, 0);
  glRotatef((360.0 / (30 * 2)) * tick, 0, 1, 0);
  glRotatef((360.0 / (30 * 4)) * tick, 0, 0, 1);
  glScalef(1.0, 2.0, 1.0);
  glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) cubeXform);

  drawCube(RED);        /* draw cube */
  glPopMatrix();

  glDepthMask(GL_FALSE);
  if (useRGB) {
    glEnable(GL_BLEND);
  } else {
    glEnable(GL_POLYGON_STIPPLE);
  }
  if (useFog) {
    glDisable(GL_FOG);
  }
  glPushMatrix();
  myShadowMatrix(groundPlane, lightPos);
  glTranslatef(0.0, 0.0, 2.0);
  glMultMatrixf((const GLfloat *) cubeXform);

  drawCube(BLACK);      /* draw ground shadow */
  glPopMatrix();

  glPushMatrix();
  myShadowMatrix(backPlane, lightPos);
  glTranslatef(0.0, 0.0, 2.0);
  glMultMatrixf((const GLfloat *) cubeXform);

  drawCube(BLACK);      /* draw back shadow */
  glPopMatrix();

  glDepthMask(GL_TRUE);
  if (useRGB) {
    glDisable(GL_BLEND);
  } else {
    glDisable(GL_POLYGON_STIPPLE);
  }
  if (useFog) {
    glEnable(GL_FOG);
  }
  if (useDB) {
    glutSwapBuffers();
  } else {
    glFlush();
  }
}

void
fog_select(int fog)
{
  glFogf(GL_FOG_MODE, fog);
  glutPostRedisplay();
}

void
menu_select(int mode)
{
  switch (mode) {
  case 1:
    moving = 1;
    glutIdleFunc(idle);
    break;
  case 2:
    moving = 0;
    glutIdleFunc(NULL);
    break;
  case 3:
    useFog = !useFog;
    useFog ? glEnable(GL_FOG) : glDisable(GL_FOG);
    glutPostRedisplay();
    break;
  case 4:
    useLighting = !useLighting;
    useLighting ? glEnable(GL_LIGHTING) :
      glDisable(GL_LIGHTING);
    glutPostRedisplay();
    break;
  case 5:
    exit(0);
    break;
  }
}

void
visible(int state)
{
  if (state == GLUT_VISIBLE) {
    if (moving)
      glutIdleFunc(idle);
  } else {
    if (moving)
      glutIdleFunc(NULL);
  }
}

int
main(int argc, char **argv)
{
  int width = 350, height = 350;
  int i;
  char *name;
  int fog_menu;

  glutInitWindowSize(width, height);
  glutInit(&argc, argv);
  /* process commmand line args */
  for (i = 1; i < argc; ++i) {
    if (!strcmp("-c", argv[i])) {
      useRGB = !useRGB;
    } else if (!strcmp("-l", argv[i])) {
      useLighting = !useLighting;
    } else if (!strcmp("-f", argv[i])) {
      useFog = !useFog;
    } else if (!strcmp("-db", argv[i])) {
      useDB = !useDB;
    } else if (!strcmp("-logo", argv[i])) {
      useLogo = !useLogo;
    } else if (!strcmp("-quads", argv[i])) {
      useQuads = !useQuads;
    } else {
      usage();
    }
  }

  /* choose visual */
  if (useRGB) {
    if (useDB) {
      glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
      name = windowNameRGBDB;
    } else {
      glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
      name = windowNameRGB;
    }
  } else {
    if (useDB) {
      glutInitDisplayMode(GLUT_DOUBLE | GLUT_INDEX | GLUT_DEPTH);
      name = windowNameIndexDB;
    } else {
      glutInitDisplayMode(GLUT_SINGLE | GLUT_INDEX | GLUT_DEPTH);
      name = windowNameIndex;
    }
  }

  glutCreateWindow(name);

  buildColormap();

  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glutVisibilityFunc(visible);

  fog_menu = glutCreateMenu(fog_select);
  glutAddMenuEntry("Linear fog", GL_LINEAR);
  glutAddMenuEntry("Exp fog", GL_EXP);
  glutAddMenuEntry("Exp^2 fog", GL_EXP2);

  glutCreateMenu(menu_select);
  glutAddMenuEntry("Start motion", 1);
  glutAddMenuEntry("Stop motion", 2);
  glutAddMenuEntry("Toggle fog", 3);
  glutAddMenuEntry("Toggle lighting", 4);
  glutAddSubMenu("Fog type", fog_menu);
  glutAddMenuEntry("Quit", 5);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  /* setup context */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 3.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -2.0);

  glEnable(GL_DEPTH_TEST);

  if (useLighting) {
    glEnable(GL_LIGHTING);
  }
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
  glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
#if 0
  glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, lightDir);
  glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 80);
  glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 25);
#endif

  glEnable(GL_NORMALIZE);

  if (useFog) {
    glEnable(GL_FOG);
  }
  glFogfv(GL_FOG_COLOR, fogColor);
  glFogfv(GL_FOG_INDEX, fogIndex);
  glFogf(GL_FOG_MODE, GL_EXP);
  glFogf(GL_FOG_DENSITY, 0.5);
  glFogf(GL_FOG_START, 1.0);
  glFogf(GL_FOG_END, 3.0);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glShadeModel(GL_SMOOTH);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  if (useLogo) {
    glPolygonStipple((const GLubyte *) sgiPattern);
  } else {
    glPolygonStipple((const GLubyte *) shadowPattern);
  }

  glClearColor(0.0, 0.0, 0.0, 1);
  glClearIndex(0);
  glClearDepth(1);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
