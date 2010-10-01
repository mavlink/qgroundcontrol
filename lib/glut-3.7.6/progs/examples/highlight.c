
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
/*
 *  scene.c
 *  This program demonstrates the use of the GL lighting model.
 *  Objects are drawn using a grey material characteristic. 
 *  A single light source illuminates the objects.
 */
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <GL/glut.h>

#define BUFSIZE 512

#define TORUS		1
#define TETRAHEDRON	2
#define ICOSAHEDRON	3

GLuint selectBuf[BUFSIZE];

int W = 500, H = 500;
GLfloat x, y;
int locating = 0;
GLuint theObject = 0;
int menu_inuse = 0;
int mouse_state = 0;

char *objectNames[] =
{"Nothing", "Torus", "Tetrahedron", "Icosahedron"};

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

/* Initialize material property and light source. */
void
myinit(void)
{
  GLfloat light_ambient[] =
  {0.2, 0.2, 0.2, 1.0};
  GLfloat light_diffuse[] =
  {1.0, 1.0, 1.0, 1.0};
  GLfloat light_specular[] =
  {1.0, 1.0, 1.0, 1.0};
  GLfloat light_position[] =
  {1.0, 1.0, 1.0, 0.0};

  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glEnable(GL_LIGHT0);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  glSelectBuffer(BUFSIZE, selectBuf);

  glNewList(TORUS, GL_COMPILE);
  glutSolidTorus(0.275, 0.85, 10, 15);
  glEndList();
  glNewList(TETRAHEDRON, GL_COMPILE);
  glutSolidTetrahedron();
  glEndList();
  glNewList(ICOSAHEDRON, GL_COMPILE);
  glutSolidIcosahedron();
  glEndList();
}

void
highlightBegin(void)
{
  static GLfloat red[4] =
  {1.0, 0.0, 0.0, 1.0};

  glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, red);
  glColor3f(1.0, 0.0, 0.0);
}

void
highlightEnd(void)
{
  glPopAttrib();
}

void
draw(void)
{
  glPushMatrix();
  glScalef(1.3, 1.3, 1.3);
  glRotatef(20.0, 1.0, 0.0, 0.0);

  glLoadName(2);
  glPushMatrix();
  if (theObject == 2)
    highlightBegin();
  glTranslatef(-0.75, -0.5, 0.0);
  glRotatef(270.0, 1.0, 0.0, 0.0);
  glCallList(TETRAHEDRON);
  if (theObject == 2)
    highlightEnd();
  glPopMatrix();

  glLoadName(1);
  glPushMatrix();
  if (theObject == 1)
    highlightBegin();
  glTranslatef(-0.75, 0.5, 0.0);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  glCallList(TORUS);
  if (theObject == 1)
    highlightEnd();
  glPopMatrix();

  glLoadName(3);
  glPushMatrix();
  if (theObject == 3)
    highlightBegin();
  glTranslatef(0.75, 0.0, -1.0);
  glCallList(ICOSAHEDRON);
  if (theObject == 3)
    highlightEnd();
  glPopMatrix();

  glPopMatrix();
}

void
myortho(void)
{
  if (W <= H)
    glOrtho(-2.5, 2.5, -2.5 * (GLfloat) H / (GLfloat) W,
      2.5 * (GLfloat) H / (GLfloat) W, -10.0, 10.0);
  else
    glOrtho(-2.5 * (GLfloat) W / (GLfloat) H,
      2.5 * (GLfloat) W / (GLfloat) H, -2.5, 2.5, -10.0, 10.0);
}

/*  processHits() prints out the contents of the 
 *  selection array.
 */
void
processHits(GLint hits, GLuint buffer[])
{
  GLuint depth = (GLuint) ~0;
  unsigned int getThisName;
  GLint i;
  GLuint names, *ptr;
  GLuint newObject;

  ptr = (GLuint *) buffer;
  newObject = 0;
  for (i = 0; i < hits; i++) {  /* for each hit  */
    getThisName = 0;
    names = *ptr;
    ptr++;              /* skip # name */
    if (*ptr <= depth) {
      depth = *ptr;
      getThisName = 1;
    }
    ptr++;              /* skip z1 */
    if (*ptr <= depth) {
      depth = *ptr;
      getThisName = 1;
    }
    ptr++;              /* skip z2 */

    if (getThisName)
      newObject = *ptr;
    ptr += names;       /* skip the names list */
  }
  if (theObject != newObject) {
    theObject = newObject;
    glutPostRedisplay();
  }
}

/* ARGSUSED */
void
locate(int value)
{
  GLint viewport[4];
  GLint hits;

  if (locating) {
    if (mouse_state == GLUT_ENTERED) {
      (void) glRenderMode(GL_SELECT);
      glInitNames();
      glPushName(-1);

      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      viewport[0] = 0;
      viewport[1] = 0;
      viewport[2] = W;
      viewport[3] = H;
      gluPickMatrix(x, H - y, 5.0, 5.0, viewport);
      myortho();
      glMatrixMode(GL_MODELVIEW);
      draw();
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      hits = glRenderMode(GL_RENDER);
    } else {
      hits = 0;
    }
    processHits(hits, selectBuf);
  }
  locating = 0;
}

void
passive(int newx, int newy)
{
  x = newx;
  y = newy;
  if (!locating) {
    locating = 1;
    glutTimerFunc(1, locate, 0);
  }
}

void
entry(int state)
{
  mouse_state = state;
  if (!menu_inuse) {
    if (state == GLUT_LEFT) {
      if (theObject != 0) {
        theObject = 0;
        glutPostRedisplay();
      }
    }
  }
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw();

  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_LINE_SMOOTH);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, 3000, 0, 3000);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  output(80, 2800, "Automatically names object under mouse.");
  output(80, 100, "Located: %s.", objectNames[theObject]);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopAttrib();

  glutSwapBuffers();
}

void
myReshape(int w, int h)
{
  W = w;
  H = h;
  glViewport(0, 0, W, H);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  myortho();
  glMatrixMode(GL_MODELVIEW);
}

void
polygon_mode(int value)
{
  switch (value) {
  case 1:
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case 2:
    glDisable(GL_LIGHTING);
    glColor3f(1.0, 1.0, 1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    break;
  }
  glutPostRedisplay();
}

void
mstatus(int status, int newx, int newy)
{
  if (status == GLUT_MENU_NOT_IN_USE) {
    menu_inuse = 0;
    passive(newx, newy);
  } else {
    menu_inuse = 1;
  }
}

void
main_menu(int value)
{
  if (value == 666)
    exit(0);
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int
main(int argc, char **argv)
{
  int submenu;

  glutInit(&argc, argv);
  glutInitWindowSize(W, H);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow(argv[0]);
  myinit();
  glutReshapeFunc(myReshape);
  glutDisplayFunc(display);
  submenu = glutCreateMenu(polygon_mode);
  glutAddMenuEntry("Filled", 1);
  glutAddMenuEntry("Outline", 2);
  glutCreateMenu(main_menu);
  glutAddMenuEntry("Quit", 666);
  glutAddSubMenu("Polygon mode", submenu);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutPassiveMotionFunc(passive);
  glutEntryFunc(entry);
  glutMenuStatusFunc(mstatus);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
