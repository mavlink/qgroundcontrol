
/* Copyright (c) Mark J. Kilgard, 1994.  */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* zoomdino demonstrates GLUT 3.0's new overlay support.  Both
   rubber-banding the display of a help message use the overlays. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#include <GL/glut.h>

typedef enum {
  RESERVED, BODY_SIDE, BODY_EDGE, BODY_WHOLE, ARM_SIDE, ARM_EDGE, ARM_WHOLE,
  LEG_SIDE, LEG_EDGE, LEG_WHOLE, EYE_SIDE, EYE_EDGE, EYE_WHOLE, DINOSAUR
} displayLists;

GLfloat angle = -150;   /* in degrees */
int moving, begin;
int W = 300, H = 300;
GLdouble bodyWidth = 3.0;
int newModel = 1;
/* *INDENT-OFF* */
GLfloat body[][2] = { {0, 3}, {1, 1}, {5, 1}, {8, 4}, {10, 4}, {11, 5},
  {11, 11.5}, {13, 12}, {13, 13}, {10, 13.5}, {13, 14}, {13, 15}, {11, 16},
  {8, 16}, {7, 15}, {7, 13}, {8, 12}, {7, 11}, {6, 6}, {4, 3}, {3, 2},
  {1, 2} };
GLfloat arm[][2] = { {8, 10}, {9, 9}, {10, 9}, {13, 8}, {14, 9}, {16, 9},
  {15, 9.5}, {16, 10}, {15, 10}, {15.5, 11}, {14.5, 10}, {14, 11}, {14, 10},
  {13, 9}, {11, 11}, {9, 11} };
GLfloat leg[][2] = { {8, 6}, {8, 4}, {9, 3}, {9, 2}, {8, 1}, {8, 0.5}, {9, 0},
  {12, 0}, {10, 1}, {10, 2}, {12, 4}, {11, 6}, {10, 7}, {9, 7} };
GLfloat eye[][2] = { {8.75, 15}, {9, 14.7}, {9.6, 14.7}, {10.1, 15},
  {9.6, 15.25}, {9, 15.25} };
GLfloat lightZeroPosition[] = {10.0, 4.0, 10.0, 1.0};
GLfloat lightZeroColor[] = {0.8, 1.0, 0.8, 1.0}; /* green-tinted */
GLfloat lightOnePosition[] = {-1.0, -2.0, 1.0, 0.0};
GLfloat lightOneColor[] = {0.6, 0.3, 0.2, 1.0}; /* red-tinted */
GLfloat skinColor[] = {0.1, 1.0, 0.1, 1.0}, eyeColor[] = {1.0, 0.2, 0.2, 1.0};
int overlaySupport, red, white, transparent, rubberbanding;
int anchorx, anchory, stretchx, stretchy, pstretchx, pstretchy;
float vx, vy, vx2, vy2, vw, vh;
float wx, wy, wx2, wy2, ww, wh;
int fancy, wasFancy, help, clearHelp;
/* *INDENT-ON* */

void
extrudeSolidFromPolygon(GLfloat data[][2], unsigned int dataSize,
  GLdouble thickness, GLuint side, GLuint edge, GLuint whole)
{
  static GLUtriangulatorObj *tobj = NULL;
  GLdouble vertex[3], dx, dy, len;
  int i;
  int count = dataSize / (int) (2 * sizeof(GLfloat));

  if (tobj == NULL) {
    tobj = gluNewTess();  /* create and initialize a GLU
                             polygontesselation object */
    gluTessCallback(tobj, GLU_BEGIN, glBegin);
    gluTessCallback(tobj, GLU_VERTEX, glVertex2fv);  /* semi-tricky 

                                                      */
    gluTessCallback(tobj, GLU_END, glEnd);
  }
  glNewList(side, GL_COMPILE);
  glShadeModel(GL_SMOOTH);  /* smooth minimizes seeing
                               tessellation */
  gluBeginPolygon(tobj);
  for (i = 0; i < count; i++) {
    vertex[0] = data[i][0];
    vertex[1] = data[i][1];
    vertex[2] = 0;
    gluTessVertex(tobj, vertex, data[i]);
  }
  gluEndPolygon(tobj);
  glEndList();
  glNewList(edge, GL_COMPILE);
  glShadeModel(GL_FLAT);  /* flat shade keeps angular hands
                             from being "smoothed" */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= count; i++) {
    /* mod function handles closing the edge */
    glVertex3f(data[i % count][0], data[i % count][1], 0.0);
    glVertex3f(data[i % count][0], data[i % count][1], thickness);

    /* Calculate a unit normal by dividing by Euclidean
       distance. We could be lazy and use
       glEnable(GL_NORMALIZE) so we could pass in arbitrary
       normals for a very slight performance hit. */

    dx = data[(i + 1) % count][1] - data[i % count][1];
    dy = data[i % count][0] - data[(i + 1) % count][0];
    len = sqrt(dx * dx + dy * dy);
    glNormal3f(dx / len, dy / len, 0.0);
  }
  glEnd();
  glEndList();
  glNewList(whole, GL_COMPILE);
  glFrontFace(GL_CW);
  glCallList(edge);
  glNormal3f(0.0, 0.0, -1.0);  /* constant normal for side */
  glCallList(side);
  glPushMatrix();
  glTranslatef(0.0, 0.0, thickness);
  glFrontFace(GL_CCW);
  glNormal3f(0.0, 0.0, 1.0);  /* opposite normal for other side */
  glCallList(side);
  glPopMatrix();
  glEndList();
}

