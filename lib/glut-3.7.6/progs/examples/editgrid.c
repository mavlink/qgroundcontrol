
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* editgrid demonstrates how a simple 2nd order grid mesh or a more
   complex 4th order grid mesh can rendered with OpenGL evaluators.
   The control points for either grid can be interactively moved in 2D
   by selecting and moving with the left mouse button.  Antialising can
   also be enabled from the pop-up menu. */

/* Compile: cc -o editgrid editgrid.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

int antialiasing = 0;
int gridSize = 20;
GLuint selectedPoint = (GLuint) ~0;
int winWidth, winHeight;
GLuint selectBuffer[64];
GLdouble modelMatrix[16], projMatrix[16];
GLint viewport[4];

/* Simple 2nd order initial grid.  4 points (2 by 2). */
GLfloat grid2x2[2][2][3] =
{
  {
    {-2.0, -2.0, 0.0},
    {2.0, -2.0, 0.0}},
  {
    {-2.0, 2.0, 0.0},
    {2.0, 2.0, 0.0}}
};

/* More complex 4nd order initial grid.  16 points (4 by 4). */
GLfloat grid4x4[4][4][3] =
{
  {
    {-2.0, -2.0, 0.0},
    {-0.5, -2.0, 0.0},
    {0.5, -2.0, 0.0},
    {2.0, -2.0, 0.0}},
  {
    {-2.0, -0.5, 0.0},
    {-0.5, -0.5, 0.0},
    {0.5, -0.5, 0.0},
    {2.0, -0.5, 0.0}},
  {
    {-2.0, 0.5, 0.0},
    {-0.5, 0.5, 0.0},
    {0.5, 0.5, 0.0},
    {2.0, 0.5, 0.0}},
  {
    {-2.0, 2.0, 0.0},
    {-0.5, 2.0, 0.0},
    {0.5, 2.0, 0.0},
    {2.0, 2.0, 0.0}}
};
GLfloat *grid = &grid4x4[0][0][0];
int uSize = 4;
int vSize = 4;

void
setupMesh(void)
{
  glEnable(GL_MAP2_VERTEX_3);
  glMapGrid2f(gridSize, 0.0, 1.0, gridSize, 0.0, 1.0);
}

void
evaluateGrid(void)
{
  glColor3f(1.0, 1.0, 1.0);
  glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, uSize, 0, 1, uSize * 3, vSize, grid);
  glEvalMesh2(GL_LINE, 0, gridSize, 0, gridSize);
}

void
drawControlPoints(void)
{
  int i;

  glColor3f(1.0, 1.0, 0.0);
  glPointSize(5.0);
  glBegin(GL_POINTS);
  for (i = 0; i < uSize * vSize; i++) {
    glVertex3fv(&grid[i * 3]);
  }
  glEnd();
}

void
selectControlPoints(void)
{
  int i;

  for (i = 0; i < uSize * vSize; i++) {
    glLoadName(i);
    glBegin(GL_POINTS);
    glVertex3fv(&grid[i * 3]);
    glEnd();
  }
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  evaluateGrid();
  drawControlPoints();
  glutSwapBuffers();
}

void
ortho(void)
{
  if (winWidth <= winHeight)
    glOrtho(-4.0, 4.0, -4.0 * (GLfloat) winHeight / (GLfloat) winWidth,
      4.0 * (GLfloat) winHeight / (GLfloat) winWidth, -4.0, 4.0);
  else
    glOrtho(-4.0 * (GLfloat) winWidth / (GLfloat) winHeight,
      4.0 * (GLfloat) winWidth / (GLfloat) winHeight, -4.0, 4.0, -4.0, 4.0);
}

