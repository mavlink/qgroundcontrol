
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  printf("Keyboard:  %s\n", glutDeviceGet(GLUT_HAS_KEYBOARD) ? "YES" : "no");
  printf("Mouse:     %s\n", glutDeviceGet(GLUT_HAS_MOUSE) ? "YES" : "no");
  printf("Spaceball: %s\n", glutDeviceGet(GLUT_HAS_SPACEBALL) ? "YES" : "no");
  printf("Dials:     %s\n", glutDeviceGet(GLUT_HAS_DIAL_AND_BUTTON_BOX) ? "YES" : "no");
  printf("Tablet:    %s\n\n", glutDeviceGet(GLUT_HAS_TABLET) ? "YES" : "no");
  printf("Mouse buttons:      %d\n", glutDeviceGet(GLUT_NUM_MOUSE_BUTTONS));
  printf("Spaceball buttons:  %d\n", glutDeviceGet(GLUT_NUM_SPACEBALL_BUTTONS));
  printf("Button box buttons: %d\n", glutDeviceGet(GLUT_NUM_BUTTON_BOX_BUTTONS));
  printf("Dials:              %d\n", glutDeviceGet(GLUT_NUM_DIALS));
  printf("Tablet buttons:     %d\n\n", glutDeviceGet(GLUT_NUM_TABLET_BUTTONS));
  printf("PASS: test11\n");
  return 0;             /* ANSI C requires main to return int. */
}
