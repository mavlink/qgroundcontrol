/* bounce.c */

/*
 * Bouncing ball demo.
 *
 * This program is in the public domain
 *
 * Brian Paul
 */

/* Conversion to GLUT by Mark J. Kilgard */

#include <math.h>
#include <stdlib.h>
#include <GL/glut.h>

#define COS(X)   cos( (X) * 3.14159/180.0 )
#define SIN(X)   sin( (X) * 3.14159/180.0 )

#define RED 1
#define WHITE 2
#define CYAN 3

GLuint Ball;
GLenum Mode;
GLfloat Zrot = 0.0, Zstep = 6.0;
GLfloat Xpos = 0.0, Ypos = 1.0;
GLfloat Xvel = 0.2, Yvel = 0.0;
GLfloat Xmin = -4.0, Xmax = 4.0;
GLfloat Ymin = -3.8, Ymax = 4.0;
GLfloat G = -0.1;

static GLuint 
make_ball(void)
{
  GLuint list;
  GLfloat a, b;
  GLfloat da = 18.0, db = 18.0;
  GLfloat radius = 1.0;
  GLuint color;
  GLfloat x, y, z;

  list = glGenLists(1);

  glNewList(list, GL_COMPILE);

  color = 0;
  for (a = -90.0; a + da <= 90.0; a += da) {

    glBegin(GL_QUAD_STRIP);
    for (b = 0.0; b <= 360.0; b += db) {

      if (color) {
	glIndexi(RED);
      } else {
	glIndexi(WHITE);
      }

      x = COS(b) * COS(a);
      y = SIN(b) * COS(a);
      z = SIN(a);
      glVertex3f(x, y, z);

      x = radius * COS(b) * COS(a + da);
      y = radius * SIN(b) * COS(a + da);
      z = radius * SIN(a + da);
      glVertex3f(x, y, z);

      color = 1 - color;
    }
    glEnd();

  }

  glEndList();

  return list;
}

static void 
reshape(int width, int height)
{
  glViewport(0, 0, (GLint) width, (GLint) height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-6.0, 6.0, -6.0, 6.0, -6.0, 6.0);
  glMatrixMode(GL_MODELVIEW);
}

/* ARGSUSED1 */
static void
key(unsigned char k, int x, int y)
{
  switch (k) {
  case 27:  /* Escape */
    exit(0);
  }
}

static void 
draw(void)
{
  GLint i;

  glClear(GL_COLOR_BUFFER_BIT);

  glIndexi(CYAN);
  glBegin(GL_LINES);
  for (i = -5; i <= 5; i++) {
    glVertex2i(i, -5);
    glVertex2i(i, 5);
  }
  for (i = -5; i <= 5; i++) {
    glVertex2i(-5, i);
    glVertex2i(5, i);
  }
  for (i = -5; i <= 5; i++) {
    glVertex2i(i, -5);
    glVertex2f(i * 1.15, -5.9);
  }
  glVertex2f(-5.3, -5.35);
  glVertex2f(5.3, -5.35);
  glVertex2f(-5.75, -5.9);
  glVertex2f(5.75, -5.9);
  glEnd();

  glPushMatrix();
  glTranslatef(Xpos, Ypos, 0.0);
  glScalef(2.0, 2.0, 2.0);
  glRotatef(8.0, 0.0, 0.0, 1.0);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  glRotatef(Zrot, 0.0, 0.0, 1.0);

  glCallList(Ball);

  glPopMatrix();

  glFlush();
  glutSwapBuffers();
}

static void 
idle(void)
{
  static float vel0 = -100.0;

  Zrot += Zstep;

  Xpos += Xvel;
  if (Xpos >= Xmax) {
    Xpos = Xmax;
    Xvel = -Xvel;
    Zstep = -Zstep;
  }
  if (Xpos <= Xmin) {
    Xpos = Xmin;
    Xvel = -Xvel;
    Zstep = -Zstep;
  }
  Ypos += Yvel;
  Yvel += G;
  if (Ypos < Ymin) {
    Ypos = Ymin;
    if (vel0 == -100.0)
      vel0 = fabs(Yvel);
    Yvel = vel0;
  }
  glutPostRedisplay();
}

void 
visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(idle);
  else
    glutIdleFunc(NULL);
}

int
main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_INDEX | GLUT_DOUBLE);

  glutCreateWindow("Bounce");
  Ball = make_ball();
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glDisable(GL_DITHER);
  glShadeModel(GL_FLAT);

  glutDisplayFunc(draw);
  glutReshapeFunc(reshape);
  glutVisibilityFunc(visible);
  glutKeyboardFunc(key);

  glutSetColor(RED, 1.0, 0.0, 0.0);
  glutSetColor(WHITE, 1.0, 1.0, 1.0);
  glutSetColor(CYAN, 0.0, 1.0, 1.0);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
