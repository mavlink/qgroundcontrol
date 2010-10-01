
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Test display callbacks are not called for non-viewable
   windows and overlays. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

int transP, opaqueP;
int main_win, sub_win;
int overlaySupported;
int subHidden, overlayHidden;
int warnIfNormalDisplay = 0;
int firstTime = 1;

void
time5_cb(int value)
{
  if (value != -3) {
    printf("ERROR: test18\n");
    exit(1);
  }
  printf("PASS: test18\n");
  exit(0);
}

void
time4_cb(int value)
{
  if (value != -6) {
    printf("ERROR: test18\n");
    exit(1);
  }
  warnIfNormalDisplay = 0;
  glutTimerFunc(750, time5_cb, -3);
  glutSetWindow(sub_win);
  glutPostRedisplay();
  glutSetWindow(main_win);
  if (overlaySupported) {
    glutPostOverlayRedisplay();
  }
}

void
time3_cb(int value)
{
  if (value != 6) {
    printf("ERROR: test18\n");
    exit(1);
  }
  glutSetWindow(main_win);
  glutHideOverlay();
  overlayHidden = 1;
  warnIfNormalDisplay = 1;
  glutTimerFunc(500, time4_cb, -6);
}

void
time2_cb(int value)
{
  if (value != 56) {
    printf("ERROR: test18\n");
    exit(1);
  }
  glutSetWindow(main_win);
  if (overlaySupported) {
    glutShowOverlay();
    overlayHidden = 0;
  }
  glutSetWindow(sub_win);
  glutHideWindow();
  subHidden = 1;
  glutTimerFunc(500, time3_cb, 6);
}

void
time_cb(int value)
{
  if (value != 456) {
    printf("ERROR: test18\n");
    exit(1);
  }
  glutSetWindow(sub_win);
  subHidden = 0;
  glutShowWindow();
  glutTimerFunc(500, time2_cb, 56);
}

void
display(void)
{
  if (warnIfNormalDisplay) {
    printf("WARNING: hiding overlay should not generate normal plane expose!\n");
    printf("does overlay operation work correctly?\n");
  }
  if (glutLayerGet(GLUT_LAYER_IN_USE) != GLUT_NORMAL) {
    printf("ERROR: test18\n");
    exit(1);
  }
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
  if (firstTime) {
    glutTimerFunc(500, time_cb, 456);
    firstTime = 0;
  }
}

void
subDisplay(void)
{
  if (glutLayerGet(GLUT_LAYER_IN_USE) != GLUT_NORMAL) {
    printf("ERROR: test18\n");
    exit(1);
  }
  if (subHidden) {
    printf("display callback generated when subwindow was hidden!\n");
    printf("ERROR: test18\n");
    exit(1);
  }
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

void
overDisplay(void)
{
  if (glutLayerGet(GLUT_LAYER_IN_USE) != GLUT_OVERLAY) {
    printf("ERROR: test18\n");
    exit(1);
  }
  if (overlayHidden) {
    printf("display callback generated when overlay was hidden!\n");
    printf("ERROR: test18\n");
    exit(1);
  }
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

void
subVis(int state)
{
  if (glutLayerGet(GLUT_LAYER_IN_USE) != GLUT_NORMAL) {
    printf("ERROR: test18\n");
    exit(1);
  }
  if (subHidden && state == GLUT_VISIBLE) {
    printf("visible callback generated when overlay was hidden!\n");
    printf("ERROR: test18\n");
    exit(1);
  }
  if (!subHidden && state == GLUT_NOT_VISIBLE) {
    printf("non-visible callback generated when overlay was shown!\n");
    printf("ERROR: test18\n");
    exit(1);
  }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitWindowSize(300, 300);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);

  main_win = glutCreateWindow("test18");

  if (glutGet(GLUT_WINDOW_COLORMAP_SIZE) != 0) {
    printf("RGBA color model windows should report zero colormap entries.\n");
    printf("ERROR: test18\n");
    exit(1);
  }

  glutInitDisplayMode(GLUT_SINGLE | GLUT_INDEX);
  glutDisplayFunc(display);

  overlaySupported = glutLayerGet(GLUT_OVERLAY_POSSIBLE);
  if (overlaySupported) {
    glutEstablishOverlay();
    glutHideOverlay();
    overlayHidden = 1;
    glutOverlayDisplayFunc(overDisplay);
    transP = glutLayerGet(GLUT_TRANSPARENT_INDEX);
    glClearIndex(glutLayerGet(GLUT_TRANSPARENT_INDEX));
    opaqueP = (transP + 1) % glutGet(GLUT_WINDOW_COLORMAP_SIZE);
    glutSetColor(opaqueP, 1.0, 0.0, 1.0);
    glClearIndex(opaqueP);
  }
  else
  {
      printf("UNRESOLVED: need overlays for this test (your window system lacks overlays)\n");
      return 0;
  }
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  sub_win = glutCreateSubWindow(main_win, 10, 10, 20, 20);
  glClearColor(0.0, 1.0, 0.0, 0.0);
  glutDisplayFunc(subDisplay);
  glutVisibilityFunc(subVis);
  glutHideWindow();
  subHidden = 1;

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
