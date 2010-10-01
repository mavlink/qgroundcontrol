
/* decal.c - by Tom McReynolds, SGI */

/* An Example of decaling, using stencil */

#include <GL/glut.h>
#include <stdlib.h>

/* ARGSUSED1 */
void 
key(unsigned char key, int x, int y)
{
  switch (key) {
  case '\033':
    exit(0);
  }
}

/* decal shape polygon onto base */
void 
decal_poly(void)
{
  glBegin(GL_QUADS);
  glNormal3f(0.f, 0.f, -1.f);
  glVertex3i(-2, 2, 0);
  glVertex3i(-2, 3, 0);
  glVertex3i(2, 3, 0);
  glVertex3i(2, 2, 0);

  glVertex3f(-.5, -3.f, 0);
  glVertex3f(-.5f, 2.f, 0);
  glVertex3f(.5f, 2.f, 0);
  glVertex3f(.5f, -3.f, 0);
  glEnd();
}

int angle = 0;

void 
redraw(void)
{
  /* clear stencil each time */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
  glDepthFunc(GL_LESS);

  glPushMatrix();
  glColor3f(1.f, 0.f, 0.f);
  glTranslatef(0.f, 0.f, -10.f);
  glScalef(5.f, 5.f, 5.f);
  glRotatef((GLfloat) angle, 0.f, 1.f, 0.f);
  glEnable(GL_NORMALIZE);
  glutSolidDodecahedron();
  glDisable(GL_NORMALIZE);
  glPopMatrix();

  glStencilFunc(GL_EQUAL, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glDepthFunc(GL_ALWAYS);

  glPushMatrix();
  glTranslatef(0.f, 0.f, -10.f);
  glRotatef((GLfloat) angle, 0.f, 1.f, 0.f);
  glRotatef(58.285f, 0.f, 1.f, 0.f);
  glTranslatef(0.f, 0.f, -7.265f);
  glColor3f(0.f, 1.f, 0.f);
  decal_poly();
  glPopMatrix();

  glDisable(GL_STENCIL_TEST);

  glutSwapBuffers();
}

void 
anim(void)
{
  angle = (angle + 1) % 360;
  glutPostRedisplay();
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(anim);
  else
    glutIdleFunc(NULL);
}

int
main(int argc, char *argv[])
{
  static GLfloat lightpos[] =
  {10.f, 5.f, 0.f, 1.f};

  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_STENCIL | GLUT_DEPTH | GLUT_DOUBLE);
  (void) glutCreateWindow("decal");
  glutDisplayFunc(redraw);
  glutKeyboardFunc(key);
  glutVisibilityFunc(visible);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glOrtho(-10., 10., -10., 10., 0., 20.);
  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
