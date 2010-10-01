
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This tests creation of a subwindow that gets "popped" to
   check if it also (erroneously) gets moved.  GLUT 3.5 had
   this bug (fixed in GLUT 3.6). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

int parent, child;
int parentDrawn = 0, childDrawn = 0;

/* ARGSUSED */
void
failTest(int value)
{
  printf("FAIL: test27\n");
  exit(1);
}

/* ARGSUSED */
void
passTest(int value)
{
  printf("PASS: test27\n");
  exit(0);
}

void
installFinish(void)
{
  if (childDrawn && parentDrawn) {
    glutTimerFunc(1000, passTest, 0);
  }
}

void
output(GLfloat x, GLfloat y, char *string)
{
  int len, i;

  glRasterPos2f(x, y);
  len = (int) strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, string[i]);
  }
}

void
displayParent(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
  parentDrawn++;
  installFinish();
}

void
displayChild(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  output(-0.4, 0.5, "this");
  output(-0.8, 0.1, "subwindow");
  output(-0.8, -0.3, "should be");
  output(-0.7, -0.7, "centered");
  glFlush();
  childDrawn++;
  installFinish();
}

int
main(int argc, char **argv)
{
  int possible;

  glutInit(&argc, argv);
  glutInitWindowSize(300, 300);
  glutInitWindowPosition(5, 5);
  glutInitDisplayMode(GLUT_RGB);
  parent = glutCreateWindow("test27");
  glClearColor(1.0, 0.0, 0.0, 0.0);
  glutDisplayFunc(displayParent);
  possible = glutGet(GLUT_DISPLAY_MODE_POSSIBLE);
  if (possible != 1) {
    printf("FAIL: glutGet returned display mode not possible: %d\n", possible);
    exit(1);
  }
  child = glutCreateSubWindow(parent, 100, 100, 100, 100);
  glClearColor(0.0, 1.0, 0.0, 0.0);
  glColor3f(0.0, 0.0, 0.0);
  glutDisplayFunc(displayChild);
  glutPopWindow();

  glutTimerFunc(10000, failTest, 0);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
