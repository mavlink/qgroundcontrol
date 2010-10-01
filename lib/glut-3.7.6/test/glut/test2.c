
/* Copyright (c) Mark J. Kilgard, 1994, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

int count = 0, save_count;
int head, tail, diff;

void
idle(void)
{
  count++;
}

void
timer2(int value)
{
  if (value != 36) {
    printf("FAIL: timer value wrong\n");
    exit(1);
  }
  if (count != save_count) {
    printf("FAIL: counter still counting\n");
    exit(1);
  }
  printf("PASS: test2\n");
  exit(0);
}

void
timer(int value)
{
  if (value != 42) {
    printf("FAIL: timer value wrong\n");
    exit(1);
  }
  if (count <= 0) {
    printf("FAIL: idle func not running\n");
    exit(1);
  }
  glutIdleFunc(NULL);
  save_count = count;
  tail = glutGet(GLUT_ELAPSED_TIME);
  diff = tail - head;
  printf("diff = %d (%d - %d)\n", diff, tail, head);
  if (diff > ((int) 500 * 1.2)) {
    printf("THIS TEST IS TIME SENSITIVE; IF IT FAILS, TRY RUNNING IT AGAIN.\n");
    printf("FAIL: timer too late\n");
    exit(1);
  }
  if (diff < ((int) 500 * .9)) {
    printf("THIS TEST IS TIME SENSITIVE; IF IT FAILS, TRY RUNNING IT AGAIN.\n");
    printf("FAIL: timer too soon\n");
    exit(1);
  }
  glutTimerFunc(100, timer2, 36);
}

/* ARGSUSED */
void
menuSelect(int value)
{
}

void
NeverVoid(void)
{
  printf("FAIL: NeverVoid should never be called\n");
  exit(1);
}

/* ARGSUSED */
void
NeverValue(int value)
{
  printf("FAIL: NeverValue most be NOT visible\n");
  exit(1);
}

#define NUM 15

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

int
main(int argc, char **argv)
{
  int win, menu;
  int marray[NUM];
  int warray[NUM];
  int i, j;
  GLint isIndex;

  glutInit(&argc, argv);
  glutInitWindowPosition(10, 10);
  glutInitWindowSize(200, 200);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
  win = glutCreateWindow("test2");
  glGetIntegerv(GL_INDEX_MODE, &isIndex);
  if (isIndex != 0) {
    printf("FAIL: window should be RGBA\n");
    exit(1);
  }
  glutSetWindow(win);
  glutDisplayFunc(display);
  menu = glutCreateMenu(menuSelect);
  glutSetMenu(menu);
  glutReshapeFunc(NULL);
  glutReshapeFunc(NULL);
  glutKeyboardFunc(NULL);
  glutKeyboardFunc(NULL);
  glutMouseFunc(NULL);
  glutMouseFunc(NULL);
  glutMotionFunc(NULL);
  glutMotionFunc(NULL);
  glutVisibilityFunc(NULL);
  glutVisibilityFunc(NULL);
  glutMenuStateFunc(NULL);
  glutMenuStateFunc(NULL);
  glutMenuStatusFunc(NULL);
  glutMenuStatusFunc(NULL);
  glutSpecialFunc(NULL);
  glutSpecialFunc(NULL);
  glutSpaceballMotionFunc(NULL);
  glutSpaceballMotionFunc(NULL);
  glutSpaceballRotateFunc(NULL);
  glutSpaceballRotateFunc(NULL);
  glutSpaceballButtonFunc(NULL);
  glutSpaceballButtonFunc(NULL);
  glutButtonBoxFunc(NULL);
  glutButtonBoxFunc(NULL);
  glutDialsFunc(NULL);
  glutDialsFunc(NULL);
  glutTabletMotionFunc(NULL);
  glutTabletMotionFunc(NULL);
  glutTabletButtonFunc(NULL);
  glutTabletButtonFunc(NULL);
  for (i = 0; i < NUM; i++) {
    marray[i] = glutCreateMenu(menuSelect);
    warray[i] = glutCreateWindow("test");
    glutDisplayFunc(display);
    for (j = 0; j < i; j++) {
      glutAddMenuEntry("Hello", 1);
      glutAddSubMenu("Submenu", menu);
    }
    if (marray[i] != glutGetMenu()) {
      printf("FAIL: current menu not %d\n", marray[i]);
      exit(1);
    }
    if (warray[i] != glutGetWindow()) {
      printf("FAIL: current window not %d\n", warray[i]);
      exit(1);
    }
    glutDisplayFunc(NeverVoid);
    glutVisibilityFunc(NeverValue);
    glutHideWindow();
  }
  for (i = 0; i < NUM; i++) {
    glutDestroyMenu(marray[i]);
    glutDestroyWindow(warray[i]);
  }
  glutTimerFunc(500, timer, 42);
  head = glutGet(GLUT_ELAPSED_TIME);
  glutIdleFunc(idle);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
