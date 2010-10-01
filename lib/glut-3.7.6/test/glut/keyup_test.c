
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <GL/glut.h>

void
key(unsigned char key, int x, int y)
{
  printf("kDN: %c <%d> @ (%d,%d)\n", key, key, x, y);
}

void
keyup(unsigned char key, int x, int y)
{
  printf("kUP: %c <%d> @ (%d,%d)\n", key, key, x, y);
}

void
special(int key, int x, int y)
{
  printf("sDN: %d @ (%d,%d)\n", key, x, y);
}

void
specialup(int key, int x, int y)
{
  printf("sUP: %d @ (%d,%d)\n", key, x, y);
}

void
menu(int value)
{
  switch(value) {
  case 1:
    glutIgnoreKeyRepeat(1);
    break;
  case 2:
    glutIgnoreKeyRepeat(0);
    break;
  case 3:
    glutKeyboardFunc(NULL);
    break;
  case 4:
    glutKeyboardFunc(key);
    break;
  case 5:
    glutKeyboardUpFunc(NULL);
    break;
  case 6:
    glutKeyboardUpFunc(keyup);
    break;
  case 7:
    glutSpecialFunc(NULL);
    break;
  case 8:
    glutSpecialFunc(special);
    break;
  case 9:
    glutSpecialUpFunc(NULL);
    break;
  case 10:
    glutSpecialUpFunc(specialup);
    break;
  }
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutCreateWindow("keyup test");
  glClearColor(0.49, 0.62, 0.75, 0.0);
  glutDisplayFunc(display);
  glutKeyboardFunc(key);
  glutKeyboardUpFunc(keyup);
  glutSpecialFunc(special);
  glutSpecialUpFunc(specialup);
  glutCreateMenu(menu);
  glutAddMenuEntry("Ignore autorepeat", 1);
  glutAddMenuEntry("Accept autorepeat", 2);
  glutAddMenuEntry("Stop key", 3);
  glutAddMenuEntry("Start key", 4);
  glutAddMenuEntry("Stop key up", 5);
  glutAddMenuEntry("Start key up", 6);
  glutAddMenuEntry("Stop special", 7);
  glutAddMenuEntry("Start special", 8);
  glutAddMenuEntry("Stop special up", 9);
  glutAddMenuEntry("Start special up", 10);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
