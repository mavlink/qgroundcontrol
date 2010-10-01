
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This program was originally written by someone else (Simon Hui?);
   I just added a bit more GLUT stuff to it. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#define VORDER 10
#define CORDER 10
#define TORDER 3

#define VMAJOR_ORDER 2
#define VMINOR_ORDER 3

#define CMAJOR_ORDER 2
#define CMINOR_ORDER 2

#define TMAJOR_ORDER 2
#define TMINOR_ORDER 2

#define VDIM 4
#define CDIM 4
#define TDIM 2

#define ONE_D 1
#define TWO_D 2

#define EVAL 3
#define MESH 4

GLenum doubleBuffer;

float rotX = 0.0, rotY = 0.0, translateZ = -1.0;

GLenum arrayType = ONE_D;
GLenum colorType = GL_FALSE;
GLenum textureType = GL_FALSE;
GLenum polygonFilled = GL_FALSE;
GLenum lighting = GL_FALSE;
GLenum mapPoint = GL_FALSE;
GLenum mapType = EVAL;

double point1[10 * 4] =
{
  -0.5, 0.0, 0.0, 1.0,
  -0.4, 0.5, 0.0, 1.0,
  -0.3, -0.5, 0.0, 1.0,
  -0.2, 0.5, 0.0, 1.0,
  -0.1, -0.5, 0.0, 1.0,
  0.0, 0.5, 0.0, 1.0,
  0.1, -0.5, 0.0, 1.0,
  0.2, 0.5, 0.0, 1.0,
  0.3, -0.5, 0.0, 1.0,
  0.4, 0.0, 0.0, 1.0,
};
double cpoint1[10 * 4] =
{
  0.0, 0.0, 1.0, 1.0,
  0.3, 0.0, 0.7, 1.0,
  0.6, 0.0, 0.3, 1.0,
  1.0, 0.0, 0.0, 1.0,
  1.0, 0.3, 0.0, 1.0,
  1.0, 0.6, 0.0, 1.0,
  1.0, 1.0, 0.0, 1.0,
  1.0, 1.0, 0.5, 1.0,
  1.0, 1.0, 1.0, 1.0,
};
double tpoint1[11 * 4] =
{
  0.0, 0.0, 0.0, 1.0,
  0.0, 0.1, 0.0, 1.0,
  0.0, 0.2, 0.0, 1.0,
  0.0, 0.3, 0.0, 1.0,
  0.0, 0.4, 0.0, 1.0,
  0.0, 0.5, 0.0, 1.0,
  0.0, 0.6, 0.0, 1.0,
  0.0, 0.7, 0.0, 1.0,
  0.0, 0.8, 0.0, 1.0,
  0.0, 0.9, 0.0, 1.0,
};
double point2[2 * 3 * 4] =
{
  -0.5, -0.5, 0.5, 1.0,
  0.0, 1.0, 0.5, 1.0,
  0.5, -0.5, 0.5, 1.0,
  -0.5, 0.5, -0.5, 1.0,
  0.0, -1.0, -0.5, 1.0,
  0.5, 0.5, -0.5, 1.0,
};
double cpoint2[2 * 2 * 4] =
{
  0.0, 0.0, 0.0, 1.0,
  0.0, 0.0, 1.0, 1.0,
  0.0, 1.0, 0.0, 1.0,
  1.0, 1.0, 1.0, 1.0,
};
double tpoint2[2 * 2 * 2] =
{
  0.0, 0.0, 0.0, 1.0,
  1.0, 0.0, 1.0, 1.0,
};
float textureImage[4 * 2 * 4] =
{
  1.0, 1.0, 1.0, 1.0,
  1.0, 0.0, 0.0, 1.0,
  1.0, 0.0, 0.0, 1.0,
  1.0, 1.0, 1.0, 1.0,
  1.0, 1.0, 1.0, 1.0,
  1.0, 0.0, 0.0, 1.0,
  1.0, 0.0, 0.0, 1.0,
  1.0, 1.0, 1.0, 1.0,
};

