/* pony.c */

/* 
 * By Brian Paul,  written July 31, 1997  for Mark.
 */

#include <GL/glut.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "readtex.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/************************ Pony-specific code *********************************/

static float BodyDepth = 0.216;

static GLfloat BodyVerts[][2] =
{
  {-0.993334, 0.344444},
  {-0.72, 0.462964},
  {-0.58, -0.411113},
  {-0.406667, -0.692593},
  {0.733334, -0.633334},
  {0.846666, -0.225926},
  {0.873335, -0.55926},
  {0.879998, -0.988888},
  {0.933332, -0.974074},
  {0.953334, -0.537037},
  {0.906667, -0.0777776},
  {0.806666, 0.0333334},
  {-0.26, 0.0111111},
  {-0.406667, 0.27037},
  {-0.54, 0.781481},
  {-0.673333, 1.00371},
  {-0.653332, 0.803704},
  {-1.05333, 0.44815}
};

static float LegDepth = 0.144;

static float FrontLegPos[3] =
{-0.36, -0.324, 0.108};
static GLfloat FrontLegVerts[][2] =
{
  {-0.23, -0.113481},
  {-0.123333, -0.528296},
  {-0.0926752, -0.728103},
  {-0.0766667, -1.232},
  {0.0233333, -1.232},
  {0.0433332, -0.743111},
  {0.0366667, -0.424593},
  {0.0699998, -0.157926},
  {0.116667, 0.049482},
  {-0.0166667, 0.197629},
  {-0.196667, 0.13837}
};

static float BackLegPos[3] =
{0.684, -0.324, 0.108};
static GLfloat BackLegVerts[][2] =
{
  {-0.24, -0.195556},
  {-0.0933332, -0.41037},
  {-0.04, -0.684445},
  {-0.113333, -1.26222},
  {0, -1.26222},
  {0.1, -0.677037},
  {0.213333, -0.121482},
  {0.153333, 0.108148},
  {-0.0533333, 0.211853},
  {-0.26, 0.063702}
};

static float ManeDepth = 0.288;
static GLfloat ManeVerts[][2] =
{
  {-0.512667, 0.578519},
  {-0.419333, 0.267407},
  {-0.299333, -0.00666719},
  {-0.239333, -0.0140724},
  {-0.226, 0.0896296},
  {-0.319333, 0.422963},
  {-0.532667, 0.741481}
};

static float EyePos[3] =
{-0.702, 0.648, 0.1116};
static float EyeSize = 0.025;

/* Display lists */
static GLuint Body = 0, FrontLeg = 0, BackLeg = 0, Mane = 0;

/* Generate an extruded, capped part from a 2-D polyline. */
static void 
ExtrudePart(int n, GLfloat v[][2], float depth)
{
  static GLUtriangulatorObj *tobj = NULL;
  int i;
  float z0 = 0.5 * depth;
  float z1 = -0.5 * depth;
  GLdouble vertex[3];

  if (tobj == NULL) {
    tobj = gluNewTess();  /* create and initialize a GLU polygon * *
                             tesselation object */
    gluTessCallback(tobj, GLU_BEGIN, glBegin);
    gluTessCallback(tobj, GLU_VERTEX, glVertex2fv);  /* semi-tricky */
    gluTessCallback(tobj, GLU_END, glEnd);
  }
  /* +Z face */
  glPushMatrix();
  glTranslatef(0.0, 0.0, z0);
  glNormal3f(0.0, 0.0, 1.0);
  gluBeginPolygon(tobj);
  for (i = 0; i < n; i++) {
    vertex[0] = v[i][0];
    vertex[1] = v[i][1];
    vertex[2] = 0.0;
    gluTessVertex(tobj, vertex, v[i]);
  }
  gluEndPolygon(tobj);
  glPopMatrix();

  /* -Z face */
  glFrontFace(GL_CW);
  glPushMatrix();
  glTranslatef(0.0, 0.0, z1);
  glNormal3f(0.0, 0.0, -1.0);
  gluBeginPolygon(tobj);
  for (i = 0; i < n; i++) {
    vertex[0] = v[i][0];
    vertex[1] = v[i][1];
    vertex[2] = z1;
    gluTessVertex(tobj, vertex, v[i]);
  }
  gluEndPolygon(tobj);
  glPopMatrix();

  glFrontFace(GL_CCW);
  /* edge polygons */
  glBegin(GL_TRIANGLE_STRIP);
  for (i = 0; i <= n; i++) {
    float x = v[i % n][0];
    float y = v[i % n][1];
    float dx = v[(i + 1) % n][0] - x;
    float dy = v[(i + 1) % n][1] - y;
    glVertex3f(x, y, z0);
    glVertex3f(x, y, z1);
    glNormal3f(dy, -dx, 0.0);
  }
  glEnd();

}

