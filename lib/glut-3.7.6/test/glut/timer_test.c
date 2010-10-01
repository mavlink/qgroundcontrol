
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* timer_test is supposed to demonstrate that window system
   event related callbacks (like the keyboard callback) do not
   "starve out" the dispatching of timer callbacks.  Run this
   program and hold down the space bar.  The correct behavior
   (assuming the system does autorepeat) is interleaved "key is 
   32" and "timer called" messages.  If you don't see "timer
   called" messages, that's a problem.  This problem exists in
   GLUT implementations through GLUT 3.2. */

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(1000 * x)
#else
#include <unistd.h>
#endif

#include <GL/glut.h>

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

/* ARGSUSED */
void
timer(int value)
{
  printf("timer called\n");
  glutTimerFunc(500, timer, 0);
}

/* ARGSUSED1 */
void
keyboard(unsigned char k, int x, int y)
{
  static int beenhere = 0;

  if (!beenhere) {
    glutTimerFunc(500, timer, 0);
    beenhere = 1;
  }
  printf("key is %d\n", k);
  sleep(1);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutCreateWindow("timer test");
  glClearColor(0.49, 0.62, 0.75, 0.0);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
