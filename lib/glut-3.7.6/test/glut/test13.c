
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(1000 * x)
#else
#include <unistd.h>
#endif
#include <GL/glut.h>

int window1, window2;
int win1reshaped = 0, win2reshaped = 0;
int win1displayed = 0, win2displayed = 0;

void
checkifdone(void)
{
  if (win1reshaped && win2reshaped && win1displayed && win2displayed) {
    sleep(1);
    printf("PASS: test13\n");
    exit(0);
  }
}

void
window1reshape(int w, int h)
{
  if (glutGetWindow() != window1) {
    printf("FAIL: window1reshape\n");
    exit(1);
  }
  glViewport(0, 0, w, h);
  win1reshaped = 1;
}

void
window1display(void)
{
  if (glutGetWindow() != window1) {
    printf("FAIL: window1display\n");
    exit(1);
  }
  glClearColor(0, 1, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
  win1displayed = 1;
  checkifdone();
}

void
window2reshape(int w, int h)
{
  if (glutGetWindow() != window2) {
    printf("FAIL: window2reshape\n");
    exit(1);
  }
  glViewport(0, 0, w, h);
  win2reshaped = 1;
}

void
window2display(void)
{
  if (glutGetWindow() != window2) {
    printf("FAIL: window2display\n");
    exit(1);
  }
  glClearColor(0, 0, 1, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
  win2displayed = 1;
  checkifdone();
}

/* ARGSUSED */
void
timefunc(int value)
{
  printf("FAIL: test13\n");
  exit(1);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);

  glutInitWindowSize(100, 100);
  glutInitWindowPosition(50, 100);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  window1 = glutCreateWindow("1");
  if (glutGet(GLUT_WINDOW_X) != 50) {
    printf("FAIL: test13\n");
    exit(1);
  }
  if (glutGet(GLUT_WINDOW_Y) != 100) {
    printf("FAIL: test13\n");
    exit(1);
  }
  glutReshapeFunc(window1reshape);
  glutDisplayFunc(window1display);

  glutInitWindowSize(100, 100);
  glutInitWindowPosition(250, 100);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  window2 = glutCreateWindow("2");
  if (glutGet(GLUT_WINDOW_X) != 250) {
    printf("FAIL: test13\n");
    exit(1);
  }
  if (glutGet(GLUT_WINDOW_Y) != 100) {
    printf("FAIL: test13\n");
    exit(1);
  }
  glutReshapeFunc(window2reshape);
  glutDisplayFunc(window2display);

  glutTimerFunc(7000, timefunc, 1);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
