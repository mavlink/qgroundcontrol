
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

void
display(void)
{
  glDrawBuffer(GL_BACK_LEFT);
  glClearColor(1.0, 0.0, 0.0, 1.0); /* red */
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawBuffer(GL_BACK_RIGHT);
  glClearColor(0.0, 0.0, 1.0, 1.0); /* blue */
  glClear(GL_COLOR_BUFFER_BIT);
  glutSwapBuffers();
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_STEREO);
  glutCreateWindow("stereo example");
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
