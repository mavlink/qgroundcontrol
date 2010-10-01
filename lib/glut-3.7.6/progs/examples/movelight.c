
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/**
 * (c) Copyright 1993, Silicon Graphics, Inc.
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
/**
 *  movelight.c
 *  This program demonstrates when to issue lighting and 
 *  transformation commands to render a model with a light 
 *  which is moved by a modeling transformation (rotate or 
 *  translate).  The light position is reset after the modeling 
 *  transformation is called.  The eye position does not change.
 *
 *  A sphere is drawn using a grey material characteristic. 
 *  A single light source illuminates the object.
 *
 *  Interaction:  pressing the left or middle mouse button
 *  alters the modeling transformation (x rotation) by 30 degrees.  
 *  The scene is then redrawn with the light in a new position.
 */
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <GL/glut.h>

#define TORUS 0
#define TEAPOT 1
#define DOD 2
#define TET 3
#define ISO 4
#define QUIT 5

static int spin = 0;
static int obj = TORUS;
static int begin;

void
output(GLfloat x, GLfloat y, char *format,...)
{
  va_list args;
  char buffer[200], *p;

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  glPushMatrix();
  glTranslatef(x, y, 0);
  for (p = buffer; *p; p++)
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
  glPopMatrix();
}

void
menu_select(int item)
{
  if (item == QUIT)
    exit(0);
  obj = item;
  glutPostRedisplay();
}

/* ARGSUSED2 */
void
movelight(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    begin = x;
  }
}

/* ARGSUSED1 */
void
motion(int x, int y)
{
  spin = (spin + (x - begin)) % 360;
  begin = x;
  glutPostRedisplay();
}

void
myinit(void)
{
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);
}

/*  Here is where the light position is reset after the modeling
 *  transformation (glRotated) is called.  This places the 
 *  light at a new position in world coordinates.  The cube
 *  represents the position of the light.
 */
void
display(void)
{
  GLfloat position[] =
  {0.0, 0.0, 1.5, 1.0};

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0.0, 0.0, -5.0);

  glPushMatrix();
  glRotated((GLdouble) spin, 0.0, 1.0, 0.0);
  glRotated(0.0, 1.0, 0.0, 0.0);
  glLightfv(GL_LIGHT0, GL_POSITION, position);

  glTranslated(0.0, 0.0, 1.5);
  glDisable(GL_LIGHTING);
  glColor3f(0.0, 1.0, 1.0);
  glutWireCube(0.1);
  glEnable(GL_LIGHTING);
  glPopMatrix();

  switch (obj) {
  case TORUS:
    glutSolidTorus(0.275, 0.85, 20, 20);
    break;
  case TEAPOT:
    glutSolidTeapot(1.0);
    break;
  case DOD:
    glPushMatrix();
    glScalef(.5, .5, .5);
    glutSolidDodecahedron();
    glPopMatrix();
    break;
  case TET:
    glutSolidTetrahedron();
    break;
  case ISO:
    glutSolidIcosahedron();
    break;
  }

  glPopMatrix();
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, 3000, 0, 3000);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  output(80, 2800, "Welcome to movelight.");
  output(80, 2650, "Right mouse button for menu.");
  output(80, 400, "Hold down the left mouse button");
  output(80, 250, "and move the mouse horizontally");
  output(80, 100, "to change the light position.");
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
  glutSwapBuffers();
}

void
myReshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(40.0, (GLfloat) w / (GLfloat) h, 1.0, 20.0);
  glMatrixMode(GL_MODELVIEW);
}

void
tmotion(int x, int y)
{
  printf("Tablet motion x = %d, y = %d\n", x, y);
}

void
tbutton(int b, int s, int x, int y)
{
  printf("b = %d, s = %d, x = %d, y = %d\n", b, s, x, y);
}

void
smotion(int x, int y, int z)
{
  fprintf(stderr, "Spaceball motion %d %d %d\n", x, y, z);
}

void
rmotion(int x, int y, int z)
{
  fprintf(stderr, "Spaceball rotate %d %d %d\n", x, y, z);
}

void
sbutton(int button, int state)
{
  fprintf(stderr, "Spaceball button %d is %s\n",
    button, state == GLUT_UP ? "up" : "down");
}

void
dials(int dial, int value)
{
  fprintf(stderr, "Dial %d is %d\n", dial, value);
  spin = value % 360;
  glutPostRedisplay();
}

void
buttons(int button, int state)
{
  fprintf(stderr, "Button %d is %s\n", button,
    state == GLUT_UP ? "up" : "down");
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(500, 500);
  glutCreateWindow(argv[0]);
  myinit();
  glutMouseFunc(movelight);
  glutMotionFunc(motion);
  glutReshapeFunc(myReshape);
  glutDisplayFunc(display);
  glutTabletMotionFunc(tmotion);
  glutTabletButtonFunc(tbutton);
  glutSpaceballMotionFunc(smotion);
  glutSpaceballRotateFunc(rmotion);
  glutSpaceballButtonFunc(sbutton);
  glutDialsFunc(dials);
  glutButtonBoxFunc(buttons);
  glutCreateMenu(menu_select);
  glutAddMenuEntry("Torus", TORUS);
  glutAddMenuEntry("Teapot", TEAPOT);
  glutAddMenuEntry("Dodecahedron", DOD);
  glutAddMenuEntry("Tetrahedron", TET);
  glutAddMenuEntry("Icosahedron", ISO);
  glutAddMenuEntry("Quit", QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
