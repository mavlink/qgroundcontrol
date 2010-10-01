
/* Copyright (c) Mark J. Kilgard, 1997.  */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* compile: cc -o martini martini.c trackball.c -lgle -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#include <GL/glut.h>
#include <GL/tube.h>
#include "trackball.h"

int spinning = 0, moving = 0;
int beginx, beginy;
int W = 300, H = 300;
float curquat[4];
float lastquat[4];
int newModel = 1;
/* *INDENT-OFF* */
GLfloat lightZeroPosition[] = {10.0, 4.0, 10.0, 1.0};
GLfloat lightZeroColor[] = {0.8, 1.0, 0.8, 1.0}; /* green-tinted */
GLfloat lightOnePosition[] = {-1.0, -2.0, 1.0, 0.0};
GLfloat lightOneColor[] = {0.6, 0.3, 0.2, 1.0}; /* red-tinted */
/* *INDENT-ON* */

void
recalcModelView(void)
{
  GLfloat m[4][4];

  glPopMatrix();
  glPushMatrix();
  build_rotmatrix(m, curquat);
  glMultMatrixf(&m[0][0]);
  newModel = 0;
}

/* the arrays in which we will store out polyline */
#define NPTS 25
double points[NPTS][3];
double radii[NPTS];
int idx = 0;

#define REV(r, y) { \
  points[idx][0] = 0.0; \
  points[idx][1] = y - 3.0; \
  points[idx][2] = 0.0; \
  radii[idx] = r; \
  idx ++; \
}

void 
InitStuff(void)
{
  /* Initialize the join style here, no capping. */
  gleSetJoinStyle(TUBE_NORM_EDGE | TUBE_JN_ANGLE);
  gleSetNumSlices(30);
  REV(0.0, 5.0);
  REV(0.0, 4.0);
  REV(2.5, 6.1);
  REV(2.75, 6.0);
  REV(2.75, 5.75);
  REV(0.25, 3.75);
  REV(0.25, 2.66);
  REV(0.4, 2.5);
  REV(0.3, 2.2);
  REV(0.4, 1.9);
  REV(0.25, 1.7);
  REV(0.25, 0.6);
  REV(0.25, 0.101);
  REV(2.0, 0.1);
  REV(2.0, 0.0);
  REV(0.0, 0.05);
  REV(0.0, -1.0);

  /* Capture rendering of martini glass in a display list. */
  glNewList(1, GL_COMPILE);
  glFrontFace(GL_CW);
  glePolyCone(4, points, NULL, radii);
  glFrontFace(GL_CCW);
  glePolyCone(15, &points[1], NULL, &radii[1]);
  glFrontFace(GL_CW);
  glePolyCone(4, &points[13], NULL, &radii[13]);
  glEndList();
}

void
redraw(void)
{
  if (newModel)
    recalcModelView();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  /* Draw the martini glass. */
  glCallList(1);
  glutSwapBuffers();
}

void
myReshape(int w, int h)
{
  glViewport(0, 0, w, h);
  W = w;
  H = h;
}

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    spinning = 0;
    glutIdleFunc(NULL);
    moving = 1;
    beginx = x;
    beginy = y;
  }
  if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
    moving = 0;
  }
}

void
animate(void)
{
  add_quats(lastquat, curquat, curquat);
  newModel = 1;
  glutPostRedisplay();
}

void
motion(int x, int y)
{
  if (moving) {
    trackball(lastquat,
      (2.0 * beginx - W) / W,
      (H - 2.0 * beginy) / H,
      (2.0 * x - W) / W,
      (H - 2.0 * y) / H
      );
    beginx = x;
    beginy = y;
    spinning = 1;
    glutIdleFunc(animate);
  }
}

GLboolean lightZeroSwitch = GL_TRUE, lightOneSwitch = GL_TRUE;

void
controlLights(int value)
{
  switch (value) {
  case 1:
    lightZeroSwitch = !lightZeroSwitch;
    if (lightZeroSwitch) {
      glEnable(GL_LIGHT0);
    } else {
      glDisable(GL_LIGHT0);
    }
    break;
  case 2:
    lightOneSwitch = !lightOneSwitch;
    if (lightOneSwitch) {
      glEnable(GL_LIGHT1);
    } else {
      glDisable(GL_LIGHT1);
    }
    break;
#ifdef GL_MULTISAMPLE_SGIS
  case 3:
    if (glIsEnabled(GL_MULTISAMPLE_SGIS)) {
      glDisable(GL_MULTISAMPLE_SGIS);
    } else {
      glEnable(GL_MULTISAMPLE_SGIS);
    }
    break;
#endif
  case 4:
    glutFullScreen();
    break;
  case 5:
    exit(0);
    break;
  }
  glutPostRedisplay();
}

void
vis(int visible)
{
  if (visible == GLUT_VISIBLE) {
    if (spinning)
      glutIdleFunc(animate);
  } else {
    if (spinning)
      glutIdleFunc(NULL);
  }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  trackball(curquat, 0.0, 0.0, 0.0, 0.0);
  glutCreateWindow("martini");
  glutDisplayFunc(redraw);
  glutReshapeFunc(myReshape);
  glutVisibilityFunc(vis);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutCreateMenu(controlLights);
  glutAddMenuEntry("Toggle right light", 1);
  glutAddMenuEntry("Toggle left light", 2);
  if (glutGet(GLUT_WINDOW_NUM_SAMPLES) > 0) {
    glutAddMenuEntry("Toggle multisampling", 3);
    glutSetWindowTitle("martini (multisample capable)");
  }
  glutAddMenuEntry("Full screen", 4);
  glutAddMenuEntry("Quit", 5);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 40.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 15.0,  /* eye is at (0,0,15) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */
  glPushMatrix();       /* dummy push so we can pop on model recalc */
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05);
  glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, lightOneColor);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  InitStuff();
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
