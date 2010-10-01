
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This tests GLUT's video resize API (currently only supported
   on SGI's InfiniteReality hardware). */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

GLfloat light_diffuse[] =
{1.0, 0.0, 0.0, 1.0};
GLfloat light_position[] =
{1.0, 1.0, 1.0, 0.0};

int x, y, w, h, dx, dy, dw, dh;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glutSolidTeapot(1.0);
  glutSwapBuffers();
}

void
show_video_size(void)
{
  printf("GLUT_VIDEO_RESIZE_X = %d\n",
    glutVideoResizeGet(GLUT_VIDEO_RESIZE_X));
  printf("GLUT_VIDEO_RESIZE_Y = %d\n",
    glutVideoResizeGet(GLUT_VIDEO_RESIZE_Y));
  printf("GLUT_VIDEO_RESIZE_WIDTH = %d\n",
    glutVideoResizeGet(GLUT_VIDEO_RESIZE_WIDTH));
  printf("GLUT_VIDEO_RESIZE_HEIGHT = %d\n",
    glutVideoResizeGet(GLUT_VIDEO_RESIZE_HEIGHT));
}

/* ARGSUSED1 */
void
key(unsigned char k, int x, int y)
{
  printf("c = %c\n", k);
  switch (k) {
  case 27:
    exit(0);
    return;
  case 'a':
    glutVideoPan(0, 0, 1280, 1024);
    break;
  case 'b':
    glutVideoPan(0, 0, 1600, 1024);
    break;
  case 'c':
    glutVideoPan(640, 512, 640, 512);
    break;
  case 'q':
    glutVideoPan(320, 256, 640, 512);
    break;
  case '1':
    glutVideoResize(0, 0, 640, 512);
    break;
  case '2':
    glutVideoResize(0, 512, 640, 512);
    break;
  case '3':
    glutVideoResize(512, 512, 640, 512);
    break;
  case '4':
    glutVideoResize(512, 0, 640, 512);
    break;
  case 's':
    glutStopVideoResizing();
    break;
  case '=':
    show_video_size();
    break;
  case ' ':
    glutPostRedisplay();
    break;
  }
}

/* ARGSUSED */
void
time2(int value)
{
  glutVideoResize(x, y, w, h);
  glutPostRedisplay();
  x -= dx;
  y -= dy;
  w += (dx * 2);
  h += (dy * 2);
  if (x > 0) {
    glutTimerFunc(100, time2, 0);
  } else {
    glutStopVideoResizing();
    printf("PASS: test21 (with video resizing tested)\n");
    exit(0);
  }
}

/* ARGSUSED */
void
time1(int value)
{
  glutVideoPan(x, y, w, h);
  x += dx;
  y += dy;
  w -= (dx * 2);
  h -= (dy * 2);
  if (x < 200) {
    glutTimerFunc(100, time1, 0);
  } else {
    glutTimerFunc(100, time2, 0);
  }
}

int
main(int argc, char **argv)
{
  int i, interact = 0;

  glutInit(&argc, argv);

  for (i = 1; i < argc; i++) {
    if (!strcmp("-i", argv[i])) {
      interact = 1;
    }
  }

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutCreateWindow("test21");

  if (!glutVideoResizeGet(GLUT_VIDEO_RESIZE_POSSIBLE)) {
    printf("video resizing not supported\n");
    printf("PASS: test21\n");
    exit(0);
  }
  glutSetupVideoResizing();
  printf("GLUT_VIDEO_RESIZE_X_DELTA = %d\n",
    dx = glutVideoResizeGet(GLUT_VIDEO_RESIZE_X_DELTA));
  printf("GLUT_VIDEO_RESIZE_Y_DELTA = %d\n",
    dy = glutVideoResizeGet(GLUT_VIDEO_RESIZE_Y_DELTA));
  printf("GLUT_VIDEO_RESIZE_WIDTH_DELTA = %d\n",
    dw = glutVideoResizeGet(GLUT_VIDEO_RESIZE_WIDTH_DELTA));
  printf("GLUT_VIDEO_RESIZE_HEIGHT_DELTA = %d\n",
    dh = glutVideoResizeGet(GLUT_VIDEO_RESIZE_HEIGHT_DELTA));
  printf("GLUT_VIDEO_RESIZE_X = %d\n",
    x = glutVideoResizeGet(GLUT_VIDEO_RESIZE_X));
  printf("GLUT_VIDEO_RESIZE_Y = %d\n",
    y = glutVideoResizeGet(GLUT_VIDEO_RESIZE_Y));
  printf("GLUT_VIDEO_RESIZE_WIDTH = %d\n",
    w = glutVideoResizeGet(GLUT_VIDEO_RESIZE_WIDTH));
  printf("GLUT_VIDEO_RESIZE_HEIGHT = %d\n",
    h = glutVideoResizeGet(GLUT_VIDEO_RESIZE_HEIGHT));
  glutStopVideoResizing();
  glutSetupVideoResizing();

  glutDisplayFunc(display);
  glutFullScreen();

  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 22.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in postivie Y direction */
  glTranslatef(0.0, 0.0, -1.0);

  glutKeyboardFunc(key);
  if (!interact) {
    glutTimerFunc(100, time1, 0);
  }
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
