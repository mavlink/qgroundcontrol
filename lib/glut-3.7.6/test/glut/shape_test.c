
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <GL/glut.h>

float w, h;

GLfloat light_diffuse[] =
{1.0, 1.0, 1.0, 1.0};
GLfloat light_position[] =
{1.0, 1.0, 1.0, 0.0};
GLUquadricObj *qobj;

void
reshape(int nw, int nh)
{
  w = nw;
  h = nh;
}

void
render(int shape)
{
  switch (shape) {
  case 1:
    glPushMatrix();
    glScalef(1.2, 1.2, 1.2);
    glutWireSphere(1.0, 20, 20);
    glPopMatrix();
    break;
  case 10:
    glPushMatrix();
    glScalef(1.2, 1.2, 1.2);
    glEnable(GL_LIGHTING);
    glutSolidSphere(1.0, 20, 20);
    glDisable(GL_LIGHTING);
    glPopMatrix();
    break;
  case 2:
    glPushMatrix();
    glRotatef(-90, 1.0, 0.0, 0.0);
    glutWireCone(1.0, 1.3, 20, 20);
    glPopMatrix();
    break;
  case 11:
    glPushMatrix();
    glRotatef(-90, 1.0, 0.0, 0.0);
    glEnable(GL_LIGHTING);
    glutSolidCone(1.0, 1.3, 20, 20);
    glDisable(GL_LIGHTING);
    glPopMatrix();
    break;
  case 3:
    glPushMatrix();
    glRotatef(-20, 0.0, 0.0, 1.0);
    glScalef(1.8, 1.8, 1.8);
    glutWireCube(1.0);
    glPopMatrix();
    break;
  case 12:
    glPushMatrix();
    glRotatef(-20, 0.0, 0.0, 1.0);
    glScalef(1.8, 1.8, 1.8);
    glEnable(GL_LIGHTING);
    glutSolidCube(1.0);
    glDisable(GL_LIGHTING);
    glPopMatrix();
    break;
  case 4:
    glPushMatrix();
    glScalef(0.9, 0.9, 0.9);
    glutWireTorus(0.5, 1.0, 15, 15);
    glPopMatrix();
    break;
  case 13:
    glPushMatrix();
    glScalef(0.9, 0.9, 0.9);
    glEnable(GL_LIGHTING);
    glutSolidTorus(0.5, 1.0, 15, 15);
    glDisable(GL_LIGHTING);
    glPopMatrix();
    break;
  case 5:
    glPushMatrix();
    glScalef(0.8, 0.8, 0.8);
    glutWireDodecahedron();
    glPopMatrix();
    break;
  case 14:
    glPushMatrix();
    glScalef(0.8, 0.8, 0.8);
    glEnable(GL_LIGHTING);
    glutSolidDodecahedron();
    glDisable(GL_LIGHTING);
    glPopMatrix();
    break;
  case 6:
    glPushMatrix();
    glScalef(0.9, 0.9, 0.9);
    glutWireTeapot(1.0);
    glPopMatrix();
    break;
  case 15:
    glPushMatrix();
    glScalef(0.9, 0.9, 0.9);
    glEnable(GL_LIGHTING);
    glutSolidTeapot(1.0);
    glDisable(GL_LIGHTING);
    glPopMatrix();
    break;
  case 7:
    glutWireOctahedron();
    break;
  case 16:
    glEnable(GL_LIGHTING);
    glutSolidOctahedron();
    glDisable(GL_LIGHTING);
    break;
  case 8:
    glPushMatrix();
    glScalef(1.2, 1.2, 1.2);
    glutWireTetrahedron();
    glPopMatrix();
    break;
  case 17:
    glPushMatrix();
    glScalef(1.2, 1.2, 1.2);
    glEnable(GL_LIGHTING);
    glutSolidTetrahedron();
    glDisable(GL_LIGHTING);
    glPopMatrix();
    break;
  case 9:
    glutWireIcosahedron();
    break;
  case 18:
    glEnable(GL_LIGHTING);
    glutSolidIcosahedron();
    glDisable(GL_LIGHTING);
    break;
  }
}

void
display(void)
{
  int i, j;

  glViewport(0, 0, w, h);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  for (j = 0; j < 6; j++) {
    for (i = 0; i < 3; i++) {
      glViewport(w / 3 * i, h / 6 * j, w / 3, h / 6);
      render(18 - (j * 3 + (2 - i)));
    }
  }
  glFlush();
}

int
main(int argc, char **argv)
{
  glutInitWindowSize(475, 950);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGB);
  glutCreateWindow("GLUT geometric shapes");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);

  glClearColor(1.0, 1.0, 1.0, 1.0);
  glColor3f(0.0, 0.0, 0.0);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 22.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in postivie Y direction */
  glTranslatef(0.0, 0.0, -3.0);
  glRotatef(25, 1.0, 0.0, 0.0);
  glRotatef(5, 0.0, 1.0, 0.0);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
