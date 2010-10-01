
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(1000 * x)
#else
#include <unistd.h>
#endif
#include <GL/glut.h>

int
main(int argc, char **argv)
{
  int a, b, d;
  int val;

  glutInit(&argc, argv);
  a = glutGet(GLUT_ELAPSED_TIME);
  sleep(1);
  b = glutGet(GLUT_ELAPSED_TIME);
  d = b - a;
  if (d < 990 || d > 1200) {
    printf("FAIL: test12\n");
    exit(1);
  }
  glutCreateWindow("dummy");
  /* try all GLUT_WINDOW_* glutGet's */
  val = glutGet(GLUT_WINDOW_X);
  val = glutGet(GLUT_WINDOW_Y);
  val = glutGet(GLUT_WINDOW_WIDTH);
  if (val != 300) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_HEIGHT);
  if (val != 300) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_BUFFER_SIZE);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_STENCIL_SIZE);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_DEPTH_SIZE);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_RED_SIZE);
  if (val < 1) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_GREEN_SIZE);
  if (val < 1) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_BLUE_SIZE);
  if (val < 1) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_ALPHA_SIZE);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_ACCUM_RED_SIZE);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_ACCUM_GREEN_SIZE);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_ACCUM_BLUE_SIZE);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_ACCUM_ALPHA_SIZE);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_DOUBLEBUFFER);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_RGBA);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_CURSOR);
  if (val != GLUT_CURSOR_INHERIT) {
    printf("FAIL: test12\n");
    exit(1);
  }
  printf("Window format id = 0x%x (%d)\n",
    glutGet(GLUT_WINDOW_FORMAT_ID), glutGet(GLUT_WINDOW_FORMAT_ID));
  glutSetCursor(GLUT_CURSOR_NONE);
  val = glutGet(GLUT_WINDOW_CURSOR);
  if (val != GLUT_CURSOR_NONE) {
    printf("FAIL: test12\n");
    exit(1);
  }
  glutWarpPointer(0, 0);
  glutWarpPointer(-5, -5);
  glutWarpPointer(2000, 2000);
  glutWarpPointer(-4000, 4000);
  val = glutGet(GLUT_WINDOW_COLORMAP_SIZE);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_PARENT);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_NUM_SAMPLES);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_WINDOW_STEREO);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  /* touch GLUT_SCREEN_* glutGet's supported */
  val = glutGet(GLUT_SCREEN_WIDTH);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_SCREEN_HEIGHT);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_SCREEN_WIDTH_MM);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  val = glutGet(GLUT_SCREEN_HEIGHT_MM);
  if (val < 0) {
    printf("FAIL: test12\n");
    exit(1);
  }
  printf("PASS: test12\n");
  return 0;             /* ANSI C requires main to return int. */
}
