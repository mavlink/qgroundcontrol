
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Try testing menu item removal and menu destruction. */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

void
displayFunc(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

void
menuFunc(int choice)
{
  printf("choice = %d\n", choice);
}

void
timefunc(int value)
{
  if (value != 1) {
    printf("FAIL: test14\n");
    exit(1);
  }
  printf("PASS: test14\n");
  exit(0);
}

int
main(int argc, char **argv)
{
  int i, menu, submenu;

  glutInit(&argc, argv);
  glutCreateWindow("test14");
  glutDisplayFunc(displayFunc);

  submenu = glutCreateMenu(menuFunc);
  glutAddMenuEntry("First", 10101);
  glutAddMenuEntry("Second", 20202);

  menu = glutCreateMenu(menuFunc);
  glutAddMenuEntry("Entry1", 101);
  glutAddMenuEntry("Entry2----------", 102);
  glutRemoveMenuItem(2);
  glutRemoveMenuItem(1);
  glutAddMenuEntry("oEntry1", 201);
  glutAddMenuEntry("o----------", 200);
  glutAddMenuEntry("oEntry2----------", 202);
  glutAddMenuEntry("oEntry3", 203);
  glutRemoveMenuItem(2);
  glutDestroyMenu(menu);

  menu = glutCreateMenu(menuFunc);
  glutAddMenuEntry("Entry1", 101);
  glutAddMenuEntry("Entry2----------", 102);
  glutRemoveMenuItem(2);
  glutRemoveMenuItem(1);
  glutAddMenuEntry("oEntry1", 201);
  glutAddMenuEntry("o----------", 200);
  glutAddMenuEntry("oEntry2----------", 202);
  glutAddMenuEntry("oEntry3", 203);
  glutRemoveMenuItem(2);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  menu = glutCreateMenu(menuFunc);
  for (i = 0; i < 10; i++) {
    glutAddMenuEntry("YES", i);
  }
  for (i = 0; i < 10; i++) {
    glutRemoveMenuItem(1);
  }
  glutAddMenuEntry("Entry1", 101);
  glutAddMenuEntry("Entry2", 102);
  glutAddMenuEntry("Entry3", 103);
  glutRemoveMenuItem(2);
  glutRemoveMenuItem(1);
  glutAddMenuEntry("----------", 303);
  for (i = 0; i < 10; i++) {
    glutAddMenuEntry("YES**************************", i);
  }
  for (i = 0; i < 9; i++) {
    glutRemoveMenuItem(3);
  }
  glutDestroyMenu(menu);

  menu = glutCreateMenu(menuFunc);
  for (i = 0; i < 10; i++) {
    glutAddMenuEntry("YES", i);
  }
  for (i = 0; i < 10; i++) {
    glutRemoveMenuItem(1);
  }
  glutAddMenuEntry("Entry1", 101);
  glutAddMenuEntry("Entry2", 102);
  glutAddMenuEntry("Entry3", 103);
  glutRemoveMenuItem(2);
  glutRemoveMenuItem(1);
  glutAddMenuEntry("----------", 303);
  for (i = 0; i < 10; i++) {
    glutAddMenuEntry("YES**************************", i);
  }
  for (i = 0; i < 9; i++) {
    glutRemoveMenuItem(3);
  }
  glutAttachMenu(GLUT_MIDDLE_BUTTON);

  menu = glutCreateMenu(menuFunc);
  glutAddMenuEntry("Entry1", 101);
  glutAddMenuEntry("Entry2", 102);
  glutAddMenuEntry("Entry3", 103);
  glutRemoveMenuItem(2);
  glutRemoveMenuItem(1);
  glutAddMenuEntry("nEntry1", 201);
  glutAddMenuEntry("nEntry2----------", 202);
  glutAddMenuEntry("nEntry3", 203);
  glutRemoveMenuItem(2);
  glutRemoveMenuItem(1);
  glutAddMenuEntry("n----------", 303);
  glutChangeToMenuEntry(1, "HELLO", 34);
  glutChangeToSubMenu(2, "HELLO menu", submenu);
  glutDestroyMenu(menu);

  menu = glutCreateMenu(menuFunc);
  glutAddMenuEntry("Entry1", 101);
  glutAddMenuEntry("Entry2", 102);
  glutAddMenuEntry("Entry3", 103);
  glutRemoveMenuItem(2);
  glutRemoveMenuItem(1);
  glutAddMenuEntry("nEntry1", 201);
  glutAddMenuEntry("nEntry2----------", 202);
  glutAddMenuEntry("nEntry3", 203);
  glutRemoveMenuItem(2);
  glutRemoveMenuItem(1);
  glutAddMenuEntry("n----------", 303);
  glutAttachMenu(GLUT_LEFT_BUTTON);

  glutTimerFunc(2000, timefunc, 1);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
