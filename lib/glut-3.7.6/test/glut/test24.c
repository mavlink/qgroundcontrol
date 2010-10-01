
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This tests various obscure interactions in menu creation and 
   destruction, including the support for Sun's Creator 3D
   overlay "high cell" overlay menu colormap cell allocation. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFinish();
}

void
timer(int value)
{
  if (value != 23) {
    printf("FAIL: bad timer value\n");
    exit(1);
  }
  printf("PASS: test24\n");
  exit(0);
}

/* ARGSUSED */
void
menuSelect(int value)
{
}

int
main(int argc, char **argv)
{
  int win1, win2, men1, men2, men3;

  glutInit(&argc, argv);

  if (0 != glutGetMenu()) {
    printf("FAIL: current menu wrong, should be zero\n");
    exit(1);
  }
  if (0 != glutGetWindow()) {
    printf("FAIL: current window wrong, should be zero\n");
    exit(1);
  }
  glutInitWindowSize(140, 140);

  /* Make sure initial glut init display mode is right. */
  if (glutGet(GLUT_INIT_DISPLAY_MODE) != (GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH)) {
    printf("FAIL: init display mode wrong\n");
    exit(1);
  }
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_STENCIL);
  if (glutGet(GLUT_INIT_DISPLAY_MODE) != (GLUT_RGBA | GLUT_SINGLE | GLUT_STENCIL)) {
    printf("FAIL: display mode wrong\n");
    exit(1);
  }
  /* Interesting case:  creating menu before creating windows. */
  men1 = glutCreateMenu(menuSelect);

  /* Make sure glutCreateMenu doesn't change init display mode. 
   */
  if (glutGet(GLUT_INIT_DISPLAY_MODE) != (GLUT_RGBA | GLUT_SINGLE | GLUT_STENCIL)) {
    printf("FAIL: display mode changed\n");
    exit(1);
  }
  if (men1 != glutGetMenu()) {
    printf("FAIL: current menu wrong\n");
    exit(1);
  }
  glutAddMenuEntry("hello", 1);
  glutAddMenuEntry("bye", 2);
  glutAddMenuEntry("yes", 3);
  glutAddMenuEntry("no", 4);
  glutAddSubMenu("submenu", 5);

  win1 = glutCreateWindow("test24");
  glutDisplayFunc(display);

  if (win1 != glutGetWindow()) {
    printf("FAIL: current window wrong\n");
    exit(1);
  }
  if (men1 != glutGetMenu()) {
    printf("FAIL: current menu wrong\n");
    exit(1);
  }
  men2 = glutCreateMenu(menuSelect);
  glutAddMenuEntry("yes", 3);
  glutAddMenuEntry("no", 4);
  glutAddSubMenu("submenu", 5);

  /* Make sure glutCreateMenu doesn't change init display mode. 
   */
  if (glutGet(GLUT_INIT_DISPLAY_MODE) != (GLUT_RGBA | GLUT_SINGLE | GLUT_STENCIL)) {
    printf("FAIL: display mode changed\n");
    exit(1);
  }
  if (men2 != glutGetMenu()) {
    printf("FAIL: current menu wrong\n");
    exit(1);
  }
  if (win1 != glutGetWindow()) {
    printf("FAIL: current window wrong\n");
    exit(1);
  }
  win2 = glutCreateWindow("test24 second");
  glutDisplayFunc(display);

  if (win2 != glutGetWindow()) {
    printf("FAIL: current window wrong\n");
    exit(1);
  }
  glutDestroyWindow(win2);

  if (0 != glutGetWindow()) {
    printf("FAIL: current window wrong, should be zero\n");
    exit(1);
  }
  men3 = glutCreateMenu(menuSelect);
  glutAddMenuEntry("no", 4);
  glutAddSubMenu("submenu", 5);

  if (glutGet(GLUT_INIT_DISPLAY_MODE) != (GLUT_RGBA | GLUT_SINGLE | GLUT_STENCIL)) {
    printf("FAIL: display mode changed\n");
    exit(1);
  }
  glutDestroyMenu(men3);

  if (0 != glutGetMenu()) {
    printf("FAIL: current menu wrong, should be zero\n");
    exit(1);
  }
  glutTimerFunc(2 * 1000, timer, 23);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
