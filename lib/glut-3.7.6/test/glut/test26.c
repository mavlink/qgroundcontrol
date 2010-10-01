
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Test for glutPostWindowRedisplay and
   glutPostWindowOverlayRedisplay introduced with GLUT 4 API. */

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>

int window1, window2, win1displayed = 0, win2displayed = 0;
int win1vis = 0, win2vis = 0;

int overlaySupported, transP, opaqueP, over1displayed = 0;

void
checkifdone(void)
{
  if ((win1displayed > 15) && (win2displayed > 15) && (!overlaySupported || over1displayed>15)) {
    printf("PASS: test26\n");
    exit(0);
  }
}

void
window1display(void)
{
  if (glutGetWindow() != window1) {
    printf("FAIL: window1display\n");
    exit(1);
  }
  glClearColor(0, 1, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
  win1displayed++;
  checkifdone();
}

void
overDisplay(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glRectf(-0.5, -0.5, 0.5, 0.5);
  glFlush();
  over1displayed++;
  checkifdone();
}

void
window2display(void)
{
  if (glutGetWindow() != window2) {
    printf("FAIL: window2display\n");
    exit(1);
  }
  glClearColor(0, 0, 1, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glutSwapBuffers();
  win2displayed++;
  checkifdone();
}

/* ARGSUSED */
void
timefunc(int value)
{
  printf("FAIL: test26\n");
  exit(1);
}

void
idle(void)
{
  static int count = 0;

  if (count % 2) {
    glutPostWindowRedisplay(window1);
    glutPostWindowRedisplay(window2);
  } else {
    glutPostWindowRedisplay(window2);
    glutPostWindowRedisplay(window1);
  }
  if (overlaySupported) {
    glutPostWindowOverlayRedisplay(window1);
  }
  count++;
}

void
window1vis(int vis)
{
  win1vis = vis;
  if (win1vis && win2vis) {
    glutIdleFunc(idle);
  }
}

void
window2status(int status)
{
  win2vis = (status == GLUT_FULLY_RETAINED) || (status == GLUT_PARTIALLY_RETAINED);
  if (win1vis && win2vis) {
    glutIdleFunc(idle);
  }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);

  glutInitWindowSize(100, 100);
  glutInitWindowPosition(50, 100);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  window1 = glutCreateWindow("1");
  glutDisplayFunc(window1display);
  glutVisibilityFunc(window1vis);

  glutInitDisplayMode(GLUT_SINGLE | GLUT_INDEX);
  overlaySupported = glutLayerGet(GLUT_OVERLAY_POSSIBLE);
  if (overlaySupported) {
    printf("testing glutPostWindowOverlayRedisplay since overlay supported\n");
    glutEstablishOverlay();
    glutOverlayDisplayFunc(overDisplay);
    transP = glutLayerGet(GLUT_TRANSPARENT_INDEX);
    glClearIndex(glutLayerGet(GLUT_TRANSPARENT_INDEX));
    opaqueP = (transP + 1) % glutGet(GLUT_WINDOW_COLORMAP_SIZE);
    glutSetColor(opaqueP, 1.0, 0.0, 0.0);
  }

  glutInitWindowPosition(250, 100);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  window2 = glutCreateWindow("2");
  glutDisplayFunc(window2display);
  glutWindowStatusFunc(window2status);

  glutTimerFunc(9000, timefunc, 1);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
