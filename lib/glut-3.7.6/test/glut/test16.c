
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Exercise all the GLUT shapes. */

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(1000 * x)
#else
#include <unistd.h>
#endif
#include <GL/glut.h>

GLfloat light_diffuse[] =
{1.0, 0.0, 0.0, 1.0};
GLfloat light_position[] =
{1.0, 1.0, 1.0, 0.0};

void
displayFunc(void)
{
  static int shape = 1;

  fprintf(stderr, " %d", shape);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  switch (shape) {
  case 1:
    glutWireSphere(1.0, 20, 20);
    break;
  case 2:
    glutSolidSphere(1.0, 20, 20);
    break;
  case 3:
    glutWireCone(1.0, 1.0, 20, 20);
    break;
  case 4:
    glutSolidCone(1.0, 1.0, 20, 20);
    break;
  case 5:
    glutWireCube(1.0);
    break;
  case 6:
    glutSolidCube(1.0);
    break;
  case 7:
    glutWireTorus(0.5, 1.0, 15, 15);
    break;
  case 8:
    glutSolidTorus(0.5, 1.0, 15, 15);
    break;
  case 9:
    glutWireDodecahedron();
    break;
  case 10:
    glutSolidDodecahedron();
    break;
  case 11:
    glutWireTeapot(1.0);
    break;
  case 12:
    glutSolidTeapot(1.0);
    break;
  case 13:
    glutWireOctahedron();
    break;
  case 14:
    glutSolidOctahedron();
    break;
  case 15:
    glutWireTetrahedron();
    break;
  case 16:
    glutSolidTetrahedron();
    break;
  case 17:
    glutWireIcosahedron();
    break;
  case 18:
    glutSolidIcosahedron();
    break;
  default:
    printf("\nPASS: test16\n");
    exit(0);
  }
  glutSwapBuffers();
  shape += 1;
  sleep(1);
  glutPostRedisplay();
}

/* ARGSUSED */
void
timefunc(int value)
{
  printf("\nFAIL: test16\n");
  exit(1);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow("test16");
  glutDisplayFunc(displayFunc);

  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHTING);
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

  /* Have a reasonably large timeout since some machines make
     take a while to render all those polygons. */
  glutTimerFunc(35000, timefunc, 1);

  fprintf(stderr, "shape =");
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
