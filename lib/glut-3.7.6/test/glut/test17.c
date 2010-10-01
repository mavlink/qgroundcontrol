
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Test for GLUT 3.0's overlay functionality. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

static int transP;
static int main_win;
static float x = 0, y = 0;

static void
render_normal(void)
{
  glutUseLayer(GLUT_NORMAL);
  glClear(GL_COLOR_BUFFER_BIT);
  glColor3f(0.0, 0.0, 1.0);
  glBegin(GL_POLYGON);
  glVertex2f(.2, .28);
  glVertex2f(.5, .58);
  glVertex2f(.2, .58);
  glEnd();
  glFlush();
}

static void
render_overlay(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glBegin(GL_POLYGON);
  glVertex2f(.2 + x, .2 + y);
  glVertex2f(.5 + x, .5 + y);
  glVertex2f(.2 + x, .5 + y);
  glEnd();
  glFlush();
}

static void
render(void)
{
  glutUseLayer(GLUT_NORMAL);
  render_normal();
  if (glutLayerGet(GLUT_HAS_OVERLAY)) {
    glutUseLayer(GLUT_OVERLAY);
    render_overlay();
  }
}

static void
render_sub(void)
{
  printf("render_sub\n");
  glutUseLayer(GLUT_NORMAL);
  render_normal();
  if (glutLayerGet(GLUT_HAS_OVERLAY)) {
    glutUseLayer(GLUT_OVERLAY);
    render_overlay();
  }
}

static int display_count = 0;
static int damage_expectation;

static void
timer(int value)
{
  if (value != 777) {
    printf("FAIL: unexpected timer value\n");
    exit(1);
  }
  damage_expectation = 1;
  glutShowWindow();
}

static void
time2(int value)
{
  if (value == 666) {
    printf("PASS: test17\n");
    exit(0);
  }
  if (value != 888) {
    printf("FAIL: bad value\n");
    exit(1);
  }
  glutDestroyWindow(main_win);
  glutTimerFunc(500, time2, 666);
}

static void
move_on(void)
{
  display_count++;
  if (display_count == 2) {
    damage_expectation = 1;
    glutIconifyWindow();
    glutTimerFunc(500, timer, 777);
  }
  if (display_count == 4) {
    printf("display_count == 4\n");
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutCreateSubWindow(main_win, 10, 10, 150, 150);
    glClearColor(0.5, 0.5, 0.5, 0.0);
    glutDisplayFunc(render_sub);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_INDEX);
    glutEstablishOverlay();
    glutCopyColormap(main_win);
    glutSetColor((transP + 1) % 2, 0.0, 1.0, 1.0);
    glutRemoveOverlay();
    glutEstablishOverlay();
    glutCopyColormap(main_win);
    glutCopyColormap(main_win);
    glutSetColor((transP + 1) % 2, 1.0, 1.0, 1.0);
    glClearIndex(transP);
    glIndexf((transP + 1) % 2);
    glutSetWindow(main_win);
    glutRemoveOverlay();
    glutTimerFunc(500, time2, 888);
  }
}

static void
display_normal(void)
{
  if (glutLayerGet(GLUT_NORMAL_DAMAGED) != damage_expectation) {
    printf("FAIL: normal damage not expected\n");
    exit(1);
  }
  render_normal();
  move_on();
}

static void
display_overlay(void)
{
  if (glutLayerGet(GLUT_OVERLAY_DAMAGED) != damage_expectation) {
    printf("FAIL: overlay damage not expected\n");
    exit(1);
  }
  render_overlay();
  move_on();
}

static void
display2(void)
{
  static int been_here = 0;

  if (glutLayerGet(GLUT_NORMAL_DAMAGED) != 0) {
    printf("FAIL: normal damage not expected\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_OVERLAY_DAMAGED) != 0) {
    printf("FAIL: overlay damage not expected\n");
    exit(1);
  }
  if (been_here) {
    glutPostOverlayRedisplay();
  } else {
    glutOverlayDisplayFunc(display_overlay);
    glutDisplayFunc(display_normal);
    damage_expectation = 0;
    glutPostOverlayRedisplay();
    glutPostRedisplay();
  }
}

static void
display(void)
{
  if (glutLayerGet(GLUT_NORMAL_DAMAGED) == 0) {
    printf("FAIL: normal damage expected\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_OVERLAY_DAMAGED) == 0) {
    printf("FAIL: overlay damage expected\n");
    exit(1);
  }
  render();

  glutDisplayFunc(display2);
  glutPostRedisplay();
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitWindowSize(300, 300);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_INDEX);

  if (!glutLayerGet(GLUT_OVERLAY_POSSIBLE)) {
    printf("UNRESOLVED: need overlays for this test (your window system lacks overlays)\n");
    exit(0);
  }
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  main_win = glutCreateWindow("test17");

  if (glutLayerGet(GLUT_LAYER_IN_USE) == GLUT_OVERLAY) {
    printf("FAIL: overlay should not be in use\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_HAS_OVERLAY)) {
    printf("FAIL: overlay should not exist\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_TRANSPARENT_INDEX) != -1) {
    printf("FAIL: transparent pixel of normal plane should be -1\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_NORMAL_DAMAGED) != 0) {
    printf("FAIL: no normal damage yet\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_OVERLAY_DAMAGED) != -1) {
    printf("FAIL: no overlay damage status yet\n");
    exit(1);
  }
  glClearColor(0.0, 1.0, 0.0, 0.0);

  glutInitDisplayMode(GLUT_SINGLE | GLUT_INDEX);

  /* Small torture test. */
  glutEstablishOverlay();
  glutRemoveOverlay();
  glutEstablishOverlay();
  glutEstablishOverlay();
  glutShowOverlay();
  glutHideOverlay();
  glutShowOverlay();
  glutRemoveOverlay();
  glutRemoveOverlay();
  glutEstablishOverlay();

  if (glutGet(GLUT_WINDOW_RGBA)) {
    printf("FAIL: overlay should not be RGBA\n");
    exit(1);
  }
  glutUseLayer(GLUT_NORMAL);
  if (!glutGet(GLUT_WINDOW_RGBA)) {
    printf("FAIL: normal should be RGBA\n");
    exit(1);
  }
  glutUseLayer(GLUT_OVERLAY);
  if (glutGet(GLUT_WINDOW_RGBA)) {
    printf("FAIL: overlay should not be RGBA\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_LAYER_IN_USE) == GLUT_NORMAL) {
    printf("FAIL: overlay should be in use\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_HAS_OVERLAY) == 0) {
    printf("FAIL: overlay should exist\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_TRANSPARENT_INDEX) == -1) {
    printf("FAIL: transparent pixel should exist\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_NORMAL_DAMAGED) != 0) {
    printf("FAIL: no normal damage yet\n");
    exit(1);
  }
  if (glutLayerGet(GLUT_OVERLAY_DAMAGED) != 0) {
    printf("FAIL: no overlay damage yet\n");
    exit(1);
  }
  transP = glutLayerGet(GLUT_TRANSPARENT_INDEX);
  glClearIndex(glutLayerGet(GLUT_TRANSPARENT_INDEX));
  glutSetColor((transP + 1) % 2, 1.0, 0.0, 1.0);
  glIndexi((transP + 1) % 2);

  glutUseLayer(GLUT_NORMAL);
  if (glutLayerGet(GLUT_LAYER_IN_USE) == GLUT_OVERLAY) {
    printf("FAIL: overlay should not be in use\n");
    exit(1);
  }
  glutDisplayFunc(display);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
