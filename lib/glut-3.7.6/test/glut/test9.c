
/* Copyright (c) Mark J. Kilgard, 1994, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

int main_w, w1, w2, w3, w4;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glutSwapBuffers();
}

/* ARGSUSED */
void
time8(int value)
{
  printf("PASS: test9\n");
  exit(0);
}

/* ARGSUSED */
void
time7(int value)
{
  glutDestroyWindow(main_w);
  glutTimerFunc(500, time8, 0);
}

/* ARGSUSED */
void
time6(int value)
{
  glutDestroyWindow(w1);
  glutTimerFunc(500, time7, 0);
  glutInitDisplayMode(GLUT_INDEX);
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
    printf("UNRESOLVED: test9 (your OpenGL lacks color index support)\n");
    exit(0);
  }
  w1 = glutCreateSubWindow(main_w, 10, 10, 10, 10);
  glutDisplayFunc(display);
  w2 = glutCreateSubWindow(w1, 10, 10, 30, 30);
  glutDisplayFunc(display);
  w3 = glutCreateSubWindow(w2, 10, 10, 50, 50);
  glutDisplayFunc(display);
  glutInitDisplayMode(GLUT_RGB);
  w4 = glutCreateSubWindow(w3, 10, 10, 70, 70);
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glutDisplayFunc(display);
}

/* ARGSUSED */
void
time5(int value)
{
  w1 = glutCreateSubWindow(main_w, 10, 10, 10, 10);
  glutDisplayFunc(display);
  w2 = glutCreateSubWindow(w1, 10, 10, 30, 30);
  glutDisplayFunc(display);
  w3 = glutCreateSubWindow(w2, 10, 10, 50, 50);
  glutDisplayFunc(display);
  glutInitDisplayMode(GLUT_RGB);
  w4 = glutCreateSubWindow(w3, 10, 10, 70, 70);
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glutDisplayFunc(display);
  glutTimerFunc(500, time6, 0);
}

/* ARGSUSED */
void
time4(int value)
{
  glutDestroyWindow(w4);
  glutTimerFunc(500, time5, 0);
}

/* ARGSUSED */
void
time3(int value)
{
  glutDestroyWindow(w3);
  glutTimerFunc(500, time4, 0);
}

/* ARGSUSED */
void
time2(int value)
{
  glutDestroyWindow(w2);
  glutTimerFunc(500, time3, 0);
}

/* ARGSUSED */
void
time1(int value)
{
  glutDestroyWindow(w1);
  glutTimerFunc(500, time2, 0);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGB);
  main_w = glutCreateWindow("test9");
  glClearColor(0.0, 0.0, 0.0, 0.0);  /* black */
  glutDisplayFunc(display);
  glutInitDisplayMode(GLUT_INDEX);
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
    printf("UNRESOLVED: test9 (your OpenGL lacks color index support)\n");
    exit(0);
  }
  w1 = glutCreateSubWindow(main_w, 10, 10, 10, 10);
  glutSetColor(1, 1.0, 0.0, 0.0);  /* red */
  glutSetColor(2, 0.0, 1.0, 0.0);  /* green */
  glutSetColor(3, 0.0, 0.0, 1.0);  /* blue */
  glClearIndex(1);
  glutDisplayFunc(display);
  w2 = glutCreateSubWindow(main_w, 30, 30, 10, 10);
  glutCopyColormap(w1);
  glClearIndex(2);
  glutDisplayFunc(display);
  w3 = glutCreateSubWindow(main_w, 50, 50, 10, 10);
  glutCopyColormap(w1);
  glClearIndex(3);
  glutDisplayFunc(display);
  w4 = glutCreateSubWindow(main_w, 70, 70, 10, 10);
  glutCopyColormap(w1);
  glutSetColor(3, 1.0, 1.0, 1.0);  /* white */
  glClearIndex(3);
  glutDisplayFunc(display);
  glutTimerFunc(750, time1, 0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