void
makeDinosaur(void)
{
  extrudeSolidFromPolygon(body, sizeof(body), bodyWidth,
    BODY_SIDE, BODY_EDGE, BODY_WHOLE);
  extrudeSolidFromPolygon(arm, sizeof(arm), bodyWidth / 4,
    ARM_SIDE, ARM_EDGE, ARM_WHOLE);
  extrudeSolidFromPolygon(leg, sizeof(leg), bodyWidth / 2,
    LEG_SIDE, LEG_EDGE, LEG_WHOLE);
  extrudeSolidFromPolygon(eye, sizeof(eye), bodyWidth + 0.2,
    EYE_SIDE, EYE_EDGE, EYE_WHOLE);
  glNewList(DINOSAUR, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, skinColor);
  glCallList(BODY_WHOLE);
  glPushMatrix();
  glTranslatef(0.0, 0.0, bodyWidth);
  glCallList(ARM_WHOLE);
  glCallList(LEG_WHOLE);
  glTranslatef(0.0, 0.0, -bodyWidth - bodyWidth / 4);
  glCallList(ARM_WHOLE);
  glTranslatef(0.0, 0.0, -bodyWidth / 4);
  glCallList(LEG_WHOLE);
  glTranslatef(0.0, 0.0, bodyWidth / 2 - 0.1);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, eyeColor);
  glCallList(EYE_WHOLE);
  glPopMatrix();
  glEndList();
}

void
recalcModelView(void)
{
  glPopMatrix();
  glPushMatrix();
  glRotatef(angle, 0.0, 1.0, 0.0);
  glTranslatef(-8, -8, -bodyWidth / 2);
  newModel = 0;
}

void
redraw(void)
{
  if (newModel)
    recalcModelView();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glCallList(DINOSAUR);
  glutSwapBuffers();
}

void
output(int x, int y, char *string)
{
  int len, i;

  glRasterPos2f(x, y);
  len = (int) strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);
  }
}

char *helpMsg[] =
{
  "Welcome to zoomdino!",
  "   Left mouse button rotates",
  "     the dinosaur.",
  "   Middle mouse button zooms",
  "     via overlay rubber-banding.",
  "   Right mouse button shows",
  "     pop-up menu.",
  "   To reset view, use \"Reset",
  "     Projection\".",
  "(This message is in the overlays.)",
  NULL
};

void
redrawOverlay(void)
{
  if (help) {
    int i;

    glClear(GL_COLOR_BUFFER_BIT);
    glIndexi(white);
    for (i = 0; helpMsg[i]; i++) {
      output(15, 24 + i * 18, helpMsg[i]);
    }
    return;
  }
  if (glutLayerGet(GLUT_OVERLAY_DAMAGED) || clearHelp) {
    /* Opps, damage means we need a full clear. */
    glClear(GL_COLOR_BUFFER_BIT);
    clearHelp = 0;
    wasFancy = 0;
  } else {
    /* Goody!  No damage.  Just erase last rubber-band. */
    if (fancy || wasFancy) {
      glLineWidth(3.0);
    }
    glIndexi(transparent);
    glBegin(GL_LINE_LOOP);
    glVertex2i(anchorx, anchory);
    glVertex2i(anchorx, pstretchy);
    glVertex2i(pstretchx, pstretchy);
    glVertex2i(pstretchx, anchory);
    glEnd();
  }
  if (wasFancy) {
    glLineWidth(1.0);
    wasFancy = 0;
  }
  if (fancy)
    glLineWidth(3.0);
  glIndexi(red);
  glBegin(GL_LINE_LOOP);
  glVertex2i(anchorx, anchory);
  glVertex2i(anchorx, stretchy);
  glVertex2i(stretchx, stretchy);
  glVertex2i(stretchx, anchory);
  glEnd();
  if (fancy) {
    glLineWidth(1.0);
    glIndexi(white);
    glBegin(GL_LINE_LOOP);
    glVertex2i(anchorx, anchory);
    glVertex2i(anchorx, stretchy);
    glVertex2i(stretchx, stretchy);
    glVertex2i(stretchx, anchory);
    glEnd();
  }
  glFlush();

  /* Remember last place rubber-banded so the rubber-band can
     be erased next redisplay. */
  pstretchx = stretchx;
  pstretchy = stretchy;
}

