
/* resolution.c */

/* by Walter Vannini (walterv@jps.net, waltervannini@hotmail.com) */

/* Copyright (c) Walter Vannini, 1998.  */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or implied. This
   program is -not- in the public domain. */

/* This example demonstrates how to use glCopyPixels with glPixelZoom
   to zoom up small renderings to fill more pixels. */

#include <GL/glut.h>
#include <stdlib.h>

void init(void);
void KeyboardFunc(unsigned char key, int x, int y);
void MenuFunc(int value);
void IdleFunc(void);
void ReshapeFunc(int w, int h);
void DisplayFunc(void);

struct ProgramState
{
 double w;
 double h;
 double ResolutionX;
 double ResolutionY;
 int shoulder;
 int elbow;
 int wrist;
 double Rotation;
 double RotationIncrement;
};
struct ProgramState ps;

void init(void)
{
 glShadeModel (GL_FLAT);
 glDisable(GL_DITHER);
 glClearColor(0.0, 0.0, 0.0, 1.0);

 ps.Rotation = 0.0;
 ps.RotationIncrement = 4.0;
 ps.ResolutionX = 1.0;
 ps.ResolutionY = 1.0;
 ps.shoulder = 0.0;
 ps.elbow = 0.0;
 ps.wrist = 0.0;
}

void KeyboardFunc (unsigned char key, int x, int y)
{
 switch (key)
 {
 case '1':
  ps.ResolutionX = 1;
  ps.ResolutionY = 1;
  glutPostRedisplay();
  break;
 case '2':
  ps.ResolutionX = 2;
  ps.ResolutionY = 2;
  glutPostRedisplay();
  break;
 case '3':
  ps.ResolutionX = 3;
  ps.ResolutionY = 3;
  glutPostRedisplay();
  break;
 case '4':
  ps.ResolutionX = 4;
  ps.ResolutionY = 4;
  glutPostRedisplay();
  break;
 case '5':
  ps.ResolutionX = 5;
  ps.ResolutionY = 5;
  glutPostRedisplay();
  break;
 case '6':
  ps.ResolutionX = 6;
  ps.ResolutionY = 6;
  glutPostRedisplay();
  break;
 case '7':
  ps.ResolutionX = 7;
  ps.ResolutionY = 7;
  glutPostRedisplay();
  break;
 case '8':
  ps.ResolutionX = 8;
  ps.ResolutionY = 8;
  glutPostRedisplay();
  break;
 case '9':
  ps.ResolutionX = 9;
  ps.ResolutionY = 9;
  glutPostRedisplay();
  break;
 case '0':
  ps.ResolutionX = 10;
  ps.ResolutionY = 10;
  glutPostRedisplay();
  break;
 case '-':
  ps.ResolutionX = 16;
  ps.ResolutionY = 16;
  glutPostRedisplay();
  break;
 case '=':
  ps.ResolutionX = 32;
  ps.ResolutionY = 32;
  glutPostRedisplay();
  break;
 case '\\':
  ps.ResolutionX = 64;
  ps.ResolutionY = 64;
  glutPostRedisplay();
  break;
 case 'o':
  ps.ResolutionX = 1;
  ps.ResolutionY = 8;
  glutPostRedisplay();
  break;
 case 'e':
  ps.ResolutionX = 8;
  ps.ResolutionY = 1;
  glutPostRedisplay();
  break;
 case 'q':
 case 'Q':
 case 27:
  exit(0);
  break;
 default:
  break;
 }
}

void MenuFunc(int value)
{
 KeyboardFunc((unsigned char) value, 0, 0);
}

void IdleFunc(void)
{
 ps.Rotation += ps.RotationIncrement;
 ps.shoulder = ps.Rotation;
 ps.elbow = ps.Rotation*0.63432;
 ps.wrist = ps.Rotation*0.4313543;
 glutPostRedisplay();
}