GLuint
pick(int x, int y)
{
  int hits;

  (void) glRenderMode(GL_SELECT);
  glInitNames();
  glPushName(~0);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluPickMatrix(x, winHeight - y, 8.0, 8.0, viewport);
  ortho();
  glMatrixMode(GL_MODELVIEW);
  selectControlPoints();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  hits = glRenderMode(GL_RENDER);
  if (hits) {
#ifdef DEBUG
    {
      unsigned int i;
      GLint names;
      GLuint *ptr;

      printf("hits = %d\n", hits);
      ptr = (GLuint *) selectBuffer;
      for (i = 0; i < hits; i++) {  /* for each hit  */
        int j;

        names = *ptr;
        printf("number of names for hit = %d\n", *ptr);
        ptr++;
        printf("  z1 is %g;", (float) *ptr / 0xffffffff);
        ptr++;
        printf("  z2 is %g\n", (float) *ptr / 0xffffffff);
        ptr++;
        printf(" the name is ");
        for (j = 0; j < names; j++) {  /* For each name. */
          printf("%d ", *ptr);
          ptr++;
        }
        printf("\n");
      }
    }
#endif
    return selectBuffer[3];
  } else {
    return ~0;
  }
}

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  winWidth = w;
  winHeight = h;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  ortho();
  glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
  viewport[0] = 0;
  viewport[1] = 0;
  viewport[2] = winWidth;
  viewport[3] = winHeight;
}

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      selectedPoint = pick(x, y);
    } else {
      selectedPoint = -1;
    }
  }
}

void
motion(int x, int y)
{
  GLdouble objx, objy, objz;

  if (selectedPoint != ~0) {
    gluUnProject(x, winHeight - y, 0.95,
      modelMatrix, projMatrix, viewport,
      &objx, &objy, &objz);
    grid[selectedPoint * 3 + 0] = objx;
    grid[selectedPoint * 3 + 1] = objy;
    glutPostRedisplay();
  }
}

/* ARGSUSED1 */
static void
keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
    exit(0);
  }
}

enum {
  M_2ND_ORDER_GRID, M_4TH_ORDER_GRID, M_INCREASE_GRID, M_DECREASE_GRID, M_TOGGLE_ANTIALIASING, M_QUIT
};

void
menu(int value)
{
  switch (value) {
  case M_2ND_ORDER_GRID:
    grid = &grid2x2[0][0][0];
    uSize = 2;
    vSize = 2;
    setupMesh();
    break;
  case M_4TH_ORDER_GRID:
    grid = &grid4x4[0][0][0];
    uSize = 4;
    vSize = 4;
    setupMesh();
    break;
  case M_INCREASE_GRID:
    gridSize += 2;
    setupMesh();
    break;
  case M_DECREASE_GRID:
    gridSize -= 2;
    if (gridSize < 2) {
      gridSize = 2;
    }
    setupMesh();
    break;
  case M_TOGGLE_ANTIALIASING:
    if (antialiasing) {
      antialiasing = 0;
      glDisable(GL_BLEND);
      glDisable(GL_LINE_SMOOTH);
      glDisable(GL_POINT_SMOOTH);
    } else {
      antialiasing = 1;
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      glEnable(GL_LINE_SMOOTH);
      glEnable(GL_POINT_SMOOTH);
    }
    break;
  case M_QUIT:
    exit(0);
    break;
  }
  glutPostRedisplay();
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow("editgrid");
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutKeyboardFunc(keyboard);
  glutMotionFunc(motion);
  glutCreateMenu(menu);
  glutAddMenuEntry("2nd order grid", M_2ND_ORDER_GRID);
  glutAddMenuEntry("4nd order grid", M_4TH_ORDER_GRID);
  glutAddMenuEntry("Increase grid sizing by 2", M_INCREASE_GRID);
  glutAddMenuEntry("Decrease grid sizing by 2", M_DECREASE_GRID);
  glutAddMenuEntry("Toggle antialiasing", M_TOGGLE_ANTIALIASING);
  glutAddMenuEntry("Quit", M_QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glSelectBuffer(sizeof(selectBuffer), selectBuffer);
  setupMesh();
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