void
defaultProjection(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  vx = -1.0;
  vw = 2.0;
  vy = -1.0;
  vh = 2.0;
  glFrustum(vx, vx + vw, vy, vy + vh, 1.0, 40);
  glMatrixMode(GL_MODELVIEW);
}

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
      moving = 1;
      begin = x;
    } else if (state == GLUT_UP) {
      glutSetCursor(GLUT_CURSOR_INHERIT);
      moving = 0;
    }
  }
  if (overlaySupport && button == GLUT_MIDDLE_BUTTON) {
    if (state == GLUT_DOWN) {
      help = 0;
      clearHelp = 1;
      rubberbanding = 1;
      anchorx = x;
      anchory = y;
      stretchx = x;
      stretchy = y;
      glutShowOverlay();
    } else if (state == GLUT_UP) {
      rubberbanding = 0;
      glutHideOverlay();
      glutUseLayer(GLUT_NORMAL);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();

#undef max
#undef min
#define max(a,b)  ((a) > (b) ? (a) : (b))
#define min(a,b)  ((a) < (b) ? (a) : (b))

      wx = min(anchorx, stretchx);
      wy = min(H - anchory, H - stretchy);
      wx2 = max(anchorx, stretchx);
      wy2 = max(H - anchory, H - stretchy);
      ww = wx2 - wx;
      wh = wy2 - wy;
      if (ww == 0 || wh == 0) {
        glutUseLayer(GLUT_NORMAL);
        defaultProjection();
      } else {

        vx2 = wx2 / W * vw + vx;
        vx = wx / W * vw + vx;
        vy2 = wy2 / H * vh + vy;
        vy = wy / H * vh + vy;
        vw = vx2 - vx;
        vh = vy2 - vy;

        glFrustum(vx, vx + vw, vy, vy + vh, 1.0, 40);
      }
      glutPostRedisplay();
      glMatrixMode(GL_MODELVIEW);
    }
  }
}

void
motion(int x, int y)
{
  if (moving) {
    angle = angle + (x - begin);
    begin = x;
    newModel = 1;
    glutPostRedisplay();
  }
  if (rubberbanding) {
    stretchx = x;
    stretchy = y;
    glutPostOverlayRedisplay();
  }
}

void
reshape(int w, int h)
{
  if (overlaySupport) {
    glutUseLayer(GLUT_OVERLAY);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glScalef(1, -1, 1);
    glTranslatef(0, -h, 0);
    glMatrixMode(GL_MODELVIEW);
    glutUseLayer(GLUT_NORMAL);
  }
  glViewport(0, 0, w, h);
  W = w;
  H = h;
}

GLboolean lightZeroSwitch = GL_TRUE, lightOneSwitch = GL_TRUE;

void
controlLights(int value)
{
  glutUseLayer(GLUT_NORMAL);
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
  case 3:
    defaultProjection();
    break;
  case 4:
    fancy = 1;
    break;
  case 5:
    fancy = 0;
    wasFancy = 1;
    break;
  case 6:
    if (!rubberbanding)
      help = 1;
    glutShowOverlay();
    glutPostOverlayRedisplay();
    break;
  }
  glutPostRedisplay();
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("zoomdino");
  glutDisplayFunc(redraw);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutCreateMenu(controlLights);
  glutAddMenuEntry("Toggle right light", 1);
  glutAddMenuEntry("Toggle left light", 2);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  makeDinosaur();
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  defaultProjection();
  gluLookAt(0.0, 0.0, 30.0,  /* eye is at (0,0,30) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in postivie Y direction */
  glPushMatrix();       /* dummy push so we can pop on model
                           recalc */
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05);
  glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, lightOneColor);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_INDEX);
  overlaySupport = glutLayerGet(GLUT_OVERLAY_POSSIBLE);
  if (overlaySupport) {
    glutEstablishOverlay();
    glutHideOverlay();
    transparent = glutLayerGet(GLUT_TRANSPARENT_INDEX);
    glClearIndex(transparent);
    red = (transparent + 1) % glutGet(GLUT_WINDOW_COLORMAP_SIZE);
    white = (transparent + 2) % glutGet(GLUT_WINDOW_COLORMAP_SIZE);
    glutSetColor(red, 1.0, 0.0, 0.0);  /* Red. */
    glutSetColor(white, 1.0, 1.0, 1.0);  /* White. */
    glutOverlayDisplayFunc(redrawOverlay);
    glutReshapeFunc(reshape);
    glutSetWindowTitle("zoomdino with rubber-banding");
    glutAddMenuEntry("------------------", 0);
    glutAddMenuEntry("Reset projection", 3);
    glutAddMenuEntry("------------------", 0);
    glutAddMenuEntry("Fancy rubber-banding", 4);
    glutAddMenuEntry("Simple rubber-banding", 5);
    glutAddMenuEntry("------------------", 0);
    glutAddMenuEntry("Show help", 6);
  } else {
    printf("Sorry, no whizzy zoomdino overlay usage!\n");
  }
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
