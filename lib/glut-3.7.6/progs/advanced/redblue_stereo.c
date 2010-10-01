
/* redblue_stereo.c - demo of stereo for red/blue filter stereo glasses */

/* by Walter Vannini (walterv@jps.net, waltervannini@hotmail.com) */

/* In stereo mode, the object is drawn in red for the left eye
   and blue for the right eye.  Viewing the scene with red/blue
   filter stereo glasses should give a sense of stereo 3D.
   glColorMask is used to control update of the red and blue
   channel.  glFrustum is used to setup two different view frustums
   for each eye based on eye separation. */

/* Copyright (c) Walter Vannini, 1998.  */

/* This program is freely distributable without licensing fees and is
   provided without guarantee or warrantee expressed or implied. This
   program is -not- in the public domain. */

#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void init(void);
void KeyboardFunc(unsigned char key, int x, int y);
void MenuFunc(int value);
void IdleFunc(void);
void ReshapeFunc(int w, int h);
void DisplayFunc(void);


struct ProgramState
{
 int w;
 int h;
 GLdouble RotationY;
 double eye;
 double zscreen;
 double znear;
 double zfar;
 double RotationIncrement;
 int solidmode;
};
struct ProgramState ps;

const double PIXELS_PER_INCH = 100.0;

void init(void)
{
 GLfloat mat_ambient[] = {0.2,0.0,0.2,1.0} ;
 GLfloat mat_diffuse[] = {0.7,0.0,0.7,1.0} ;
 GLfloat mat_specular[] = {0.1,0.0,0.1,1.0} ;
 GLfloat mat_shininess[]={20.0};

 GLfloat light_position[]={0.0,5.0,20.0,1.0};

 GLfloat light_ambient0[]= {1.0,0.0,0.0,1.0};
 GLfloat light_diffuse0[]= {1.0,0.0,0.0,1.0};
 GLfloat light_specular0[]={1.0,0.0,0.0,1.0};

 GLfloat light_ambient1[]= {0.0,0.0,1.0,1.0};
 GLfloat light_diffuse1[]= {0.0,0.0,1.0,1.0};
 GLfloat light_specular1[]={0.0,0.0,1.0,1.0};

 glDisable(GL_DITHER);
 glClearColor(0.0, 0.0, 0.0, 1.0);
 glShadeModel(GL_SMOOTH);
 glEnable(GL_DEPTH_TEST);
 glEnable(GL_NORMALIZE);
 glEnable(GL_CULL_FACE);
 glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);


 glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
 glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
 glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
 glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

 glLightfv(GL_LIGHT0, GL_POSITION, light_position);
 glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient0);
 glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse0);
 glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular0);

 glLightfv(GL_LIGHT1, GL_POSITION, light_position);
 glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient1);
 glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse1);
 glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular1);
 glEnable(GL_LIGHTING);

 ps.eye=0.80;
 ps.zscreen = 10.0;
 ps.znear = 7.0;
 ps.zfar = 13.0;
 ps.RotationY = 0.0;
 ps.RotationIncrement = 4.0;
 ps.solidmode = 1;
}

void KeyboardFunc(unsigned char key, int x, int y)
{
 switch(key)
 {
 case 27:  /* escape */
 case 'q':
 case 'Q':
  exit(0);
  break;
 case 's': /* stereo */
  ps.eye = 0.80;
  break;
 case '1':
  ps.solidmode = 1;
  glFrontFace(GL_CCW);
  break;
 case '2':
  ps.solidmode = 2;
  glFrontFace(GL_CCW);
  break;
 case '3':
  ps.solidmode = 3;
  /* The teapot polygons face the wrong way.  Sigh. */
  glFrontFace(GL_CW);
  break;
 case '4':
  ps.solidmode = 4;
  glFrontFace(GL_CCW);
  break;
 case 'm': /* mono */
  ps.eye = 0.0;
  break;
 }
}

void MenuFunc(int value)
{
  KeyboardFunc((unsigned char) value, 0, 0);
}

void IdleFunc(void)
{
 ps.RotationY += ps.RotationIncrement;
 glutPostRedisplay();
}

void ReshapeFunc(int w, int h)
{
 glViewport(0,0,  w,  h);
 ps.w = w;
 ps.h = h;
}

