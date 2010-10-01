
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#define NUM_DIALS 8
#define NUM_BUTTONS 32

int *dials, *buttons;

#undef PI /* Some systems may have this defined. */
#define PI            3.14159265358979323846

void
drawCircle(int x, int y, int r, int dir)
{
   float angle;

   glPushMatrix();
   glTranslatef(x,y,0);
   glBegin(GL_TRIANGLE_FAN);
   glVertex2f(0,0);
   for(angle = 2*PI; angle >= 0; angle -= PI/12) {
      glVertex2f(r*cos(angle),r*sin(angle));
   }
   glEnd();
   glColor3f(0,0,1);
   glBegin(GL_LINES);
   glVertex2f(0,0);
   glVertex2f(r*cos(dir*PI/180),r*sin(dir*PI/180));
   glEnd();
   glPopMatrix();
}

void
displayDials(void)
{
  int i;

  for(i=0;i<NUM_DIALS;i++) {
    glColor3f(0, 1, 0);
    drawCircle(60 + ((i+1)%2) * 100, 60 + (i/2) * 100, 40, dials[NUM_DIALS-1-i]-90);
  }
}

void
displayButtons(void)
{
  int i, n;

  glBegin(GL_QUADS);
  for(i=0,n=0;i<NUM_BUTTONS;i++,n++) {
    switch(n) {
    case 0:
    case 5:
    case 30:
      n++;
    }
    if(buttons[i]) {
      glColor3f(1,0,0);
    } else {
      glColor3f(1,1,1);
    }
    glVertex2f((n%6)*40+250,(n/6)*40+10);
    glVertex2f((n%6)*40+270,(n/6)*40+10);
    glVertex2f((n%6)*40+270,(n/6)*40+30);
    glVertex2f((n%6)*40+250,(n/6)*40+30);
  }
  glEnd();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  displayDials();
  displayButtons();
  glutSwapBuffers();
}

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glScalef(1, -1, 1);
  glTranslatef(0, -h, 0);
}

void
dodial(int dial, int value)
{
  if(dial > 0 && dial <= NUM_DIALS) {
  dials[dial - 1] = value % 360;
  glutPostRedisplay();
  }
}

void
dobutton(int button, int state)
{
  if(button > 0 && button <= NUM_BUTTONS) {
    buttons[button-1] = (state == GLUT_DOWN);
    glutPostRedisplay();
  }
}

int
main(int argc, char **argv)
{
  int width, height;
  glutInit(&argc, argv);
  dials = (int*) calloc(NUM_DIALS, sizeof(int));
  buttons = (int*) calloc(NUM_BUTTONS, sizeof(int));
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  width = 240 + 240;
  height = 100*((NUM_DIALS+1)/2) + 20;
  if(height < 240) height = 240;
  glutInitWindowSize(width, height);
  glutCreateWindow("GLUT dials & buttons");
  glClearColor(0.5, 0.5, 0.5, 1.0);
  glLineWidth(3.0);
  glutDialsFunc(dodial);
  glutButtonBoxFunc(dobutton);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutInitWindowSize(240, 240);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
