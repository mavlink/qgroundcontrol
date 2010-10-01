
/* textext.c - by David Blythe, SGI */

/* Example of using texturing for 3D transformable fonts. */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"
#include "textmap.h"

static float scale = .03;
static char *string = "OpenGL rules";
static float transx, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;
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
    case GLUT_RIGHT_BUTTON:
      break;
    }
  } else if (state == GLUT_UP) {
    mot = 0;
  }
}

void 
up(void)
{
  scale += .0025;
}

void 
down(void)
{
  scale -= .0025;
}

void 
help(void)
{
  printf("Usage: textext [string]\n");
  printf("'h'            - help\n");
  printf("'UP'           - scale up\n");
  printf("'DOWN'         - scale down\n");
  printf("left mouse     - pan\n");
  printf("middle mouse   - rotate\n");
}

void 
init(void)
{
  texfntinit("Times-Italic.bw");
  glEnable(GL_TEXTURE_2D);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(90., 1., .1, 10.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0., 0., -1.5);
}

void 
display(void)
{
  float width = texstrwidth(string);
  glClear(GL_COLOR_BUFFER_BIT);
  glPushMatrix();
  glTranslatef(transx, transy, 0.f);
  glRotatef(rotx, 0., 1., 0.);
  glRotatef(roty, 1., 0., 0.);
  glScalef(scale, scale, 0.);
  glTranslatef(-width * 5, 0.f, 0.f);
  texfntstroke(string, 0.f, 0.f);
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
  case 'h':
    help();
    break;
  case '\033':
    exit(1);
    break;
  default:
    break;
  }
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
  }
  glutPostRedisplay();
}

int 
main(int argc, char **argv)
{
  if (argc > 1)
    string = argv[1];
  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  (void) glutCreateWindow("textext");
  init();
  glutDisplayFunc(display);
  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