void DisplayFunc(void)
{
 double xfactor=1.0, yfactor=1.0;
 double Eye =0.0;
 int i;

 if(ps.w < ps.h)
 {
  xfactor = 1.0;
  yfactor = ps.h/ps.w;
 }
 else if(ps.h < ps.w)
 {
  xfactor = ps.w/ps.h;
  yfactor = 1.0;
 }

 glClear(GL_COLOR_BUFFER_BIT);
 for(i=0;i<2;i++)
 {
  glEnable(GL_LIGHT0 + i);
  glClear(GL_DEPTH_BUFFER_BIT);
  if(i==0) /* left eye - RED */
  {
   Eye = ps.eye;
   glColorMask(GL_TRUE,GL_FALSE,GL_FALSE,GL_TRUE);
  }
  else /* if(i==1) right eye - BLUE */
  {
   Eye = -ps.eye;
   glColorMask(GL_FALSE,GL_FALSE,GL_TRUE,GL_TRUE);
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(
   (-(ps.w/(2.0*PIXELS_PER_INCH))+Eye) *(ps.znear/ps.zscreen)*xfactor,
   (ps.w/(2.0*PIXELS_PER_INCH)+Eye)    *(ps.znear/ps.zscreen)*xfactor,
   -(ps.h/(2.0*PIXELS_PER_INCH))*(ps.znear/ps.zscreen)*yfactor,
   (ps.h/(2.0*PIXELS_PER_INCH))*(ps.znear/ps.zscreen)*yfactor,
   ps.znear, ps.zfar);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(Eye,0.0,0.0);
  glTranslated(0,0,-ps.zscreen);

  switch(ps.solidmode)
  {
  case 1:
   {
    glTranslated(cos(ps.RotationY*M_PI/180.0),
     0, -sin(ps.RotationY*M_PI/180.0) );
    glRotated(ps.RotationY, 0.0,0.1,0.0);
    glPushMatrix();
     glScalef(0.5,0.5,0.5);
     glutSolidDodecahedron();
    glPopMatrix();
    break;
   }
  case 2:
   {
    glTranslated(cos(ps.RotationY*M_PI/180.0),
     0, -sin(ps.RotationY*M_PI/180.0) );
    glRotated(ps.RotationY, 0.0,0.1,0.0);
    glutSolidIcosahedron();
    break;
   }
  case 3:
   {
    glRotated(60.0, 1 , 0, 0);
    glTranslated(cos(ps.RotationY*M_PI/180.0),
     0, -sin(ps.RotationY*M_PI/180.0) );
    glRotated(ps.RotationY, 0.0,0.1,0.0);
    glutSolidTeapot(1.0);
    break;
   }
  case 4:
   {
    double SunRadius = 0.2;
    double SunToEarth = 1.0;
    double EarthRadius = 0.15;

    glutSolidSphere(SunRadius, 20, 16);   /* draw sun */
    glRotated(60.0, 1 , 0, 0);
    glPushMatrix();
     glRotated(90.0, 1 , 0, 0);
     glutSolidTorus(0.01,SunToEarth, 10,50);
    glPopMatrix();
    glRotated(ps.RotationY, 0.0,0.1,0.0);
    glTranslatef (SunToEarth, 0.0, 0.0);
    glutSolidSphere(EarthRadius, 10, 8);    /* draw earth */
    break;
   }
  }
  glDisable(GL_LIGHT0 + i);
 }
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
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
int main(int argc, char **argv)
{
 glutInit(&argc, argv);
 glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA );
 ps.w = 512;
 ps.h = 512;
 glutInitWindowSize(ps.w, ps.h);
 glutInitWindowPosition(100,100);

 glutCreateWindow(argv[0]);
 init();
 glutVisibilityFunc(VisibilityFunc); 
 glutDisplayFunc(DisplayFunc);
 glutReshapeFunc(ReshapeFunc);
 glutKeyboardFunc(KeyboardFunc);
 glutCreateMenu(MenuFunc);
 glutAddMenuEntry("Stereo", 's');
 glutAddMenuEntry("Mono", 'm');
 glutAddMenuEntry("Dodecahedron", '1');
 glutAddMenuEntry("Icosahedron", '2');
 glutAddMenuEntry("Teapot", '3');
 glutAddMenuEntry("Solar system", '4');
 glutAttachMenu(GLUT_RIGHT_BUTTON);
 glutMainLoop();
 return 0;
}

