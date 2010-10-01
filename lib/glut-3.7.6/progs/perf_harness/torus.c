
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <string.h>
#include <GL/glut.h>

/* Modify these variables if necessary to control the number of
   iterations for per time sample, the GLUT display mode for the
   window, and the minimum test running time in seconds. */
extern int testIterationsStep, testDisplayMode, testMinimumTestTime;

void
testInit(int argc, char **argv, int width, int height)
{
  static GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};
  static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
  int solid, i;

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  gluPerspective(25.0, width/height, 1.0, 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,
    0.0, 0.0, 0.0,
    0.0, 1.0, 0.);
  glTranslatef(0.0, 0.0, -1.0);

  glColor3f(1.0, 0.0, 0.0);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  solid = 1;
  for (i = 1; i < argc; i++) {
    if (!strcmp("-light", argv[i])) {
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
    } else if (!strcmp("-depth", argv[i])) {
      glEnable(GL_DEPTH_TEST);
    } else if (!strcmp("-wire", argv[i])) {
      solid = 0;
    }
  }
  glNewList(1, GL_COMPILE);
  if (solid) {
    glutSolidTorus(0.25, 0.75, 100, 100);
  } else {
    glutWireTorus(0.25, 0.75, 100, 100);
  }
  glEndList();
}

void
testRender(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glCallList(1);
}