/* 
 * Build the four display lists which make up the pony.
 */
static void 
MakePony(void)
{
  static GLfloat blue[4] =
  {0.1, 0.1, 1.0, 1.0};
  static GLfloat black[4] =
  {0.0, 0.0, 0.0, 1.0};
  static GLfloat pink[4] =
  {1.0, 0.5, 0.5, 1.0};

  Body = glGenLists(1);
  glNewList(Body, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
  ExtrudePart(sizeof(BodyVerts) / sizeof(GLfloat) / 2, BodyVerts, BodyDepth);

  /* eyes */
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, black);
  glNormal3f(0.0, 0.0, 1.0);
  glBegin(GL_POLYGON);
  glVertex3f(EyePos[0] - EyeSize, EyePos[1] - EyeSize, EyePos[2]);
  glVertex3f(EyePos[0] + EyeSize, EyePos[1] - EyeSize, EyePos[2]);
  glVertex3f(EyePos[0] + EyeSize, EyePos[1] + EyeSize, EyePos[2]);
  glVertex3f(EyePos[0] - EyeSize, EyePos[1] + EyeSize, EyePos[2]);
  glEnd();
  glNormal3f(0.0, 0.0, -1.0);
  glBegin(GL_POLYGON);
  glVertex3f(EyePos[0] - EyeSize, EyePos[1] + EyeSize, -EyePos[2]);
  glVertex3f(EyePos[0] + EyeSize, EyePos[1] + EyeSize, -EyePos[2]);
  glVertex3f(EyePos[0] + EyeSize, EyePos[1] - EyeSize, -EyePos[2]);
  glVertex3f(EyePos[0] - EyeSize, EyePos[1] - EyeSize, -EyePos[2]);
  glEnd();
  glEndList();

  Mane = glGenLists(1);
  glNewList(Mane, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, pink);
  ExtrudePart(sizeof(ManeVerts) / sizeof(GLfloat) / 2, ManeVerts, ManeDepth);
  glEndList();

  FrontLeg = glGenLists(1);
  glNewList(FrontLeg, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
  ExtrudePart(sizeof(FrontLegVerts) / sizeof(GLfloat) / 2,
    FrontLegVerts, LegDepth);
  glEndList();

  BackLeg = glGenLists(1);
  glNewList(BackLeg, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
  ExtrudePart(sizeof(BackLegVerts) / sizeof(GLfloat) / 2,
    BackLegVerts, LegDepth);
  glEndList();
}

/* 
 * Draw the pony.  legAngle should be in [-15,15] or so.
 * The pony display lists will be constructed the first time this is called.
 */
static void 
DrawPony(float legAngle)
{
  if (!Body) {
    MakePony();
  }
  assert(Body);

  /* BODY */
  glCallList(Body);

  /* MANE */
  glCallList(Mane);

  /* FRONT +Z LEG */
  glPushMatrix();
  glTranslatef(FrontLegPos[0], FrontLegPos[1], FrontLegPos[2]);
  glRotatef(legAngle, 0.0, 0.0, 1.0);
  glCallList(FrontLeg);
  glPopMatrix();

  /* FRONT -Z LEG */
  glPushMatrix();
  glTranslatef(FrontLegPos[0], FrontLegPos[1], -FrontLegPos[2]);
  glRotatef(-legAngle, 0.0, 0.0, 1.0);
  glCallList(FrontLeg);
  glPopMatrix();

  /* BACK +Z LEG */
  glPushMatrix();
  glTranslatef(BackLegPos[0], BackLegPos[1], BackLegPos[2]);
  glRotatef(-legAngle, 0.0, 0.0, 1.0);
  glCallList(BackLeg);
  glPopMatrix();

  /* BACK -Z LEG */
  glPushMatrix();
  glTranslatef(BackLegPos[0], BackLegPos[1], -BackLegPos[2]);
  glRotatef(legAngle, 0.0, 0.0, 1.0);
  glCallList(BackLeg);
  glPopMatrix();
}

/************************* end of pony code **********************************/

static float Speed = 2.0;

static float LegAngleStep = 0.75;
static float LegMaxAngle = 15.0;
static float LegAngle = 0.0, LegDeltaAngle = 0.5;

static float WalkAngle = -90.0, DeltaWalkAngle = 0.225;
static float WalkRadius = 4.0;

static float Xrot = 0, Yrot = 30.0;

static GLboolean AnimFlag = GL_TRUE;

static void 
Idle(void)
{
    /* update animation vars */
    LegAngle += LegDeltaAngle * Speed;
    if (LegAngle > LegMaxAngle) {
      LegDeltaAngle = -LegAngleStep;
    } else if (LegAngle < -LegMaxAngle) {
      LegDeltaAngle = LegAngleStep;
    }
    WalkAngle += DeltaWalkAngle * Speed;

    glutPostRedisplay();
}

static void 
DrawGround(void)
{
  static GLuint ground = 0;

  if (ground == 0) {
    const int rows = 20, columns = 20;
    float sizeA = 1.25, sizeB = 0.2;
    float x, z;
    int i, j;
    GLfloat mat[2][4] =
    {
      {0.0, 0.6, 0.0, 1.0},
      {0.1, 0.8, 0.1, 1.0}
    };

    ground = glGenLists(1);
    glNewList(ground, GL_COMPILE);

    glNormal3f(0.0, 1.0, 0.0);

    x = -(columns * (sizeA + sizeB)) / 4;
    for (i = 0; i <= rows; i++) {
      float size = (i & 1) ? sizeA : sizeB;
      z = -(rows * (sizeA + sizeB)) / 4;
      glBegin(GL_QUAD_STRIP);
      for (j = 0; j <= columns; j++) {
        /* glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat[(i+j)%2]); */
        glColor4fv(mat[(i + j) % 2]);
        glVertex3f(x + size, 0.0, z);
        glVertex3f(x, 0.0, z);
        if (j & 1)
          z += sizeA;
        else
          z += sizeB;
      }
      glEnd();
      x += size;
    }

    glEndList();
  }
  glCallList(ground);
}

static void 
DrawLogo(void)
{
  glEnable(GL_TEXTURE_2D);
  glShadeModel(GL_SMOOTH);
  glBegin(GL_POLYGON);
  glColor3f(1, 0, 0);
  glTexCoord2f(0, 0);
  glVertex2f(-1.0, -0.5);
  glColor3f(0, 1, 0);
  glTexCoord2f(1, 0);
  glVertex2f(1.0, -0.5);
  glColor3f(0, 0, 1);
  glTexCoord2f(1, 1);
  glVertex2f(1.0, 0.5);
  glColor3f(1, 1, 0);
  glTexCoord2f(0, 1);
  glVertex2f(-1.0, 0.5);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glShadeModel(GL_FLAT);
}

static void 
Display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* viewing */
  glPushMatrix();
  glRotatef(Xrot, 1.0, 0.0, 0.0);
  glRotatef(Yrot, 0.0, 1.0, 0.0);

  /* ground */
  glDisable(GL_LIGHTING);
  glPushMatrix();
  glTranslatef(0.0, -1.6, 0.0);
  DrawGround();
  glPopMatrix();

  /* logo */
  glPushMatrix();
  glScalef(2.5, 2.5, 2.5);
  DrawLogo();
  glPopMatrix();

  /* pony */
  {
    float xPos, zPos;
    xPos = WalkRadius * cos(WalkAngle * M_PI / 180.0);
    zPos = WalkRadius * sin(WalkAngle * M_PI / 180.0);
    glEnable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(xPos, 0.0, zPos);
    glRotatef(-WalkAngle + 90.0, 0.0, 1.0, 0.0);
    DrawPony(LegAngle);
    glPopMatrix();
  }

  glPopMatrix();
  glutSwapBuffers();
}

static void 
Reshape(int width, int height)
{
  float ar;
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  ar = (float) width / (float) height;
  glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -23.0 / 2.5);
}

