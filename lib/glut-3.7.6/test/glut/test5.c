
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

GLfloat comp;
int mask = 0;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
  mask |= 1 << glutGetWindow();
}

void
timeout(int value)
{
  if (value != 1) {
    printf("FAIL: test5\n");
    exit(1);
  }
  if (mask != 0x6) {
    printf("FAIL: test5\n");
    exit(1);
  }
  printf("PASS: test5\n");
  exit(0);
}

int
main(int argc, char **argv)
{
  int win1, win2, size;
  GLint isIndex;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_INDEX);
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
    printf("UNRESOLVED: test5 (your OpenGL lacks color index support)\n");
    exit(0);
  }
  glutInitWindowPosition(45, 45);
  win1 = glutCreateWindow("CI test 1");
  glGetIntegerv(GL_INDEX_MODE, &isIndex);
  if (isIndex == 0) {
    printf("FAIL: test5\n");
    exit(1);
  }
  size = glutGet(GLUT_WINDOW_COLORMAP_SIZE);
  if (size <= 1 || size > (1 << glutGet(GLUT_WINDOW_BUFFER_SIZE))) {
    printf("FAIL: test5\n");
    exit(1);
  }
  glutSetColor(2, 1.0, 3.4, 0.5);
  comp = glutGetColor(2, GLUT_RED);
  if (comp != 1.0) {
    printf("FAIL: test5\n");
    exit(1);
  }
  comp = glutGetColor(2, GLUT_GREEN);
  if (comp != 1.0) {
    printf("FAIL: test5\n");
    exit(1);
  }
  comp = glutGetColor(2, GLUT_BLUE);
  if (comp != 0.5) {
    printf("FAIL: test5\n");
    exit(1);
  }
  glutInitWindowPosition(450, 450);
  win2 = glutCreateWindow("CI test 2");
  glutCopyColormap(win1);
  if (glutGetColor(2, GLUT_RED) != 1.0) {
    printf("FAIL: test5\n");
    exit(1);
  }
  if (glutGetColor(2, GLUT_GREEN) != 1.0) {
    printf("FAIL: test5\n");
    exit(1);
  }
  if (glutGetColor(2, GLUT_BLUE) != 0.5) {
    printf("FAIL: test5\n");
    exit(1);
  }
  glutSetColor(2, -1.0, 0.25, 0.75);
  glutSetWindow(win1);
  if (win1 != glutGetWindow()) {
    printf("FAIL: test5\n");
    exit(1);
  }
  glutDisplayFunc(display);
  if (glutGetColor(2, GLUT_RED) != 1.0) {
    printf("FAIL: test5\n");
    exit(1);
  }
  if (glutGetColor(2, GLUT_GREEN) != 1.0) {
    printf("FAIL: test5\n");
    exit(1);
  }
  if (glutGetColor(2, GLUT_BLUE) != 0.5) {
    printf("FAIL: test5\n");
    exit(1);
  }
  glutSetWindow(win2);
  if (win2 != glutGetWindow()) {
    printf("FAIL: test5\n");
    exit(1);
  }
  if (glutGetColor(2, GLUT_RED) != 0.0) {
    printf("FAIL: test5\n");
    exit(1);
  }
  if (glutGetColor(2, GLUT_GREEN) != 0.25) {
    printf("FAIL: test5\n");
    exit(1);
  }
  if (glutGetColor(2, GLUT_BLUE) != 0.75) {
    printf("FAIL: test5\n");
    exit(1);
  }
  glutTimerFunc(1500, timeout, 1);
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
