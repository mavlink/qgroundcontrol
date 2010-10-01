
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <GL/glut.h>
#include <stdio.h>

#define GAP 10

int main_w, w1, w2, w3, w4;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

void
vis(int visState)
{
  printf("VIS: win=%d, v=%d\n", glutGetWindow(), visState);
}

void
reshape(int w, int h)
{
  int width = 50;
  int height = 50;

  glViewport(0, 0, w, h);
  if (w > 50) {
    width = (w - 3 * GAP) / 2;
  } else {
    width = 10;
  }
  if (h > 50) {
    height = (h - 3 * GAP) / 2;
  } else {
    height = 10;
  }
  glutSetWindow(w1);
  glutPositionWindow(GAP, GAP);
  glutReshapeWindow(width, height);
  glutSetWindow(w2);
  glutPositionWindow(GAP + width + GAP, GAP);
  glutReshapeWindow(width, height);
  glutSetWindow(w3);
  glutPositionWindow(GAP, GAP + height + GAP);
  glutReshapeWindow(width, height);
  glutSetWindow(w4);
  glutPositionWindow(GAP + width + GAP, GAP + height + GAP);
  glutReshapeWindow(width, height);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB);
  glutInitWindowSize(210, 210);
  main_w = glutCreateWindow("4 subwindows");
  glutDisplayFunc(display);
  glutVisibilityFunc(vis);
  glutReshapeFunc(reshape);
  glClearColor(1.0, 1.0, 1.0, 1.0);
  w1 = glutCreateSubWindow(main_w, 10, 10, 90, 90);
  glutDisplayFunc(display);
  glutVisibilityFunc(vis);
  glClearColor(1.0, 0.0, 0.0, 1.0);
  w2 = glutCreateSubWindow(main_w, 110, 10, 90, 90);
  glutDisplayFunc(display);
  glutVisibilityFunc(vis);
  glClearColor(0.0, 1.0, 0.0, 1.0);
  w3 = glutCreateSubWindow(main_w, 10, 110, 90, 90);
  glutDisplayFunc(display);
  glutVisibilityFunc(vis);
  glClearColor(0.0, 0.0, 1.0, 1.0);
  w4 = glutCreateSubWindow(main_w, 110, 110, 90, 90);
  glutDisplayFunc(display);
  glutVisibilityFunc(vis);
  glClearColor(1.0, 1.0, 0.0, 1.0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