static void
Init(void)
{
  static float ambient[] =
  {0.1, 0.1, 0.1, 1.0};
  static float diffuse[] =
  {1.0, 1.0, 1.0, 1.0};
  static float position[] =
  {0.0, 0.0, -150.0, 0.0};
  static float front_mat_diffuse[] =
  {1.0, 0.2, 1.0, 1.0};
  static float back_mat_diffuse[] =
  {1.0, 1.0, 0.2, 1.0};
  static float lmodel_ambient[] =
  {1.0, 1.0, 1.0, 1.0};
  static float lmodel_twoside[] =
  {GL_TRUE};
  static float decal[] =
  {GL_DECAL};
  static float repeat[] =
  {GL_REPEAT};
  static float nr[] =
  {GL_NEAREST};

  glFrontFace(GL_CCW);

  glEnable(GL_DEPTH_TEST);

  glMap1d(GL_MAP1_VERTEX_4, 0.0, 1.0, VDIM, VORDER, point1);
  glMap1d(GL_MAP1_COLOR_4, 0.0, 1.0, CDIM, CORDER, cpoint1);

  glMap2d(GL_MAP2_VERTEX_4, 0.0, 1.0, VMINOR_ORDER * VDIM, VMAJOR_ORDER, 0.0,
    1.0, VDIM, VMINOR_ORDER, point2);
  glMap2d(GL_MAP2_COLOR_4, 0.0, 1.0, CMINOR_ORDER * CDIM, CMAJOR_ORDER, 0.0,
    1.0, CDIM, CMINOR_ORDER, cpoint2);
  glMap2d(GL_MAP2_TEXTURE_COORD_2, 0.0, 1.0, TMINOR_ORDER * TDIM,
    TMAJOR_ORDER, 0.0, 1.0, TDIM, TMINOR_ORDER, tpoint2);

  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, position);

  glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
  glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

  glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, decal);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nr);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nr);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, 2, 4, 0, GL_RGBA, GL_FLOAT,
    (GLvoid *) textureImage);
}

static void
DrawPoints1(void)
{
  GLint i;

  glColor3f(0.0, 1.0, 0.0);
  glPointSize(2);
  glBegin(GL_POINTS);
  for (i = 0; i < VORDER; i++) {
    glVertex4dv(&point1[i * 4]);
  }
  glEnd();
}

static void
DrawPoints2(void)
{
  GLint i, j;

  glColor3f(1.0, 0.0, 1.0);
  glPointSize(2);
  glBegin(GL_POINTS);
  for (i = 0; i < VMAJOR_ORDER; i++) {
    for (j = 0; j < VMINOR_ORDER; j++) {
      glVertex4dv(&point2[i * 4 * VMINOR_ORDER + j * 4]);
    }
  }
  glEnd();
}

static void
DrawMapEval1(float du)
{
  float u;

  glColor3f(1.0, 0.0, 0.0);
  glBegin(GL_LINE_STRIP);
  for (u = 0.0; u < 1.0; u += du) {
    glEvalCoord1d(u);
  }
  glEvalCoord1d(1.0);
  glEnd();
}

static void
DrawMapEval2(float du, float dv)
{
  float u, v, tmp;

  glColor3f(1.0, 0.0, 0.0);
  for (v = 0.0; v < 1.0; v += dv) {
    glBegin(GL_QUAD_STRIP);
    for (u = 0.0; u <= 1.0; u += du) {
      glEvalCoord2d(u, v);
      tmp = (v + dv < 1.0) ? (v + dv) : 1.0;
      glEvalCoord2d(u, tmp);
    }
    glEvalCoord2d(1.0, v);
    glEvalCoord2d(1.0, v + dv);
    glEnd();
  }
}

static void
RenderEval(void)
{

  if (colorType) {
    glEnable(GL_MAP1_COLOR_4);
    glEnable(GL_MAP2_COLOR_4);
  } else {
    glDisable(GL_MAP1_COLOR_4);
    glDisable(GL_MAP2_COLOR_4);
  }

  if (textureType) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_MAP2_TEXTURE_COORD_2);
  } else {
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_MAP2_TEXTURE_COORD_2);
  }

  if (polygonFilled) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

  glShadeModel(GL_SMOOTH);

  switch (mapType) {
  case EVAL:
    switch (arrayType) {
    case ONE_D:
      glDisable(GL_MAP2_VERTEX_4);
      glEnable(GL_MAP1_VERTEX_4);
      DrawPoints1();
      DrawMapEval1(0.1 / VORDER);
      break;
    case TWO_D:
      glDisable(GL_MAP1_VERTEX_4);
      glEnable(GL_MAP2_VERTEX_4);
      DrawPoints2();
      DrawMapEval2(0.1 / VMAJOR_ORDER, 0.1 / VMINOR_ORDER);
      break;
    }
    break;
  case MESH:
    switch (arrayType) {
    case ONE_D:
      DrawPoints1();
      glDisable(GL_MAP2_VERTEX_4);
      glEnable(GL_MAP1_VERTEX_4);
      glColor3f(0.0, 0.0, 1.0);
      glMapGrid1d(40, 0.0, 1.0);
      if (mapPoint) {
        glPointSize(2);
        glEvalMesh1(GL_POINT, 0, 40);
      } else {
        glEvalMesh1(GL_LINE, 0, 40);
      }
      break;
    case TWO_D:
      DrawPoints2();
      glDisable(GL_MAP1_VERTEX_4);
      glEnable(GL_MAP2_VERTEX_4);
      glColor3f(0.0, 0.0, 1.0);
      glMapGrid2d(20, 0.0, 1.0, 20, 0.0, 1.0);
      if (mapPoint) {
        glPointSize(2);
        glEvalMesh2(GL_POINT, 0, 20, 0, 20);
      } else if (polygonFilled) {
        glEvalMesh2(GL_FILL, 0, 20, 0, 20);
      } else {
        glEvalMesh2(GL_LINE, 0, 20, 0, 20);
      }
      break;
    default:;
      /* Mesa makes GLenum be a C "enum" and gcc will warn if
         all the cases of an enum are not tested in a switch
	 statement.  Add default case to supress the error. */
    }
    break;
  }
}

