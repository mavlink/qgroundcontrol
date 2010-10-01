
/* silhouette.c - by Tom McReynolds, SGI */

/* Doing Silhouette Edges with stencil */

#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>

enum {
  CONE = 1
};

/* Draw a cone */
void
cone(void)
{
  glPushMatrix();
  glTranslatef(0.f, 0.f, -30.f);
  glCallList(CONE);
  glPopMatrix();
}

/* Draw a torus */
void
torus(void)
{
  glutSolidTorus(10., 20., 20, 20);
}

enum {
  SIL, OBJ, SIL_AND_OBJ, TOGGLE
};

int rendermode = OBJ;

void (*curobj) (void) = cone;

void 
menu(int mode)
{
  switch (mode) {
  case SIL:
  case OBJ:
  case SIL_AND_OBJ:
    rendermode = mode;
    break;
  case TOGGLE:
    if (curobj == cone)
      curobj = torus;
    else
      curobj = cone;
    break;
  }
  glutPostRedisplay();
}

int winWidth = 512;
int winHeight = 512;

/* used to get current width and height of viewport */
void
reshape(int wid, int ht)
{
  glViewport(0, 0, wid, ht);
  winWidth = wid;
  winHeight = ht;
  glutPostRedisplay();
}

GLfloat viewangle;

void
drawsilhouette(void)
{
  int i;

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glDisable(GL_DEPTH_TEST);  /* so the depth buffer doesn't change */
  for (i = -1; i < 2; i += 2) {  /* set stencil around object */
    glViewport(i, 0, winWidth + i, winHeight);
    curobj();
  }
  for (i = -1; i < 2; i += 2) {
    glViewport(0, i, winWidth, winHeight + i);
    curobj();
  }

  /* cut out stencil where object is */
  glViewport(0, 0, winWidth, winHeight);
  glStencilFunc(GL_ALWAYS, 0, 0);
  curobj();

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  glStencilFunc(GL_EQUAL, 1, 1);

  glDisable(GL_LIGHTING);
  glColor3f(1.f, 0.f, 0.f);  /* draw silhouette red */
  glRotatef(-viewangle, 0.f, 1.f, 0.f);
  glRecti(-50, -50, 50, 50);
  glRotatef(viewangle, 0.f, 1.f, 0.f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glDisable(GL_STENCIL_TEST);
}

void
redraw(void)
{
  /* clear stencil each time */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glPushMatrix();
  glRotatef(viewangle, 0.f, 1.f, 0.f);

  switch (rendermode) {
  case SIL:
    drawsilhouette();
    break;
  case SIL_AND_OBJ:
    drawsilhouette();
    curobj();
    break;
  case OBJ:
    curobj();
    break;
  }

  glPopMatrix();
  glutSwapBuffers();
}

/* animate scene by rotating */
enum {
  ANIM_LEFT, ANIM_RIGHT
};
int animDirection = ANIM_LEFT;

void 
anim(void)
{
  if (animDirection == ANIM_LEFT)
    viewangle -= 1.f;
  else
    viewangle += 1.f;
  glutPostRedisplay();
}

/* ARGSUSED1 */
/* special keys, like array and F keys */
void 
special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_LEFT:
    glutIdleFunc(anim);
    animDirection = ANIM_LEFT;
    break;
  case GLUT_KEY_RIGHT:
    glutIdleFunc(anim);
    animDirection = ANIM_RIGHT;
    break;
  case GLUT_KEY_UP:
  case GLUT_KEY_DOWN:
    glutIdleFunc(0);
    break;
  }
}

/* ARGSUSED1 */
void 
key(unsigned char key, int x, int y)
{
  switch (key) {
  case 'a':
    viewangle -= 10.f;
    glutPostRedisplay();
    break;
  case 's':
    viewangle += 10.f;
    glutPostRedisplay();
    break;
  case '\033':
    exit(0);
  }
}

int picked_object;
int xpos = 0, ypos = 0;
int newxpos, newypos;
int startx, starty;

int
main(int argc, char **argv)
{
  static GLfloat lightpos[] =
  {25.f, 50.f, -50.f, 1.f};
  static GLfloat cone_mat[] =
  {0.f, .5f, 1.f, 1.f};
  GLUquadricObj *cone, *base;

  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_STENCIL | GLUT_DEPTH | GLUT_DOUBLE);
  (void) glutCreateWindow("silhouette edges");
  glutDisplayFunc(redraw);
  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutReshapeFunc(reshape);

  glutCreateMenu(menu);
  glutAddMenuEntry("Object", OBJ);
  glutAddMenuEntry("Silhouette Only", SIL);
  glutAddMenuEntry("Object and Silhouette", SIL_AND_OBJ);
  glutAddMenuEntry("Toggle Object", TOGGLE);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  /* make display list for cone; for efficiency */

  glNewList(CONE, GL_COMPILE);
  cone = gluNewQuadric();
  base = gluNewQuadric();
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cone_mat);
  gluQuadricOrientation(base, GLU_INSIDE);
  gluDisk(base, 0., 15., 32, 1);
  gluCylinder(cone, 15., 0., 60., 32, 32);
  gluDeleteQuadric(cone);
  gluDeleteQuadric(base);
  glEndList();

  glMatrixMode(GL_PROJECTION);
  glOrtho(-50., 50., -50., 50., -50., 50.);
  glMatrixMode(GL_MODELVIEW);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
