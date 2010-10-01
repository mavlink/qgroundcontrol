
/* tvertex.c - by David Blythe (with help from Mark Kilgard), SGI */

/* T-vertex artifacts example.  The moral: Avoid vertex edge junctions that
   make a T-shape. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>

static float scale = 1.;
static float transx = 0, transy = 0;
static float rotx = 29, roty = -21;  /* Initially askew. */
static int ox = -1, oy = -1;
static int show_t = 1;
static int mot;
#define PAN	1
#define ROT	2

void
pan(int x, int y)
{
  transx += (x - ox) / 500.;
  transy -= (y - oy) / 500.;
  ox = x;
  oy = y;
  glutPostRedisplay();
}

void
rotate(int x, int y)
{
  rotx += x - ox;
  if (rotx > 360.)
    rotx -= 360.;
  else if (rotx < -360.)
    rotx += 360.;
  roty += y - oy;
  if (roty > 360.)
    roty -= 360.;
  else if (roty < -360.)
    roty += 360.;
  ox = x;
  oy = y;
  glutPostRedisplay();
}

void
motion(int x, int y)
{
  if (mot == PAN)
    pan(x, y);
  else if (mot == ROT)
    rotate(x, y);
}

void
mouse(int button, int state, int x, int y)
{
  if (state == GLUT_DOWN) {
    switch (button) {
    case GLUT_LEFT_BUTTON:
      mot = PAN;
      motion(ox = x, oy = y);
      break;
    case GLUT_MIDDLE_BUTTON:
      mot = ROT;
      motion(ox = x, oy = y);
      break;
    }
  } else if (state == GLUT_UP) {
    mot = 0;
  }
}

void
toggle_t(void)
{
  show_t ^= 1;
}

void
wire(void)
{
  static int w;

  if (w ^= 1)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void
light(void)
{
  static int l = 1;

  if (l ^= 1)
    glEnable(GL_LIGHTING);
  else
    glDisable(GL_LIGHTING);
}

void
up(void)
{
  scale += .1;
}

void
down(void)
{
  scale -= .1;
}

void
help(void)
{
  printf("Usage: tvertex\n");
  printf("'h'            - help\n");
  printf("'l'            - toggle lighting\n");
  printf("'t'            - toggle T vertex\n");
  printf("'w'            - toggle wireframe\n");
  printf("'UP'           - scale up\n");
  printf("'DOWN'         - scale down\n");
  printf("left mouse     - pan\n");
  printf("middle mouse   - rotate\n");
}

void
init(void)
{
  GLfloat pos[4] =
  {0.0, 0.0, 1.0, 1.0};

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(50., 1., .1, 10.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0., 0., -3.5);

  /* The default light is "infinite"; a local light is important to ensure
     varying lighting color calculations at the vertices. */
  glLightfv(GL_LIGHT0, GL_POSITION, pos);

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glPushMatrix();
  glTranslatef(transx, transy, 0.f);
  glRotatef(rotx, 0., 1., 0.);
  glRotatef(roty, 1., 0., 0.);
  glScalef(scale, scale, 1.);
  if (show_t) {
    glBegin(GL_QUADS);
    glVertex2f(-1., 0.);
    glVertex2f(0., 0.);
    glVertex2f(0., 1.);
    glVertex2f(-1., 1.);

    glVertex2f(0., 0.);
    glVertex2f(1., 0.);
    glVertex2f(1., 1.);
    glVertex2f(0., 1.);

    glVertex2f(-1., -1.);
    glVertex2f(1., -1.);
    glVertex2f(1., 0.);
    glVertex2f(-1., 0.);
    glEnd();
  } else {
    glBegin(GL_QUADS);
    glVertex2f(-1., 0.);
    glVertex2f(0., 0.);
    glVertex2f(0., 1.);
    glVertex2f(-1., 1.);

    glVertex2f(0., 0.);
    glVertex2f(1., 0.);
    glVertex2f(1., 1.);
    glVertex2f(0., 1.);

    glVertex2f(-1., -1.);
    glVertex2f(0., -1.);
    glVertex2f(0., 0.);
    glVertex2f(-1., 0.);

    glVertex2f(0., -1.);
    glVertex2f(1., -1.);
    glVertex2f(1., 0.);
    glVertex2f(0., 0.);
    glEnd();
  }
  glPopMatrix();
  glutSwapBuffers();
}

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y)
{
  switch (key) {
  case 'l':
    light();
    break;
  case 't':
    toggle_t();
    break;
  case 'w':
    wire();
    break;
  case 'h':
    help();
    break;
  case '\033':
    exit(0);
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

/* ARGSUSED1 */
void
special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_UP:
    up();
    break;
  case GLUT_KEY_DOWN:
    down();
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

void
menu(int value)
{
  key((unsigned char) value, 0, 0);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  (void) glutCreateWindow("T vertex artifact demo");
  init();
  glutDisplayFunc(display);
  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutCreateMenu(menu);
  glutAddMenuEntry("Toggle T vertex", 't');
  glutAddMenuEntry("Toggle wireframe/solid", 'w');
  glutAddMenuEntry("Toggle lighting", 'l');
  glutAddMenuEntry("Quit", '\033');
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
