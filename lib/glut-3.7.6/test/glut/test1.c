
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
  "-display",
  ":0",
  "-geometry",
  "500x400-34-23",
  "-indirect",
  "-iconic",
  NULL};

int fake_argc = sizeof(fake_argv) / sizeof(char *) - 1;

int
main(int argc, char **argv)
{
  char *altdisplay;
  int screen_width, screen_height;
  int got;

  altdisplay = getenv("GLUT_TEST_ALT_DISPLAY");
  if (altdisplay) {
    fake_argv[2] = altdisplay;
  }
  glutInit(&fake_argc, fake_argv);
  if (fake_argc != 1) {
    printf("FAIL: argument processing\n");
    exit(1);
  }
  if ((got = glutGet(GLUT_INIT_WINDOW_WIDTH)) != 500) {
    printf("FAIL: width wrong, got %d, not 500\n", got);
    exit(1);
  }
  if ((got = glutGet(GLUT_INIT_WINDOW_HEIGHT)) != 400) {
    printf("FAIL: width height, got %d, not 400\n", got);
    exit(1);
  }
  screen_width = glutGet(GLUT_SCREEN_WIDTH);
  screen_height = glutGet(GLUT_SCREEN_HEIGHT);
  if ((got = glutGet(GLUT_INIT_WINDOW_X)) != (screen_width - 500 - 34)) {
    printf("FAIL: width x, got %d, not %d\n", got, screen_width - 500 - 34);
    exit(1);
  }
  if ((got = glutGet(GLUT_INIT_WINDOW_Y)) != (screen_height - 400 - 23)) {
    printf("FAIL: width y, got %d, not %d\n", got, screen_height - 400 - 23);
    exit(1);
  }
  if (glutGet(GLUT_INIT_DISPLAY_MODE) !=
    (GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH)) {
    printf("FAIL: width wrong\n");
    exit(1);
  }
  glutInitWindowPosition(10, 10);
  glutInitWindowSize(200, 200);
  glutInitDisplayMode(
    GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  if (glutGet(GLUT_INIT_WINDOW_WIDTH) != 200) {
    printf("FAIL: width wrong\n");
    exit(1);
  }
  if (glutGet(GLUT_INIT_WINDOW_HEIGHT) != 200) {
    printf("FAIL: width wrong\n");
    exit(1);
  }
  if (glutGet(GLUT_INIT_WINDOW_X) != 10) {
    printf("FAIL: width wrong\n");
    exit(1);
  }
  if (glutGet(GLUT_INIT_WINDOW_Y) != 10) {
    printf("FAIL: width wrong\n");
    exit(1);
  }
  if (glutGet(GLUT_INIT_DISPLAY_MODE) !=
    (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL)) {
    printf("FAIL: width wrong\n");
    exit(1);
  }
  printf("PASS: test1\n");
  return 0;             /* ANSI C requires main to return int. */
}