static void
Reshape(int width, int height)
{

  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 10.0);
  glMatrixMode(GL_MODELVIEW);
}

/* ARGSUSED1 */
static void
Key(unsigned char key, int x, int y)
{
  switch (key) {
  case '1':
    arrayType = ONE_D;
    glDisable(GL_AUTO_NORMAL);
    glutPostRedisplay();
    break;
  case '2':
    arrayType = TWO_D;
    glEnable(GL_AUTO_NORMAL);
    glutPostRedisplay();
    break;
  case '3':
    mapType = EVAL;
    glutPostRedisplay();
    break;
  case '4':
    mapType = MESH;
    glutPostRedisplay();
    break;
  case '5':
    polygonFilled = !polygonFilled;
    glutPostRedisplay();
    break;
  case '6':
    mapPoint = !mapPoint;
    glutPostRedisplay();
    break;
  case '7':
    colorType = !colorType;
    glutPostRedisplay();
    break;
  case '8':
    textureType = !textureType;
    glutPostRedisplay();
    break;
  case '9':
    lighting = !lighting;
    if (lighting) {
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      if (arrayType == TWO_D) {
        glEnable(GL_AUTO_NORMAL);
      } else {
        glDisable(GL_AUTO_NORMAL);
      }
    } else {
      glDisable(GL_LIGHTING);
      glDisable(GL_LIGHT0);
      glDisable(GL_AUTO_NORMAL);
    }
    glutPostRedisplay();
    break;
  case 27:             /* Escape key. */
    exit(0);
  }
}

static void
Menu(int value)
{
  /* Menu items have key values assigned to them.  Just pass
     this value to the key routine. */
  Key((unsigned char) value, 0, 0);
}

/* ARGSUSED1 */
static void
SpecialKey(int key, int x, int y)
{

  switch (key) {
  case GLUT_KEY_LEFT:
    rotY -= 30;
    glutPostRedisplay();
    break;
  case GLUT_KEY_RIGHT:
    rotY += 30;
    glutPostRedisplay();
    break;
  case GLUT_KEY_UP:
    rotX -= 30;
    glutPostRedisplay();
    break;
  case GLUT_KEY_DOWN:
    rotX += 30;
    glutPostRedisplay();
    break;
  }
}

static void
Draw(void)
{

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

  glTranslatef(0.0, 0.0, translateZ);
  glRotatef(rotX, 1, 0, 0);
  glRotatef(rotY, 0, 1, 0);
  RenderEval();

  glPopMatrix();

  if (doubleBuffer) {
    glutSwapBuffers();
  } else {
    glFlush();
  }
}

static void
Args(int argc, char **argv)
{
  GLint i;

  doubleBuffer = GL_FALSE;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-sb") == 0) {
      doubleBuffer = GL_FALSE;
    } else if (strcmp(argv[i], "-db") == 0) {
      doubleBuffer = GL_TRUE;
    }
  }
}

int
main(int argc, char **argv)
{
  GLenum type;

  glutInit(&argc, argv);
  Args(argc, argv);

  type = GLUT_RGB | GLUT_DEPTH;
  type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
  glutInitDisplayMode(type);
  glutInitWindowSize(300, 300);
  glutCreateWindow("Evaluator Test");

  glutCreateMenu(Menu);
  glutAddMenuEntry("One dimensional", '1');
  glutAddMenuEntry("Two dimensional", '2');
  glutAddMenuEntry("Eval map type", '3');
  glutAddMenuEntry("Mesh map type", '4');
  glutAddMenuEntry("Toggle filled", '5');
  glutAddMenuEntry("Toggle map point", '6');
  glutAddMenuEntry("Toggle color", '7');
  glutAddMenuEntry("Toggle texture", '8');
  glutAddMenuEntry("Toggle lighting", '9');
  glutAddMenuEntry("Quit", 27);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutAttachMenu(GLUT_LEFT_BUTTON);

  Init();

  glutReshapeFunc(Reshape);
  glutKeyboardFunc(Key);
  glutSpecialFunc(SpecialKey);
  glutDisplayFunc(Draw);
  glutMainLoop();
  return 0;
}