/* ARGSUSED1 */
static void 
Key(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
    exit(0);
    break;
  case ' ':
    AnimFlag = !AnimFlag;
    if (AnimFlag) {
      glutIdleFunc(Idle);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  }
  glutPostRedisplay();
}

/* ARGSUSED1 */
static void 
SpecialKey(int key, int x, int y)
{
  float step = 2.0;
  switch (key) {
  case GLUT_KEY_UP:
    Xrot += step;
    break;
  case GLUT_KEY_DOWN:
    Xrot -= step;
    break;
  case GLUT_KEY_LEFT:
    Yrot -= step;
    break;
  case GLUT_KEY_RIGHT:
    Yrot += step;
    break;
  }
  glutPostRedisplay();
}

static void 
Init(void)
{
  GLfloat lightPos[4] =
  {1.0, 10.0, 10.0, 0.0};
  glClearColor(0.5, 0.8, 0.99, 1.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
  glShadeModel(GL_FLAT);

  LoadRGBMipmaps("logo.bw", 1);
}

static void
vis(int visible)
{
  if (visible == GLUT_VISIBLE) {
    if (AnimFlag)
      glutIdleFunc(Idle);
  } else {
    if (AnimFlag)
      glutIdleFunc(NULL);
  }
}

int 
main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitWindowSize(640, 480);

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

  glutCreateWindow("Blue Pony");

  Init();

  glutReshapeFunc(Reshape);
  glutKeyboardFunc(Key);
  glutSpecialFunc(SpecialKey);
  glutDisplayFunc(Display);
  glutVisibilityFunc(vis);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
