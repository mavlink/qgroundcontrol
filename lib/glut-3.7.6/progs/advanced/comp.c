
/* comp.c - by David Blythe, SGI */

/* Porter/Duff compositing operations using OpenGL alpha blending. */

/* NOTE:  This program uses OpenGL blending functions that need the    frame
   buffer to retain a destination alpha component.  Examples of such
   hardware:  O2, IMPACT, RealityEngine, and InfiniteReality. */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"

static int w = 640, h = 640;

void
myReshape(int nw, int nh)
{
  w = nw, h = nh;
  glClearColor(0, 0, 0, 0);
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

static unsigned *a_data = 0, *b_data = 0;
static int a_width, a_height, b_width, b_height;

void
init_tris(void)
{
  if (a_data)
    free(a_data);
  if (b_data)
    free(b_data);
  a_data = (unsigned *) malloc(w * h);
  b_data = (unsigned *) malloc(w * h);
  glViewport(0, 0, w / 2, h / 2);
  glClear(GL_COLOR_BUFFER_BIT);
  glColor4f(1, 0, 0, 1);
  glBegin(GL_TRIANGLES);
  glVertex2f(-1, -1);
  glVertex2f(-1, 1);
  glVertex2f(.5, 1);
  glEnd();
  glReadPixels(0, 0, w / 2, h / 2, GL_RGBA, GL_UNSIGNED_BYTE, a_data);

  glClear(GL_COLOR_BUFFER_BIT);
  glColor4f(0, 1, 0, 1);
  glBegin(GL_TRIANGLES);
  glVertex2f(1, -1);
  glVertex2f(1, 1);
  glVertex2f(-.5, 1);
  glEnd();
  glReadPixels(0, 0, w / 2, h / 2, GL_RGBA, GL_UNSIGNED_BYTE, b_data);

  a_width = b_width = w / 2, a_height = b_height = h / 2;
}

void
init_images(void)
{
  int comp;

  a_data = read_texture("a.rgb", &a_width, &a_height, &comp);
  printf("%dx%dx%d\n", a_width, a_height, comp);
  b_data = read_texture("b.rgb", &b_width, &b_height, &comp);
  printf("%dx%dx%d\n", b_width, b_height, comp);
}

void
display(void)
{
  int i;

  glDrawBuffer(GL_FRONT_AND_BACK);
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawBuffer(GL_BACK);

  glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
  /* image A */
  glViewport(0, 0, w / 2, h / 2);
  glRasterPos2f(-1, -1);
  glDrawPixels(a_width, a_height, GL_RGBA, GL_UNSIGNED_BYTE,
    a_data);
  glEnable(GL_BLEND);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
  glCopyPixels(0, 0, w / 2, h / 2, GL_COLOR);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDisable(GL_BLEND);

  /* image B */
  glViewport(w / 2, 0, w / 2, h / 2);
  glRasterPos2f(-1, -1);
  glDrawPixels(b_width, b_height, GL_RGBA, GL_UNSIGNED_BYTE,
    b_data);
  glEnable(GL_BLEND);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
  glCopyPixels(w / 2, 0, w / 2, h / 2, GL_COLOR);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glReadBuffer(GL_BACK);
  glDrawBuffer(GL_FRONT);

  for (i = 0; i < 4; i++) {
    glDisable(GL_BLEND);
    switch (i) {
    case 0:            /* a over b */
      glViewport(0, 0, w / 2, h / 2);
      glRasterPos2f(-1, -1);
      glCopyPixels(0, 0, w / 2, h / 2, GL_COLOR);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
      glCopyPixels(w / 2, 0, w / 2, h / 2, GL_COLOR);
      break;
    case 1:            /* a under b */
      glViewport(w / 2, 0, w / 2, h / 2);
      glRasterPos2f(-1, -1);
      glCopyPixels(0, 0, w / 2, h / 2, GL_COLOR);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glCopyPixels(w / 2, 0, w / 2, h / 2, GL_COLOR);
      break;
    case 2:            /* b in a */
      glViewport(0, h / 2, w / 2, h / 2);
      glRasterPos2f(-1, -1);
      glCopyPixels(0, 0, w / 2, h / 2, GL_COLOR);
      glEnable(GL_BLEND);
      glBlendFunc(GL_DST_ALPHA, GL_ZERO);
      glCopyPixels(w / 2, 0, w / 2, h / 2, GL_COLOR);
      break;
    case 3:            /* b out of a */
      glViewport(w / 2, h / 2, w / 2, h / 2);
      glRasterPos2f(-1, -1);
      glCopyPixels(0, 0, w / 2, h / 2, GL_COLOR);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ZERO);
      glCopyPixels(w / 2, 0, w / 2, h / 2, GL_COLOR);
      break;
    }
  }
  glViewport(0, 0, w, h);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
  glColor4f(1., 1., 1., 0.);
  glRectf(-1, -1, 1, 1);
  glFlush();
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y)
{
  if (key == '\033')
    exit(0);
}

int
main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitWindowSize(w, h);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
    printf("comp: requires a frame buffer with destination alpha\n");
    exit(1);
  }
  glutCreateWindow("comp");

  if (argc > 1)
    init_images();
  else
    init_tris();
  glutDisplayFunc(display);
  glutReshapeFunc(myReshape);
  glutKeyboardFunc(key);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
