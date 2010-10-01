/* winpos.c */

/*
 * Example of how to use the GL_MESA_window_pos extension.
 *
 * This program is in the public domain
 *
 * Brian Paul
 */

/* Conversion to GLUT by Mark J. Kilgard */

/* The GL_MESA_window_pos extension lets you set the OpenGL raster position
   (used for positioning output from glBitmap and glDrawPixels) based on
   window coordinates (assuming an origin at the lower-left window corner).
   Mesa has an extension to do this operation quickly, but the program
   will emulate the raster position update if the extension is not available.
   See the implementation of glWindowPos4fMESAemulate. -mjk */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.14159265
#endif

#define WIDTH 16
#define HEIGHT 16
int sizeX = WIDTH, sizeY = HEIGHT;
GLubyte data[WIDTH * HEIGHT * 3];

/* glWindowPos4fMESAemulate & glWindowPos2fMESAemulate are lifted from the
   Mesa 2.0 src/winpos.c emulation code. -mjk */

/*
 * OpenGL implementation of glWindowPos*MESA()
 */
void 
glWindowPos4fMESAemulate(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
  GLfloat fx, fy;

  /* Push current matrix mode and viewport attributes */
  glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

  /* Setup projection parameters */
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glDepthRange(z, z);
  glViewport((int) x - 1, (int) y - 1, 2, 2);

  /* set the raster (window) position */
  fx = x - (int) x;
  fy = y - (int) y;
  glRasterPos4f(fx, fy, 0.0, w);

  /* restore matrices, viewport and matrix mode */
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glPopAttrib();
}

void
glWindowPos2fMESAemulate(GLfloat x, GLfloat y)
{
  glWindowPos4fMESAemulate(x, y, 0, 1);
}

/* Make cheesy pixel image for glDrawPixels. */
static void
init(void)
{
  int i, j;
  static char *pattern[16] =
  {
    "    ........    ",
    "   ..........   ",
    "  ...xx..xx...  ",
    "  ............  ",
    "   ...oooo...   ",
    "*   ........    ",
    "*oooooooooooooo*",
    "      oooo     *",
    "      oooo      ",
    "      xxxx      ",
    "      xxxx      ",
    "     xx  xx     ",
    "    xx    xx    ",
    "   xx      xx   ",
    "   **      **   ",
    "  ***      ***  ",
  };
  GLubyte red, green, blue;

  /* Generate a pixel image from pattern. */
  for (i = 0; i < HEIGHT; i++) {
    for (j = 0; j < WIDTH; j++) {
      switch (pattern[HEIGHT - i - 1][j]) {
      case '.':
        red = 0xff;
        green = 0xff;
        blue = 0x00;
        break;
      case 'o':
        red = 0xff;
        green = 0x00;
        blue = 0x00;
        break;
      case 'x':
        red = 0x00;
        green = 0xff;
        blue = 0x00;
        break;
      case '*':
        red = 0xff;
        green = 0x00;
        blue = 0xff;
        break;
      case ' ':
        red = 0x00;
        green = 0x00;
        blue = 0x00;
        break;
      }
      data[(i * WIDTH + j) * 3 + 0] = red;
      data[(i * WIDTH + j) * 3 + 1] = green;
      data[(i * WIDTH + j) * 3 + 2] = blue;
    }
  }
}

#ifdef GL_MESA_window_pos
int emulate;
#endif

static void
draw(void)
{
  GLfloat angle;

  glClear(GL_COLOR_BUFFER_BIT);

  for (angle = -45.0; angle <= 135.0; angle += 10.0) {
    GLfloat x = 50.0 + 200.0 * cos(angle * M_PI / 180.0);
    GLfloat y = 50.0 + 200.0 * sin(angle * M_PI / 180.0);

#ifdef GL_MESA_window_pos
    /* Don't need to worry about the modelview or projection
       matrices!!! */
    if (!emulate)
      glWindowPos2fMESA(x, y);
    else
#endif
      glWindowPos2fMESAemulate(x, y);
    glDrawPixels(sizeX, sizeY, GL_RGB,
      GL_UNSIGNED_BYTE, data);
  }
  glFlush();
}

/* ARGSUSED1 */
static void
key(unsigned char k, int x, int y)
{
  switch (k) {
  case 27:
    exit(0);
  }
}

int
main(int argc, char *argv[])
{
  glutInitWindowSize(500, 500);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);

  glutCreateWindow("winpos");

  if (glutExtensionSupported("GL_MESA_window_pos")) {
    printf("OpenGL implementation supports Mesa's GL_MESA_window_pos extension.");
#ifdef GL_MESA_window_pos
    emulate = 0;
#else
    printf("(Not compiled to support the extension.)\n");
    printf("Emulating...\n");
#endif
  } else {
    printf("Sorry, GL_MESA_window_pos extension not available.\n");
    printf("Emulating...\n");
#ifdef GL_MESA_window_pos
    emulate = 1;
#else
    printf("(Not compiled to support the extension even if available.)\n");
#endif
  }

  init();

  glutDisplayFunc(draw);
  glutKeyboardFunc(key);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
