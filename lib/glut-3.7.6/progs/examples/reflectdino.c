
/* Copyright (c) Mark J. Kilgard, 1994, 1997.  */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Very simple example of how to achieve reflections on a flat
   surface using OpenGL blending.  The example has a mode using
   OpenGL stenciling to avoid drawing the reflection not on the top of the
   floor.  Initially, stenciling is not used so if you look (by holding
   down the left mouse button and moving) at the dinosaur from "below"
   the floor, you'll see a bogus dinosaur and appreciate how the basic
   technique works.  Enable stenciling with the popup menu and the
   bogus dinosaur goes away!  Also, notice that OpenGL lighting works
   correctly with reflections. */

/* Check out the comments in the "redraw" routine to see how the
   reflection blending and surface stenciling is done. */

/* This program is derived from glutdino.c */

/* Compile: cc -o reflectdino reflectdino.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#include <GL/glut.h>

typedef enum {
  RESERVED, BODY_SIDE, BODY_EDGE, BODY_WHOLE, ARM_SIDE, ARM_EDGE, ARM_WHOLE,
  LEG_SIDE, LEG_EDGE, LEG_WHOLE, EYE_SIDE, EYE_EDGE, EYE_WHOLE
} displayLists;

GLfloat angle = 20;   /* in degrees */
GLfloat angle2 = 30;   /* in degrees */
int moving, startx, starty;
int W = 300, H = 300;
int useStencil = 0;  /* Initially, allow the artifacts. */
GLdouble bodyWidth = 3.0;
float jump = 0.0;
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
GLfloat lightZeroPosition[] = {10.0, 14.0, 10.0, 1.0};
GLfloat lightZeroColor[] = {0.8, 1.0, 0.8, 1.0}; /* green-tinted */
GLfloat lightOnePosition[] = {-1.0, 1.0, 1.0, 0.0};
GLfloat lightOneColor[] = {0.6, 0.3, 0.2, 1.0}; /* red-tinted */
GLfloat skinColor[] = {0.1, 1.0, 0.1, 1.0}, eyeColor[] = {1.0, 0.2, 0.2, 1.0};
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
                             polygon tesselation object */
    gluTessCallback(tobj, GLU_BEGIN, glBegin);
    gluTessCallback(tobj, GLU_VERTEX, glVertex2fv);  /* semi-tricky */
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
       distance. We * could be lazy and use
       glEnable(GL_NORMALIZE) so we could pass in * arbitrary
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
}

void
drawDinosaur(void)
{
  glPushMatrix();
  glTranslatef(0.0, jump, 0.0);
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
  glPopMatrix();
}

void
drawFloor(void)
{
  glDisable(GL_LIGHTING);
  glBegin(GL_QUADS);
    glVertex3f(-18.0, 0.0, 27.0);
    glVertex3f(27.0, 0.0, 27.0);
    glVertex3f(27.0, 0.0, -18.0);
    glVertex3f(-18.0, 0.0, -18.0);
  glEnd();
  glEnable(GL_LIGHTING);
}

