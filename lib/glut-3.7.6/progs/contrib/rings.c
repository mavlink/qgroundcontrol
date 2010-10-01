/* rings.c
 *
 * To compile: cc -o rings rings.c -lGL -lGLU -lX11 -lglut -lXmu -lm
 *
 * Usage: rings
 *
 * Homework 4, Part 1: perspective, hierarchical coords, moving eye pos.
 *
 * Do a slow zoom on a bunch of rings (ala Superman III?)
 *
 * Philip Winston - 2/21/95
 * pwinston@hmc.edu
 * http://www.cs.hmc.edu/people/pwinston
 *
 */

#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {MENU_STARTOVER, MENU_ZOOM_OUT, MENU_STOP_RINGS, MENU_STOP_FADE, 
              MENU_START_RINGS, MENU_START_FADE, MENU_QUIT} MenuChoices;

typedef enum {NOTALLOWED, CONE, TORUS, INNERMAT, OUTTERMAT} DisplayLists;

#define STEPS 30

int Fade = 1;   /* Start moving out */

float Axis = 0, AxisInc = (2.0 * M_PI / STEPS);

GLfloat InnerRad, OutterRad, Tilt, Trans, TransCone, Dist;

  /* mainly computes the translation amount as a function of the
     tilt angle and torus radii */
void myInit(void)
{
  float sinoftilt;

  InnerRad = 0.70;
  OutterRad = 5.0;
  Tilt = 15;
  Dist = 10;

  sinoftilt = sin(Tilt * M_PI*2/360);

  Trans = (2*OutterRad + InnerRad) * sinoftilt + InnerRad +
          ((1 - sinoftilt) * InnerRad) - (InnerRad * 1/10);

  TransCone = Trans + (OutterRad * sinoftilt + InnerRad);
}

  /* I used code from the book's accnot.c as a starting point for lighting.
     I have one positional light in center, then one directional */
void myglInit(void)
{
  GLfloat light0_position[] = { 1.0, 0.2, 1.0, 0.0 };
  GLfloat light1_position[] = { 0.0, 0.0, 0.0, 1.0 };
  GLfloat light1_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light1_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat lm_ambient[] = { 0.2, 0.2, 0.2, 1.0 };

  glEnable(GL_NORMALIZE);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
  glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
  glLightfv(GL_LIGHT1, GL_DIFFUSE,  light1_diffuse);
  glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
  glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.2);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lm_ambient);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);

  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);

  glFlush();
} 

void myreshape(GLsizei w, GLsizei h)
{
  glViewport(0,0,w,h);
  glFlush();
}

  /* setup display lists to change material for inner/outter rings and
     to draw a single torus or cone */
void MakeDisplayLists(void)
{
  GLfloat cone_diffuse[] = { 0.0, 0.7, 0.7, 1.0 };
  GLfloat mat1_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat mat2_ambient[] = { 0.0, 0.0, 0.0, 0.0 };
  GLfloat torus1_diffuse[] = { 0.7, 0.7, 0.0, 1.0 };
  GLfloat torus2_diffuse[] = { 0.3, 0.0, 0.0, 1.0 };
  GLfloat mat1_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat mat2_specular[] = { 0.5, 0.5, 0.5, 1.0 };

  glNewList(INNERMAT, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat1_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 50.0);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, torus1_diffuse);    
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat1_ambient);
  glEndList();

  glNewList(OUTTERMAT, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat2_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 25.0);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, torus2_diffuse);    
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat2_ambient);
  glEndList();

  glNewList(CONE, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, cone_diffuse);    
    glPushMatrix();
      glTranslatef(0, -TransCone, 0);
      glRotatef(90, 1, 0, 0);
      glutSolidCone(OutterRad, 10, 8, 8);
    glPopMatrix();
  glEndList();

  glNewList(TORUS, GL_COMPILE);
    glPushMatrix();
      glRotatef(90, 1, 0, 0);
      glutSolidTorus(InnerRad, OutterRad, 15, 25);
    glPopMatrix();
  glEndList();
}
  
  /* Draw three rings, rotated and translate so they look cool */
void DrawRings(float axis)
{
  GLfloat x = sin(axis), y = cos(axis);

  glPushMatrix();
    glTranslatef(0, Trans, 0);
    glRotatef(Tilt, x, 0, y);
    glCallList(TORUS);  
  glPopMatrix();

  glPushMatrix();
    glRotatef(-Tilt, x, 0, y);
    glCallList(TORUS);
  glPopMatrix();

  glPushMatrix();
    glTranslatef(0, -Trans, 0);
    glRotatef(Tilt, x, 0, y);
    glCallList(TORUS);
  glPopMatrix();
}

  /* Draw the inner thing, then glScale and draw 3 huge rings */
void mydisplay(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1, 1, -1, 1, 10, 1000); 
  gluLookAt(0, 0, Dist, 0, 0, 0, 0, 1, 0);

  glMatrixMode(GL_MODELVIEW);

  glCallList(INNERMAT);
  DrawRings(Axis);
  glCallList(CONE);

  glCallList(OUTTERMAT);
  glPushMatrix();
    glScalef(10, 10, 10);
    DrawRings(Axis/3);
  glPopMatrix();

  glutSwapBuffers();
  glFlush();
}

  /* rotate the axis and adjust position if nec. */
void myidle(void)
{ 
  Axis += AxisInc;

  if (Dist < 15 && Fade)    /* start slow */
    Dist += 0.1;
  else if (Dist < 800 && Fade)   /* don't go back too far */
    Dist *= 1.005;  

  mydisplay();
}

  /* nothing fancy */
void handlemenu(int value)
{
  switch (value) {
    case MENU_STARTOVER:
      Dist = 10; Axis = 0; Fade = 1;
      AxisInc = (2.0 * M_PI / STEPS);
      glutChangeToMenuEntry(3, "Stop rings", MENU_STOP_RINGS);
      glutChangeToMenuEntry(4, "Stop fade", MENU_STOP_FADE);
      break;
    case MENU_ZOOM_OUT:
      Dist = 800;
      break;
    case MENU_STOP_RINGS:
      AxisInc = 0;
      glutChangeToMenuEntry(3, "Start rings", MENU_START_RINGS);
      break;
    case MENU_START_RINGS:
      AxisInc = (2.0 * M_PI / STEPS);
      glutChangeToMenuEntry(3, "Stop rings", MENU_STOP_RINGS);
      break;
    case MENU_STOP_FADE:
      Fade = 0;
      glutChangeToMenuEntry(4, "Start fade", MENU_START_FADE);
      break;
    case MENU_START_FADE:
      Fade = 1;
      glutChangeToMenuEntry(4, "Stop fade", MENU_STOP_FADE);
      break;
    case MENU_QUIT:
      exit(0);
      break;
    }
}

void MenuInit(void)
{
  glutCreateMenu(handlemenu);
  glutAddMenuEntry("Start Over", MENU_STARTOVER);
  glutAddMenuEntry("Zoom Out", MENU_ZOOM_OUT);
  glutAddMenuEntry("Stop rings", MENU_STOP_RINGS);
  glutAddMenuEntry("Stop fade", MENU_STOP_FADE);
  glutAddMenuEntry("Quit", MENU_QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void
vis(int visible)
{
  if (visible == GLUT_VISIBLE) {
      glutIdleFunc(myidle);
  } else {
      glutIdleFunc(NULL);
  }
}

int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("rings");

  myInit();
  myglInit(); 

  MakeDisplayLists();
  MenuInit();

  glutReshapeFunc(myreshape);
  glutDisplayFunc(mydisplay);
  glutVisibilityFunc(vis);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
