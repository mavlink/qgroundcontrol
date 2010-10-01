
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

char *fake_argv[] =
{
  "program",
#if 1
  "-iconic",
#endif
  NULL};

int fake_argc = sizeof(fake_argv) / sizeof(char *) - 1;
int displayed = 0;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glutSwapBuffers();
  displayed = 1;
}

void
timer(int value)
{
  if (displayed) {
    printf("FAIL: test28\n");
    exit(1);
  }
  printf("PASS: test28\n");
  exit(0);
}

int
main(int argc, char **argv)
{
  glutInit(&fake_argc, fake_argv);
  if (fake_argc != 1) {
    printf("FAIL: argument processing\n");
    exit(1);
  }
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow("test28");
  glutDisplayFunc(display);
  glutTimerFunc(2000, timer, 0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
