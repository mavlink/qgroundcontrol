/* 
 * agv_example.c  (version 1.0)
 *
 * Example program to show how to use AGV
 *
 * See agviewer.h, agviewer.c and comments within for more info
 *
 * Philip Winston - 4/11/95
 * pwinston@hmc.edu
 * http://www.cs.hmc.edu/people/pwinston
 */

#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "agviewer.h"

typedef enum {NOTALLOWED, AXES, STUFF, RING } DisplayLists;

int DrawAxes = 0;

#define ROTATEINC 2;

GLfloat Rotation = 0;  /* start ring flat and not spinning */
int     Rotating = 0;


void myGLInit(void)
{
  GLfloat mat_ambuse[] = { 0.6, 0.0, 0.0, 1.0 };
  GLfloat mat_specular[] = { 0.4, 0.4, 0.4, 1.0 };

  GLfloat light0_position[] = { 0.6, 0.4, 0.3, 0.0 };

  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_ambuse);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, 25.0);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glEnable(GL_NORMALIZE);

  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);

  glShadeModel(GL_SMOOTH);

  glFlush();
} 


void MakeDisplayLists(void)
{
  glNewList(STUFF, GL_COMPILE);
  glPushMatrix();
    glutSolidCube(1.0);
    glTranslatef(2, 0, 0);
    glutSolidSphere(0.5, 10, 10);
    glTranslatef(-2, 0, 3);
    glRotatef(-90, 1, 0, 0);
    glutSolidCone(0.5, 1.0, 8, 8);
  glPopMatrix();
  glEndList();

  glNewList(RING, GL_COMPILE);
    glutSolidTorus(0.1, 0.5, 8, 15);
  glEndList();
}


void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  gluPerspective(60, 1, 0.01, 100);

    /* so this replaces gluLookAt or equiv */
  agvViewTransform();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

    /* we call agvMakeAxesList() to make this display list */
  if (DrawAxes)
    glCallList(AXES);

  glCallList(STUFF);

  glTranslatef(-2, 1, -2);
  glRotatef(Rotation, 1, 0, 0);
  glCallList(RING);

  glutSwapBuffers();
  glFlush();
}


  /* rotate the axis and adjust position if nec. */
void rotatethering(void)
{ 
  Rotation += ROTATEINC;

  if (agvMoving)   /* we since we are the only idle function, we must */
    agvMove();     /* give AGV the chance to update the eye position */

  glutPostRedisplay();
}

typedef enum { MENU_AXES, MENU_QUIT, MENU_RING } MenuChoices;

void handlemenu(int value)
{
  switch (value) {
    case MENU_AXES:
      DrawAxes = !DrawAxes;
      break;
    case MENU_QUIT:
      exit(0);
      break;
    case MENU_RING:
      Rotating = !Rotating;
      if (Rotating) {
	glutIdleFunc(rotatethering);    /* install our idle function */
	agvSetAllowIdle(0);             /* and tell AGV to not */
      } else {
	glutIdleFunc(NULL);    /* uninstall our idle function      */
	agvSetAllowIdle(1);    /* and tell AGV it can mess with it */
      }
      break;
    }
  glutPostRedisplay();
}

void visible(int v)
{
  if (v == GLUT_VISIBLE) {
    if (Rotating) {
      glutIdleFunc(rotatethering);
      agvSetAllowIdle(0);
    } else {
      glutIdleFunc(NULL);
      agvSetAllowIdle(1);      
    }
  } else {
      glutIdleFunc(NULL);
      agvSetAllowIdle(0);
  }
}


void MenuInit(void)
{
  int sub2 = glutCreateMenu(agvSwitchMoveMode);   /* pass these right to */
  glutAddMenuEntry("Flying move",  FLYING);       /* agvSwitchMoveMode() */
  glutAddMenuEntry("Polar move",   POLAR);

  glutCreateMenu(handlemenu);
  glutAddSubMenu("Movement", sub2);
  glutAddMenuEntry("Toggle Axes", MENU_AXES);
  glutAddMenuEntry("Toggle ring rotation", MENU_RING);
  glutAddMenuEntry("Quit", MENU_QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}


int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow("AGV example");
  
  glutVisibilityFunc(visible);

  if (Rotating)
    glutIdleFunc(rotatethering);
 
    /*
     * let AGV know if it can mess with the idle function (if we've
     * just installed an idle function, we tell AGV it can't touch it)
     */

  agvInit(!Rotating);
    
    /* 
     * agvInit() installs mouse, motion, and keyboard handles, but 
     * we don't care for this example cause we only use right button menu
     */

  agvMakeAxesList(AXES);  /* create AGV axes */

  myGLInit(); 
  MakeDisplayLists();
  MenuInit();

  glutDisplayFunc(display);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}