void ReshapeFunc (int w, int h)
{
 ps.w =w;
 ps.h = h;
 glMatrixMode (GL_PROJECTION);
 glLoadIdentity ();
 gluPerspective(65.0, ps.w/ps.h, 1.0, 20.0);
 glMatrixMode(GL_MODELVIEW);
 glLoadIdentity();
 glTranslatef (0.0, 0.0, -5.0);
}

void DisplayFunc(void)
{
 double ForearmLength= 1.2;
 double UpperarmLength =1.0;
 double HandLength = 0.5;
 glViewport (0.0, 0.0, (GLsizei) (ps.w/ps.ResolutionX), (GLsizei) (ps.h/ps.ResolutionY));
 glMatrixMode(GL_MODELVIEW);

 glClear (GL_COLOR_BUFFER_BIT);
 glPushMatrix();
 {
  glRotatef (ps.shoulder, 0.0, 0.0, 1.0);
  glTranslatef (0.5*UpperarmLength, 0.0, 0.0);
  glPushMatrix();
  {
   glScalef (UpperarmLength, 0.6, 1.0);
   glutWireCube (1.0);
  }
  glPopMatrix();
  glTranslatef (0.5*UpperarmLength, 0.0, 0.0);

  glRotatef (ps.elbow, 0.0, 0.0, 1.0);
  glTranslatef (0.5*ForearmLength, 0.0, 0.0);
  glPushMatrix();
  {
   glScalef (ForearmLength, 0.4, 1.0);
   glutWireCube (1.0);
  }
  glPopMatrix();
  glTranslatef (0.5*ForearmLength, 0.0, 0.0);

  glRotatef (ps.wrist, 0.0, 0.0, 1.0);
  glTranslatef (0.5*HandLength, 0.0, 0.0);
  glPushMatrix();
  {
   glScalef (HandLength, 0.2, 1.0);
   glutWireCube (1.0);
  }
  glPopMatrix();
 }
 glPopMatrix();

 glPixelZoom(ps.ResolutionX, ps.ResolutionY);
 glCopyPixels(0.0,0.0,
  (GLsizei) (ps.w/ps.ResolutionX), (GLsizei) (ps.h/ps.ResolutionY), GL_COLOR);

 glutSwapBuffers();
}

void
VisibilityFunc(int vis)
{
  if (vis == GLUT_VISIBLE) {
    glutIdleFunc(IdleFunc);
  } else {
    glutIdleFunc(NULL);
  }
}

int main(int argc, char** argv)
{
 glutInit(&argc, argv);
 glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
 ps.h=500;
 ps.w=500;
 glutInitWindowSize ((int) ps.w, (int) ps.h);
 glutInitWindowPosition (100, 100);
 glutCreateWindow (argv[0]);

 init ();
 glutVisibilityFunc(VisibilityFunc);
 glutDisplayFunc(DisplayFunc);
 glutReshapeFunc(ReshapeFunc);
 glutKeyboardFunc(KeyboardFunc);
 glutCreateMenu(MenuFunc);
 glutAddMenuEntry("resolution 1x1", '1');
 glutAddMenuEntry("resolution 2x2", '2');
 glutAddMenuEntry("resolution 3x3", '3');
 glutAddMenuEntry("resolution 4x4", '4');
 glutAddMenuEntry("resolution 5x5", '5');
 glutAddMenuEntry("resolution 6x6", '6');
 glutAddMenuEntry("resolution 7x7", '7');
 glutAddMenuEntry("resolution 8x8", '8');
 glutAddMenuEntry("resolution 9x9", '9');
 glutAddMenuEntry("resolution 10x10", '0');
 glutAddMenuEntry("resolution 16x16", '-');
 glutAddMenuEntry("resolution 32x32", '=');
 glutAddMenuEntry("resolution 64x64", '\\');
 glutAddMenuEntry("resolution 1x8", 'o');
 glutAddMenuEntry("resolution 8x1", 'e');
 glutAttachMenu(GLUT_RIGHT_BUTTON);

 glutMainLoop();
 return 0;
}

