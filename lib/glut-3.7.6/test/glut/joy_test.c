
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

void
joystick(unsigned int buttonMask, int x, int y, int z)
{
  printf("joy 0x%x, x=%d y=%d z=%d\n", buttonMask, x, y, z);
}

void
joyPoll(void)
{
  printf("force\n");
  glutForceJoystickFunc();
}

void
menu(int value)
{
  switch(value) {
  case 1:
    glutJoystickFunc(joystick, 100);
    glutIdleFunc(NULL);
    break;
  case 2:
    glutJoystickFunc(NULL, 0);
    glutIdleFunc(NULL);
    break;
  case 3:
    glutJoystickFunc(joystick, 0);
    glutIdleFunc(joyPoll);
    break;
  }
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

void
keyboard(unsigned char c, int x, int y)
{
  if (c == 27) exit(0);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutCreateWindow("joystick test");
  glClearColor(0.29, 0.62, 1.0, 0.0);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutCreateMenu(menu);
  glutAddMenuEntry("Enable joystick callback", 1);
  glutAddMenuEntry("Disable joystick callback", 2);
  glutAddMenuEntry("Force joystick polling", 3);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
