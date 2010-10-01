
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

int win, subwin;
int mainmenu, submenu;
int item = 666;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

/* ARGSUSED1 */
void
gokey(unsigned char key, int x, int y)
{
  char str[100];
  int mods;

  mods = glutGetModifiers();
  printf("key = %d, mods = 0x%x\n", key, mods);
  if (mods & GLUT_ACTIVE_ALT) {
    switch (key) {
    case '1':
      printf("Change to sub menu 1\n");
      glutChangeToSubMenu(1, "sub 1", submenu);
      break;
    case '2':
      printf("Change to sub menu 2\n");
      glutChangeToSubMenu(2, "sub 2", submenu);
      break;
    case '3':
      printf("Change to sub menu 3\n");
      glutChangeToSubMenu(3, "sub 3", submenu);
      break;
    case '4':
      printf("Change to sub menu 4\n");
      glutChangeToSubMenu(4, "sub 4", submenu);
      break;
    case '5':
      printf("Change to sub menu 5\n");
      glutChangeToSubMenu(5, "sub 5", submenu);
      break;
    }
  } else {
    switch (key) {
    case '1':
      printf("Change to menu entry 1\n");
      glutChangeToMenuEntry(1, "entry 1", 1);
      break;
    case '2':
      printf("Change to menu entry 2\n");
      glutChangeToMenuEntry(2, "entry 2", 2);
      break;
    case '3':
      printf("Change to menu entry 3\n");
      glutChangeToMenuEntry(3, "entry 3", 3);
      break;
    case '4':
      printf("Change to menu entry 4\n");
      glutChangeToMenuEntry(4, "entry 4", 4);
      break;
    case '5':
      printf("Change to menu entry 5\n");
      glutChangeToMenuEntry(5, "entry 5", 5);
      break;
    case 'a':
    case 'A':
      printf("Adding menu entry %d\n", item);
      sprintf(str, "added entry %d", item);
      glutAddMenuEntry(str, item);
      item++;
      break;
    case 's':
    case 'S':
      printf("Adding submenu %d\n", item);
      sprintf(str, "added submenu %d", item);
      glutAddSubMenu(str, submenu);
      item++;
      break;
    case 'q':
      printf("Remove 1\n");
      glutRemoveMenuItem(1);
      break;
    case 'w':
      printf("Remove 2\n");
      glutRemoveMenuItem(2);
      break;
    case 'e':
      printf("Remove 3\n");
      glutRemoveMenuItem(3);
      break;
    case 'r':
      printf("Remove 4\n");
      glutRemoveMenuItem(4);
      break;
    case 't':
      printf("Remove 5\n");
      glutRemoveMenuItem(5);
      break;
    }
  }
}

void
keyboard(unsigned char key, int x, int y)
{
  glutSetMenu(mainmenu);
  gokey(key, x, y);
}

void
keyboard2(unsigned char key, int x, int y)
{
  glutSetMenu(submenu);
  gokey(key, x, y);
}

void
menu(int value)
{
  printf("menu: entry = %d\n", value);
}

void
menu2(int value)
{
  printf("menu2: entry = %d\n", value);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  win = glutCreateWindow("menu test");
  glClearColor(0.3, 0.3, 0.3, 0.0);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  submenu = glutCreateMenu(menu2);
  glutAddMenuEntry("Sub menu 1", 1001);
  glutAddMenuEntry("Sub menu 2", 1002);
  glutAddMenuEntry("Sub menu 3", 1003);
  mainmenu = glutCreateMenu(menu);
  glutAddMenuEntry("First", -1);
  glutAddMenuEntry("Second", -2);
  glutAddMenuEntry("Third", -3);
  glutAddSubMenu("Submenu init", submenu);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  subwin = glutCreateSubWindow(win, 50, 50, 50, 50);
  glClearColor(0.7, 0.7, 0.7, 0.0);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard2);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
