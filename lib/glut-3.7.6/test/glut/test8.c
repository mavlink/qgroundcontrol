
/* Copyright (c) Mark J. Kilgard, 1994, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

int main_w, w[4], win;
int num;

/* ARGSUSED */
void
time2(int value)
{
  printf("PASS: test8\n");
  exit(0);
}

/* ARGSUSED */
void
time1(int value)
{
  glutSetWindow(w[1]);
  glutIdleFunc(NULL);
  glutKeyboardFunc(NULL);
  glutSetWindow(w[0]);
  glutIdleFunc(NULL);   /* redundant */
  glutKeyboardFunc(NULL);
  glutSetWindow(main_w);
  glutIdleFunc(NULL);   /* redundant */
  glutKeyboardFunc(NULL);
  glutDestroyWindow(w[1]);
  glutDestroyWindow(w[0]);
  glutDestroyWindow(main_w);
  glutTimerFunc(500, time2, 0);
}

void
display(void)
{
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  if (glutGet(GLUT_INIT_WINDOW_WIDTH) != 300) {
    printf("FAIL: init width wrong\n");
    exit(1);
  }
  if (glutGet(GLUT_INIT_WINDOW_HEIGHT) != 300) {
    printf("FAIL: init height wrong\n");
    exit(1);
  }
  if (glutGet(GLUT_INIT_WINDOW_X) != -1) {
    printf("FAIL: init x wrong\n");
    exit(1);
  }
  if (glutGet(GLUT_INIT_WINDOW_Y) != -1) {
    printf("FAIL: init y wrong\n");
    exit(1);
  }
  if (glutGet(GLUT_INIT_DISPLAY_MODE) != (GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH)) {
    printf("FAIL: init display mode wrong\n");
    exit(1);
  }
  glutInitDisplayMode(GLUT_RGB);
  main_w = glutCreateWindow("main");
  glutDisplayFunc(display);
  num = glutGet(GLUT_DISPLAY_MODE_POSSIBLE);
  if (num != 1) {
    printf("FAIL: glutGet returned display mode not possible: %d\n", num);
    exit(1);
  }
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (0 != num) {
    printf("FAIL: glutGet returned wrong # children: %d\n", num);
    exit(1);
  }
  w[0] = glutCreateSubWindow(main_w, 10, 10, 20, 20);
  glutDisplayFunc(display);
  num = glutGet(GLUT_WINDOW_PARENT);
  if (main_w != num) {
    printf("FAIL: glutGet returned bad parent: %d\n", num);
    exit(1);
  }
  glutSetWindow(main_w);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (1 != num) {
    printf("FAIL: glutGet returned wrong # children: %d\n", num);
    exit(1);
  }
  w[1] = glutCreateSubWindow(main_w, 40, 10, 20, 20);
  glutDisplayFunc(display);
  num = glutGet(GLUT_WINDOW_PARENT);
  if (main_w != num) {
    printf("FAIL: glutGet returned bad parent: %d\n", num);
    exit(1);
  }
  glutSetWindow(main_w);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (2 != num) {
    printf("FAIL: glutGet returned wrong # children: %d\n", num);
    exit(1);
  }
  w[2] = glutCreateSubWindow(main_w, 10, 40, 20, 20);
  glutDisplayFunc(display);
  num = glutGet(GLUT_WINDOW_PARENT);
  if (main_w != num) {
    printf("FAIL: glutGet returned bad parent: %d\n", num);
    exit(1);
  }
  glutSetWindow(main_w);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (3 != num) {
    printf("FAIL: glutGet returned wrong # children: %d\n", num);
    exit(1);
  }
  w[3] = glutCreateSubWindow(main_w, 40, 40, 20, 20);
  glutDisplayFunc(display);
  num = glutGet(GLUT_WINDOW_PARENT);
  if (main_w != num) {
    printf("FAIL: glutGet returned bad parent: %d\n", num);
    exit(1);
  }
  glutSetWindow(main_w);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (4 != num) {
    printf("FAIL: glutGet returned wrong # children: %d\n", num);
    exit(1);
  }
  glutDestroyWindow(w[3]);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (3 != num) {
    printf("FAIL: glutGet returned wrong # children: %d\n", num);
    exit(1);
  }
  w[3] = glutCreateSubWindow(main_w, 40, 40, 20, 20);
  glutDisplayFunc(display);
  glutCreateSubWindow(w[3], 40, 40, 20, 20);
  glutDisplayFunc(display);
  glutCreateSubWindow(w[3], 40, 40, 20, 20);
  glutDisplayFunc(display);
  win = glutCreateSubWindow(w[3], 40, 40, 20, 20);
  glutDisplayFunc(display);
  glutCreateSubWindow(win, 40, 40, 20, 20);
  glutDisplayFunc(display);
  win = glutCreateSubWindow(w[3], 40, 40, 20, 20);
  glutDisplayFunc(display);
  glutCreateSubWindow(win, 40, 40, 20, 20);
  glutDisplayFunc(display);
  glutDestroyWindow(w[3]);

  w[3] = glutCreateSubWindow(main_w, 40, 40, 20, 20);
  glutDisplayFunc(display);

  glutTimerFunc(500, time1, 0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
