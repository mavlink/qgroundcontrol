
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

int cursor[] =
{
  GLUT_CURSOR_INHERIT,
  GLUT_CURSOR_NONE,
  GLUT_CURSOR_FULL_CROSSHAIR,
  GLUT_CURSOR_RIGHT_ARROW,
  GLUT_CURSOR_LEFT_ARROW,
  GLUT_CURSOR_INFO,
  GLUT_CURSOR_DESTROY,
  GLUT_CURSOR_HELP,
  GLUT_CURSOR_CYCLE,
  GLUT_CURSOR_SPRAY,
  GLUT_CURSOR_WAIT,
  GLUT_CURSOR_TEXT,
  GLUT_CURSOR_CROSSHAIR,
  GLUT_CURSOR_UP_DOWN,
  GLUT_CURSOR_LEFT_RIGHT,
  GLUT_CURSOR_TOP_SIDE,
  GLUT_CURSOR_BOTTOM_SIDE,
  GLUT_CURSOR_LEFT_SIDE,
  GLUT_CURSOR_RIGHT_SIDE,
  GLUT_CURSOR_TOP_LEFT_CORNER,
  GLUT_CURSOR_TOP_RIGHT_CORNER,
  GLUT_CURSOR_BOTTOM_RIGHT_CORNER,
  GLUT_CURSOR_BOTTOM_LEFT_CORNER,
  0,
};

char *name[] =
{
  "INHERIT",
  "NONE",
  "FULL CROSSHAIR",
  "RIGHT ARROW",
  "LEFT ARROW",
  "INFO",
  "DESTROY",
  "HELP",
  "CYCLE",
  "SPRAY",
  "WAIT",
  "TEXT",
  "CROSSHAIR",
  "UP DOWN",
  "LEFT RIGHT",
  "TOP SIDE",
  "BOTTOM SIDE",
  "LEFT SIDE",
  "RIGHT SIDE",
  "TOP LEFT CORNER",
  "TOP RIGHT CORNER",
  "BOTTOM RIGHT CORNER",
  "BOTTOM LEFT CORNER",
  NULL,
};

int win;

void
futureSetCursor(int value)
{
  glutSetCursor(GLUT_CURSOR_HELP);
}

void
menu(int value)
{
  int cursor;

  if(value < 0) {
    switch(value) {
    case -1:
      glutSetWindow(win);
      glutWarpPointer(25, 25);
      return;
    case -2:
      glutSetWindow(win);
      glutWarpPointer(-25, -25);
      return;
    case -3:
      glutSetWindow(win);
      glutWarpPointer(250, 250);
      return;
    case -4:
      glutSetWindow(win);
      glutWarpPointer(2000, 200);
      return;
    case -5:
      glutTimerFunc(3000, futureSetCursor, glutGetWindow());
      return;
    }
  }
  glutSetCursor(value);
  cursor = glutGet(GLUT_WINDOW_CURSOR);
  if (cursor != value) {
    printf("cursor_test: cursor not set right\n");
    exit(1);
  }
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

void keyboard0( unsigned char key, int x, int y )
{
    printf( "main window: %d %c (%d, %d)\n", key, key, x, y );
}

void keyboardup( unsigned char key, int x, int y )
{
    printf( "main window (UP): %d %c (%d, %d)\n", key, key, x, y );
}

void keyboard1( unsigned char key, int x, int y )
{
    printf( "subwindow 1: %d %c (%d, %d)\n", key, key, x, y );
}

void keyboard2( unsigned char key, int x, int y )
{
    printf( "subwindow 2: %d %c (%d, %d)\n", key, key, x, y );
}

void keyboard3( unsigned char key, int x, int y )
{
    printf( "subwindow 3: %d %c (%d, %d)\n", key, key, x, y );
}

void motion( int x, int y )
{
	printf( "mouse motion: %d %d\n", x, y );
}

int
main(int argc, char **argv)
{
  int i;
  int sub1, sub2;

  glutInit(&argc, argv);
  win = glutCreateWindow("cursor test");
  glutSetCursor(GLUT_CURSOR_INFO);
  glClearColor(0.49, 0.62, 0.75, 0.0);
  glutDisplayFunc(display);
  glutCreateMenu(menu);
  for (i = 0; name[i] != NULL; i++) {
    glutAddMenuEntry(name[i], cursor[i]);
  }
  glutAddMenuEntry("Warp to (25,25)", -1);
  glutAddMenuEntry("Warp to (-25,-25)", -2);
  glutAddMenuEntry("Warp to (250,250)", -3);
  glutAddMenuEntry("Warp to (2000,200)", -4);
  glutAddMenuEntry("Set cursor in 3 secs", -5);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutKeyboardFunc( keyboard0 );
  glutKeyboardUpFunc( keyboardup );
  glutMotionFunc( motion );
  sub1 = glutCreateSubWindow(win, 10, 10, 90, 90);
  glutSetCursor(GLUT_CURSOR_CROSSHAIR);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutKeyboardFunc( keyboard1 );
  glClearColor(0.3, 0.82, 0.55, 0.0);
  glutDisplayFunc(display);
  sub2 = glutCreateSubWindow(win, 80, 80, 90, 90);
  glutSetCursor(GLUT_CURSOR_WAIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutKeyboardFunc( keyboard2 );
  glClearColor(0.9, 0.2, 0.2, 0.0);
  glutDisplayFunc(display);
  glutCreateSubWindow(sub2, 30, 30, 30, 30 );
  glutSetCursor(GLUT_CURSOR_TEXT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutKeyboardFunc( keyboard3 );
  glClearColor(0.9, 0.9, 0.2, 0.0);
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