void
redraw(void)
{
  if (useStencil) {
    /* Clear; default stencil clears to zero. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  } else {
    /* Not using stencil; just clear color and depth. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  glPushMatrix();
    /* Perform scene rotations based on user mouse input. */
    glRotatef(angle2, 1.0, 0.0, 0.0);
    glRotatef(angle, 0.0, 1.0, 0.0);

    /* Translate the dinosaur to be at (0,0,0). */
    glTranslatef(-8, -8, -bodyWidth / 2);

    glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
    glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);

    if (useStencil) {
     
      /* We can eliminate the visual "artifact" of seeing the "flipped"
	 dinosaur underneath the floor by using stencil.  The idea is
	 draw the floor without color or depth update but so that 
	 a stencil value of one is where the floor will be.  Later when
	 rendering the dinosaur reflection, we will only update pixels
	 with a stencil value of 1 to make sure the reflection only
	 lives on the floor, not below the floor. */

      /* Don't update color or depth. */
      glDisable(GL_DEPTH_TEST);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

      /* Draw 1 into the stencil buffer. */
      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
      glStencilFunc(GL_ALWAYS, 1, 0xffffffff);

      /* Now render floor; floor pixels just get their stencil set to 1. */
      drawFloor();

      /* Re-enable update of color and depth. */ 
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glEnable(GL_DEPTH_TEST);

      /* Now, only render where stencil is set to 1. */
      glStencilFunc(GL_EQUAL, 1, 0xffffffff);  /* draw if ==1 */
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }

    glPushMatrix();

      /* The critical reflection step: Reflect dinosaur through the floor
         (the Y=0 plane) to make a relection. */
      glScalef(1.0, -1.0, 1.0);

      /* Position lights now in reflected space. */
      glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
      glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);

      /* XXX Ugh, unfortunately the back face culling reverses when we reflect
	 the dinosaur.  Easy solution is just disable back face culling for
	 rendering the reflection.  Also, the normals for lighting get screwed
	 up by the scale; enabled normalize to ensure normals are still
	 properly normalized despite the scaling.  We could have fixed the
	 dinosaur rendering code, but this is more expedient. */
      glEnable(GL_NORMALIZE);
      glCullFace(GL_FRONT);

      /* Draw the reflected dinosaur. */
      drawDinosaur();

      /* Disable noramlize again and re-enable back face culling. */
      glDisable(GL_NORMALIZE);
      glCullFace(GL_BACK);

    glPopMatrix();

    /* Restore light positions on returned from reflected space. */
    glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
    glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);


    if (useStencil) {
      /* Don't want to be using stenciling for drawing the actual dinosaur
	 (not its reflection) and the floor. */
      glDisable(GL_STENCIL_TEST);
    }

    /* Back face culling will get used to only draw either the top or the
       bottom floor.  This let's us get a floor with two distinct
       appearances.  The top floor surface is reflective and kind of red.
       The bottom floor surface is not reflective and blue. */

    /* Draw "top" of floor.  Use blending to blend in reflection. */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.7, 0.0, 0.0, 0.3);
    drawFloor();
    glDisable(GL_BLEND);

    /* Draw "bottom" of floor in blue. */
    glFrontFace(GL_CW);  /* Switch face orientation. */
    glColor4f(0.1, 0.1, 0.7, 1.0);
    drawFloor();
    glFrontFace(GL_CCW);

    /* Draw "actual" dinosaur, not its reflection. */
    drawDinosaur();

  glPopMatrix();

  glutSwapBuffers();
}

/* ARGSUSED2 */
void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    moving = 1;
    startx = x;
    starty = y;
  }
  if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
    moving = 0;
  }
}

/* ARGSUSED1 */
void
motion(int x, int y)
{
  if (moving) {
    angle = angle + (x - startx);
    angle2 = angle2 + (y - starty);
    startx = x;
    starty = y;
    glutPostRedisplay();
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
  case 3:
    useStencil = 1 - useStencil;
    break;
  }
  glutPostRedisplay();
}

void
idle(void)
{
  static float time;

  time = glutGet(GLUT_ELAPSED_TIME) / 500.0;

  jump = 3.0 * fabs(sin(time));
  glutPostRedisplay();
}

void 
visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(idle);
  else
    glutIdleFunc(NULL);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
  glutCreateWindow("Leapin' Lizards");
  glutDisplayFunc(redraw);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutVisibilityFunc(visible);
  glutCreateMenu(controlLights);
  glutAddMenuEntry("Toggle right light", 1);
  glutAddMenuEntry("Toggle left light", 2);
  glutAddMenuEntry("Toggle stenciling out reflection artifacts", 3);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  makeDinosaur();
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 80.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 40.0,  /* eye is at (0,0,30) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in postivie Y direction */
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, lightOneColor);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);

    glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
    glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
