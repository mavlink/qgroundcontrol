
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* X compile line: cc -o showtxf showtxf.c texfont.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <GL/glut.h>

#include "TexFont.h"

unsigned char *raster;
int imgwidth, imgheight;
int max_ascent, max_descent;
int len;
int ax = 0, ay = 0;
int doubleBuffer = 1, verbose = 0;
char *filename = "default.txf";
TexFont *txf;

/* If resize is called, enable drawing into the full screen area
   (glViewport). Then setup the modelview and projection matrices to map 2D
   x,y coodinates directly onto pixels in the window (lower left origin).
   Then set the raster position (where the image would be drawn) to be offset
   from the upper left corner, and then offset by the current offset (using a
   null glBitmap). */
void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0, h - imgheight, 0);
  glRasterPos2i(0, 0);
  glBitmap(0, 0, 0, 0, ax, -ay, NULL);
}

void
display(void)
{
  /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT);

  /* Re-blit the image. */
  glDrawPixels(imgwidth, imgheight,
    GL_LUMINANCE, GL_UNSIGNED_BYTE,
    txf->teximage);

  /* Swap the buffers if necessary. */
  if (doubleBuffer) {
    glutSwapBuffers();
  }
}

static int moving = 0, ox, oy;

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {

      /* Left mouse button press.  Update last seen mouse position. And set
         "moving" true since button is pressed. */
      ox = x;
      oy = y;
      moving = 1;

    } else {

      /* Left mouse button released; unset "moving" since button no longer
         pressed. */
      moving = 0;

    }
  }
}

void
motion(int x, int y)
{
  /* If there is mouse motion with the left button held down... */
  if (moving) {

    /* Figure out the offset from the last mouse position seen. */
    ax += (x - ox);
    ay += (y - oy);

    /* Offset the raster position based on the just calculated mouse position
       delta.  Use a null glBitmap call to offset the raster position in
       window coordinates. */
    glBitmap(0, 0, 0, 0, x - ox, oy - y, NULL);

    /* Request a window redraw. */
    glutPostRedisplay();

    /* Update last seen mouse position. */
    ox = x;
    oy = y;
  }
}

int
main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-sb")) {
      doubleBuffer = 0;
    } else if (!strcmp(argv[i], "-v")) {
      verbose = 1;
    } else {
      filename = argv[i];
    }
  }
  if (filename == NULL) {
    fprintf(stderr, "usage: showtxf [GLUT-options] [-sb] [-v] txf-file\n");
    exit(1);
  }

  txf = txfLoadFont(filename);
  if (txf == NULL) {
    fprintf(stderr, "Problem loading %s\n", filename);
    exit(1);
  }

  imgwidth = txf->tex_width;
  imgheight = txf->tex_height;

  if (doubleBuffer) {
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  } else {
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  }
  glutInitWindowSize(imgwidth, imgheight);
  glutCreateWindow(filename);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  /* Use a gray background so teximage with black backgrounds will show
     against showtxf's background. */
  glClearColor(0.2, 0.2, 0.2, 1.0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
