
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

int w1, w2;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
}

void
time9(int value)
{
  if (value != 9) {
    printf("FAIL: time9 expected 9\n");
    exit(1);
  }
  printf("PASS: test7\n");
  exit(0);
}

void
time8(int value)
{
  if (value != 8) {
    printf("FAIL: time8 expected 8\n");
    exit(1);
  }
  printf("window 1 to 350x250+20+200; window 2 to 50x150+50+50\n");
  glutSetWindow(w1);
  glutReshapeWindow(350, 250);
  glutPositionWindow(20, 200);
  glutSetWindow(w2);
  glutReshapeWindow(50, 150);
  glutPositionWindow(50, 50);
  glutTimerFunc(1000, time9, 9);
}

void
time7(int value)
{
  if (value != 7) {
    printf("FAIL: time7 expected 7\n");
    exit(1);
  }
  printf("window 1 fullscreen; window 2 popped on top\n");
  glutSetWindow(w1);
  glutShowWindow();
  glutFullScreen();
  glutSetWindow(w2);
  glutShowWindow();
  glutPopWindow();
  /* It can take a long time for glutFullScreen to really happen
     on a Windows 95 PC.  I believe this has to do with the memory
     overhead for resizing a huge soft color and/or ancillary buffers. */
  glutTimerFunc(6000, time8, 8);
}

void
time6(int value)
{
  if (value != 6) {
    printf("FAIL: time6 expected 6\n");
    exit(1);
  }
  printf("change icon tile for both windows\n");
  glutSetWindow(w1);
  glutSetIconTitle("icon1");
  glutSetWindow(w2);
  glutSetIconTitle("icon2");
  glutTimerFunc(1000, time7, 7);
}

void
time5(int value)
{
  if (value != 5) {
    printf("FAIL: time5 expected 5\n");
    exit(1);
  }
  glutSetWindow(w1);
  if (glutGet(GLUT_WINDOW_X) != 20) {
    printf("WARNING: x position expected to be 20\n");
  }
  if (glutGet(GLUT_WINDOW_Y) != 20) {
    printf("WARNING: y position expected to be 20\n");
  }
  if (glutGet(GLUT_WINDOW_WIDTH) != 250) {
    printf("WARNING: width expected to be 250\n");
  }
  if (glutGet(GLUT_WINDOW_HEIGHT) != 250) {
    printf("WARNING: height expected to be 250\n");
  }
  glutSetWindow(w2);
  if (glutGet(GLUT_WINDOW_X) != 250) {
    printf("WARNING: x position expected to be 250\n");
  }
  if (glutGet(GLUT_WINDOW_Y) != 250) {
    printf("WARNING: y position expected to be 250\n");
  }
  if (glutGet(GLUT_WINDOW_WIDTH) != 150) {
    printf("WARNING: width expected to be 150\n");
  }
  if (glutGet(GLUT_WINDOW_HEIGHT) != 150) {
    printf("WARNING: height expected to be 150\n");
  }
  printf("iconify both windows\n");
  glutSetWindow(w1);
  glutIconifyWindow();
  glutSetWindow(w2);
  glutIconifyWindow();
  glutTimerFunc(1000, time6, 6);
}

void
time4(int value)
{
  if (value != 4) {
    printf("FAIL: time4 expected 4\n");
    exit(1);
  }
  printf("reshape and reposition window\n");
  glutSetWindow(w1);
  glutReshapeWindow(250, 250);
  glutPositionWindow(20, 20);
  glutSetWindow(w2);
  glutReshapeWindow(150, 150);
  glutPositionWindow(250, 250);
  glutTimerFunc(1000, time5, 5);
}

void
time3(int value)
{
  if (value != 3) {
    printf("FAIL: time3 expected 3\n");
    exit(1);
  }
  printf("show both windows again\n");
  glutSetWindow(w1);
  glutShowWindow();
  glutSetWindow(w2);
  glutShowWindow();
  glutTimerFunc(1000, time4, 4);
}

void
time2(int value)
{
  if (value != 2) {
    printf("FAIL: time2 expected 2\n");
    exit(1);
  }
  printf("hiding w1; iconify w2\n");
  glutSetWindow(w1);
  glutHideWindow();
  glutSetWindow(w2);
  glutIconifyWindow();
  glutTimerFunc(1000, time3, 3);
}

void
time1(int value)
{
  if (value != 1) {
    printf("FAIL: time1 expected 1\n");
    exit(1);
  }
  printf("changing window titles\n");
  glutSetWindow(w1);
  glutSetWindowTitle("changed title");
  glutSetWindow(w2);
  glutSetWindowTitle("changed other title");
  glutTimerFunc(2000, time2, 2);
}

int
main(int argc, char **argv)
{
  glutInitWindowPosition(20, 20);
  glutInit(&argc, argv);
  w1 = glutCreateWindow("test 1");
  glutDisplayFunc(display);
  glutInitWindowPosition(200, 200);
  w2 = glutCreateWindow("test 2");
  glutDisplayFunc(display);
  glutTimerFunc(1000, time1, 1);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
