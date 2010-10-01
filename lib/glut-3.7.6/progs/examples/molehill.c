
/* Copyright (c) Mark J. Kilgard, 1995. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* molehill uses the GLU NURBS routines to draw some nice surfaces. */

#include <GL/glut.h>

GLfloat mat_red_diffuse[] = { 0.7, 0.0, 0.1, 1.0 };
GLfloat mat_green_diffuse[] = { 0.0, 0.7, 0.1, 1.0 };
GLfloat mat_blue_diffuse[] = { 0.0, 0.1, 0.7, 1.0 };
GLfloat mat_yellow_diffuse[] = { 0.7, 0.8, 0.1, 1.0 };
GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_shininess[] = { 100.0 };
GLfloat knots[8] = { 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 };
GLfloat pts1[4][4][3], pts2[4][4][3];
GLfloat pts3[4][4][3], pts4[4][4][3];
GLUnurbsObj *nurb;
int u, v;

static void 
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glCallList(1);
  glFlush();
}

int 
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutCreateWindow("molehill");
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_AUTO_NORMAL);
  glEnable(GL_NORMALIZE);
  nurb = gluNewNurbsRenderer();
  gluNurbsProperty(nurb, GLU_SAMPLING_TOLERANCE, 25.0);
  gluNurbsProperty(nurb, GLU_DISPLAY_MODE, GLU_FILL);

  /* Build control points for NURBS mole hills. */
  for(u=0; u<4; u++) {
    for(v=0; v<4; v++) {
      /* Red. */
      pts1[u][v][0] = 2.0*((GLfloat)u);
      pts1[u][v][1] = 2.0*((GLfloat)v);
      if((u==1 || u == 2) && (v == 1 || v == 2))
	/* Stretch up middle. */
	pts1[u][v][2] = 6.0;
      else
	pts1[u][v][2] = 0.0;

      /* Green. */
      pts2[u][v][0] = 2.0*((GLfloat)u - 3.0);
      pts2[u][v][1] = 2.0*((GLfloat)v - 3.0);
      if((u==1 || u == 2) && (v == 1 || v == 2))
	if(u == 1 && v == 1) 
	  /* Pull hard on single middle square. */
	  pts2[u][v][2] = 15.0;
	else
	  /* Push down on other middle squares. */
	  pts2[u][v][2] = -2.0;
      else
	pts2[u][v][2] = 0.0;

      /* Blue. */
      pts3[u][v][0] = 2.0*((GLfloat)u - 3.0);
      pts3[u][v][1] = 2.0*((GLfloat)v);
      if((u==1 || u == 2) && (v == 1 || v == 2))
	if(u == 1 && v == 2)
	  /* Pull up on single middple square. */
	  pts3[u][v][2] = 11.0;
	else
	  /* Pull up slightly on other middle squares. */
	  pts3[u][v][2] = 2.0;
      else
	pts3[u][v][2] = 0.0;

      /* Yellow. */
      pts4[u][v][0] = 2.0*((GLfloat)u);
      pts4[u][v][1] = 2.0*((GLfloat)v - 3.0);
      if((u==1 || u == 2 || u == 3) && (v == 1 || v == 2))
	if(v == 1) 
	  /* Push down front middle and right squares. */
	  pts4[u][v][2] = -2.0;
	else
	  /* Pull up back middle and right squares. */
	  pts4[u][v][2] = 5.0;
      else
	pts4[u][v][2] = 0.0;
    }
  }
  /* Stretch up red's far right corner. */
  pts1[3][3][2] = 6;
  /* Pull down green's near left corner a little. */
  pts2[0][0][2] = -2;
  /* Turn up meeting of four corners. */
  pts1[0][0][2] = 1;
  pts2[3][3][2] = 1;
  pts3[3][0][2] = 1;
  pts4[0][3][2] = 1;

  glMatrixMode(GL_PROJECTION);
  gluPerspective(55.0, 1.0, 2.0, 24.0);
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(0.0, 0.0, -15.0);
  glRotatef(330.0, 1.0, 0.0, 0.0);

  glNewList(1, GL_COMPILE);
    /* Render red hill. */
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_red_diffuse);
    gluBeginSurface(nurb);
      gluNurbsSurface(nurb, 8, knots, 8, knots,
        4 * 3, 3, &pts1[0][0][0],
        4, 4, GL_MAP2_VERTEX_3);
    gluEndSurface(nurb);

    /* Render green hill. */
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_green_diffuse);
    gluBeginSurface(nurb);
      gluNurbsSurface(nurb, 8, knots, 8, knots,
        4 * 3, 3, &pts2[0][0][0],
        4, 4, GL_MAP2_VERTEX_3);
    gluEndSurface(nurb);

    /* Render blue hill. */
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_blue_diffuse);
    gluBeginSurface(nurb);
      gluNurbsSurface(nurb, 8, knots, 8, knots,
        4 * 3, 3, &pts3[0][0][0],
        4, 4, GL_MAP2_VERTEX_3);
    gluEndSurface(nurb);

    /* Render yellow hill. */
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_yellow_diffuse);
    gluBeginSurface(nurb);
      gluNurbsSurface(nurb, 8, knots, 8, knots,
        4 * 3, 3, &pts4[0][0][0],
        4, 4, GL_MAP2_VERTEX_3);
    gluEndSurface(nurb);
  glEndList();

  glutDisplayFunc(display);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
