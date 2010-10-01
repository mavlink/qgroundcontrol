
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

int *dials, *buttons;
int numdials, numbuttons;
int dw, bw;

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void
drawCircle(int x, int y, int r, int dir)
{
   float angle;

   glPushMatrix();
   glTranslatef(x,y,0);
   glBegin(GL_TRIANGLE_FAN);
   glVertex2f(0,0);
   for(angle = 2*M_PI; angle >= 0; angle -= M_PI/12) {
      glVertex2f(r*cos(angle),r*sin(angle));
   }
   glEnd();
   glColor3f(0,0,1);
   glBegin(GL_LINES);
   glVertex2f(0,0);
   glVertex2f(r*cos(dir*M_PI/180),r*sin(dir*M_PI/180));
   glEnd();
   glPopMatrix();
}

void
displayDials(void)
{
  int i;

  glClear(GL_COLOR_BUFFER_BIT);
  for(i=0;i<numdials;i++) {
    glColor3f(0, 1, 0);
    drawCircle(100 + (i%2) * 100, 100 + (i/2) * 100, 40, -dials[i]+90);
  }
  glutSwapBuffers();
}

void
displayButtons(void)
{
  int i, n;

  glClear(GL_COLOR_BUFFER_BIT);
  glBegin(GL_QUADS);
  for(i=0,n=0;i<32;i++,n++) {
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
    glVertex2f((n%6)*40+10,(n/6)*40+10);
    glVertex2f((n%6)*40+30,(n/6)*40+10);
    glVertex2f((n%6)*40+30,(n/6)*40+30);
    glVertex2f((n%6)*40+10,(n/6)*40+30);
  }
  glEnd();
  glutSwapBuffers();
}

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  if(glutGetWindow() == bw) {
     glScalef(1, -1, 1);
     glTranslatef(0, -h, 0);
  }
  glMatrixMode(GL_MODELVIEW);
}

void
dodial(int dial, int value)
{
  dials[dial - 1] = value % 360;
  glutPostWindowRedisplay(dw);
}

void
dobutton(int button, int state)
{
  if(button <= numbuttons) {
    buttons[button-1] = (state == GLUT_DOWN);
    glutPostWindowRedisplay(bw);
  }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  numdials = glutDeviceGet(GLUT_NUM_DIALS);
  if(numdials <= 0) {
     fprintf(stderr, "dials: No dials available\n");
     exit(1);
  }
  numbuttons = glutDeviceGet(GLUT_NUM_BUTTON_BOX_BUTTONS);
  dials = (int*) calloc(numdials, sizeof(int));
  buttons = (int*) calloc(numbuttons, sizeof(int));
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(300, ((numdials+1)/2) * 100 + 100);
  dw = glutCreateWindow("GLUT dials");
  glClearColor(0.5, 0.5, 0.5, 1.0);
  glLineWidth(3.0);
  glutDialsFunc(dodial);
  glutButtonBoxFunc(dobutton);
  glutDisplayFunc(displayDials);
  glutReshapeFunc(reshape);
  glutInitWindowSize(240, 240);
  bw = glutCreateWindow("GLUT button box");
  glClearColor(0.5, 0.5, 0.5, 1.0);
  glutDisplayFunc(displayButtons);
  glutReshapeFunc(reshape);
  glutDialsFunc(dodial);
  glutButtonBoxFunc(dobutton);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
