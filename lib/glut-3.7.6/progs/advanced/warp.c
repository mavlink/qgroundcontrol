
/* warp.c - by David Blythe, SGI */

/* Image warping operations can be done via OpenGL texture mapping. */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include "texture.h"

static unsigned *image;
static int width, height, components;
static float incr = .05, dir = 1.0;

#define MAXMESH 32

float Ml[4 * 2 * (MAXMESH + 1) * 2 * (MAXMESH + 1)];

float N = 1.5;
float B = -1.5;

void
mesh1(float x0, float x1, float y0, float y1,
  float s0, float s1, float t0, float t1, float z, int nx, int ny)
{
  float y, x, s, t, dx, dy, ds, dt, vb[3], tb[2];
  float v;
  float *mp = Ml;

  dx = (x1 - x0) / nx;
  dy = (y1 - y0) / ny;
  ds = (s1 - s0) / nx;
  dt = (t1 - t0) / ny;
  y = y0;
  t = t0;
  vb[2] = z;
  while (y < y1) {
    x = x0;
    s = s0;
    while (x <= x1) {
      tb[0] = s;
      tb[1] = t;
      vb[0] = x;
      vb[1] = y;
      v = N * N - x * x - y * y;
      if (v < 0.0)
        v = 0.0;
      vb[2] = sqrt(v) + B;
      if (vb[2] < 0.)
        vb[2] = 0.0;
      *mp++ = tb[0];
      *mp++ = tb[1];
      mp += 2;
      *mp++ = vb[0];
      *mp++ = vb[1];
      *mp++ = vb[2];
      mp++;
      tb[1] = t + dt;
      vb[1] = y + dy;
      v = N * N - x * x - (y + dy) * (y + dy);
      if (v < 0.0)
        v = 0.0;
      vb[2] = sqrt(v) + B;
      if (vb[2] < 0.)
        vb[2] = 0.0;
      *mp++ = tb[0];
      *mp++ = tb[1];
      mp += 2;
      *mp++ = vb[0];
      *mp++ = vb[1];
      *mp++ = vb[2];
      mp++;
      x += dx;
      s += ds;
    }
    y += dy;
    t += dt;
  }
}

void
drawmesh(int nx, int ny)
{
  float *mp = Ml;
  int i, j;

  glColor4f(1, 1, 1, 1);
  for (i = ny + 1; i; i--) {
    glBegin(GL_TRIANGLE_STRIP);
    for (j = nx + 1; j; j--) {
      glTexCoord2fv(mp);
      glVertex3fv(mp + 4);
      glTexCoord2fv(mp + 8);
      glVertex3fv(mp + 12);
      mp += 16;
    }
    glEnd();
  }
}

void
move(void)
{
  if (N > 2.1 || N < 1.5)
    dir = -dir;
  N += incr * dir;
  mesh1(-1.5, 1.5, -1.5, 1.5, 0.0, 1.0, 0.0, 1.0, 0.0, MAXMESH, MAXMESH);
  glutPostRedisplay();
}

void
alphaup(void)
{
  incr += .01;
  if (incr > .1)
    incr = .1;
  glutPostRedisplay();
}

void
alphadown(void)
{
  incr -= .01;
  if (incr < 0)
    incr = 0;
  glutPostRedisplay();
}

void
wire(void)
{
  static int wire_mode;
  if (wire_mode ^= 1)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void
help(void)
{
  printf("'h'   - help\n");
  printf("'w'   - wire frame\n");
  printf("UP	  - faster\n");
  printf("DOWN  - slower\n");
}

void
init(char *filename)
{
  if (filename) {
    image = read_texture(filename, &width, &height, &components);
    if (image == NULL) {
      fprintf(stderr, "Error: Can't load image file \"%s\".\n",
        filename);
      exit(1);
    } else {
      printf("%d x %d image loaded\n", width, height);
    }
    if (components < 3 || components > 4) {
      printf("must be RGB or RGBA image\n");
      exit(1);
    }
  } else {
    int i, j;
    components = 4;
    width = height = 512;
    image = (unsigned *) malloc(width * height * sizeof(unsigned));
    for (j = 0; j < height; j++)
      for (i = 0; i < width; i++) {
        if (i & 64)
          image[i + j * width] = 0xff;
        else
          image[i + j * width] = 0xff00;
        if (j & 64)
          image[i + j * width] |= 0xff0000;
      }

  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, components, width,
    height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
    image);
  glEnable(GL_TEXTURE_2D);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(90., 1., .1, 10.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0., 0., -1.5);
  glClearColor(.25, .25, .25, 0.);

}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  drawmesh(MAXMESH, MAXMESH);
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
  case '\033':
    exit(0);
    break;
  case 'h':
    help();
    break;
  case 'w':
    wire();
    break;
  }
}

/* ARGSUSED1 */
void
special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_UP:
    alphaup();
    break;
  case GLUT_KEY_DOWN:
    alphadown();
    break;
  }
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(move);
  else
    glutIdleFunc(NULL);
}

void
menu(int value)
{
  if(value < 0)
    special(-value, 0, 0);
  else
    key((unsigned char) value, 0, 0);
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  (void) glutCreateWindow("warp");
  init(argv[1]);
  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutVisibilityFunc(visible);
  glutCreateMenu(menu);
  glutAddMenuEntry("Toggle wireframe", 'w');
  glutAddMenuEntry("Quicken warping", -GLUT_KEY_UP);
  glutAddMenuEntry("Slow warping", -GLUT_KEY_DOWN);
  glutAddMenuEntry("Quit", '\033');
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
