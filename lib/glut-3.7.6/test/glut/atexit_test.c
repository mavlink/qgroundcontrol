
/* Copyright (c) Mark J. Kilgard, 1998. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* When you quite a GLUT program, the atexit callbacks should
   always be called.  In GLUT 3.6 and earlier, this was not happening.
   It should be fixed in GLUT 3.7. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

void
exitHappened(void)
{
  printf("PASS: atexit test\n");
}

int
main(int argc, char **argv)
{
  atexit(exitHappened);
  glutInitWindowSize(400, 100);
  glutInit(&argc, argv);
  glutCreateWindow("at exit test (quit via window border)");
  glutDisplayFunc(display);
  printf("\nIf you quite via the window manager you should see\n"
    "a message with the word pass capitalized.\n\n");
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
