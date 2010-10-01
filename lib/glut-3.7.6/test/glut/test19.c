
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This test makes sure damaged gets set when a window is
   resized smaller. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

int width = -1, height = -1;
int displayCount = 0;

/* ARGSUSED */
void
done(int value)
{
  if (displayCount != 2) {
    fprintf(stderr, "FAIL: test19, damage expected\n");
    exit(1);
  }
  fprintf(stderr, "PASS: test19\n");
  exit(0);
}

void
reshape(int w, int h)
{
  printf("window reshaped: w=%d, h=%d\n", w, h);
  width = w;
  height = h;
}

void
display(void)
{
  if (glutLayerGet(GLUT_NORMAL_DAMAGED) == 0) {
    fprintf(stderr, "FAIL: test19, damage expected\n");
    exit(1);
  }
  displayCount++;
  if (width == -1 || height == -1) {
    fprintf(stderr, "FAIL: test19, reshape not called\n");
    exit(1);
  }
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
  if (displayCount == 1) {
    glutReshapeWindow(width / 2, height / 2);
    glutTimerFunc(1000, done, 0);
  }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutCreateWindow("test19");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
