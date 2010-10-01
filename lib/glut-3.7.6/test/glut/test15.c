
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This test makes sure that if you post a redisplay within a
   display callback, another display callback will be
   generated. I believe this is useful for progressive
   refinement of an image.  Draw it once at a coarse
   tesselation to get something on the screen; then redraw at a
   higher level of tesselation.  Pre-GLUT 2.3 fails this test. */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

GLfloat light_diffuse[] =
{1.0, 0.0, 0.0, 1.0};
GLfloat light_position[] =
{1.0, 1.0, 1.0, 0.0};
GLUquadricObj *qobj;

void
displayFunc(void)
{
  static int tesselation = 3;

  fprintf(stderr, " %d", tesselation);
  if (tesselation > 23) {
    printf("\nPASS: test15\n");
    exit(0);
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gluSphere(qobj, /* radius */ 1.0,
    /* slices */ tesselation, /* stacks */ tesselation);
  glutSwapBuffers();
  tesselation += 1;
  glutPostRedisplay();
}

/* ARGSUSED */
void
timefunc(int value)
{
  printf("\nFAIL: test15\n");
  exit(1);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow("test15");
  glutDisplayFunc(displayFunc);

  qobj = gluNewQuadric();
  gluQuadricDrawStyle(qobj, GLU_FILL);
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
  glTranslatef(0.0, 0.0, -1.0);

  /* Have a reasonably large timeout since some machines make
     take a while to render all those polygons. */
  glutTimerFunc(15000, timefunc, 1);

  fprintf(stderr, "tesselations =");
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
