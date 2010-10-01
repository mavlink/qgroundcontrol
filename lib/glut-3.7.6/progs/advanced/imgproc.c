
/* imgproc.c - by David Blythe, SGI */

/* Examples of various image processing operations coded as OpenGL
   accumulation buffer operations.  This allows extremely fast   
   image processing on machines with hardware accumulation buffers
   (RealityEngine, InfiniteReality, VGX). */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <GL/glut.h>
#include "texture.h"

static unsigned *image, *null;
static int width, height, components;
static void (*func) (void);
static float alpha = 1.;
static float luma = .5;
static int reset = 1;
static int format = GL_RGBA;

void
brighten(void)
{
  if (reset) {
    memset(null, 0, width * height * sizeof *null);
    reset = 0;
  }
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, image);
  glAccum(GL_LOAD, alpha / 2.);
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, null);
  glAccum(GL_ACCUM, (1 - alpha) / 2.);
  glAccum(GL_RETURN, 2.0);
}

void
saturate(void)
{
  if (reset) {
    memset(null, 0xff, width * height * sizeof *null);
    reset = 0;
  }
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, image);
  glAccum(GL_LOAD, alpha / 2.);
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, null);
  glAccum(GL_ACCUM, (1 - alpha) / 2.);
  glAccum(GL_RETURN, 2.0);
}

void
contrast(void)
{
  if (reset) {
    memset(null, luma, width * height * sizeof *null);
    reset = 0;
  }
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, image);
  glAccum(GL_LOAD, alpha / 2.);
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, null);
  glAccum(GL_ACCUM, (1 - alpha) / 2.);
  glAccum(GL_RETURN, 2.0);
}

void
balance(void)
{
  if (reset) {
    memset(null, luma, width * height * sizeof *null);
    reset = 0;
  }
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, image);
  glAccum(GL_LOAD, alpha / 2.);
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, null);
  glAccum(GL_ACCUM, (1 - alpha) / 2.);
  glAccum(GL_RETURN, 2.0);
}

void
sharpen(void)
{
  if (reset) {
    gluScaleImage(format, width, height, GL_UNSIGNED_BYTE, image,
      width / 4, height / 4, GL_UNSIGNED_BYTE, null);
    gluScaleImage(format, width / 4, height / 4, GL_UNSIGNED_BYTE, null,
      width, height, GL_UNSIGNED_BYTE, null);
    reset = 0;
  }
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, image);
  glAccum(GL_LOAD, alpha / 2.);
  glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, null);
  glAccum(GL_ACCUM, (1 - alpha) / 2.);
  glAccum(GL_RETURN, 2.0);
}

void 
set_brighten(void)
{
  func = brighten;
  reset = 1;
  printf("brighten\n");
}

void 
set_saturate(void)
{
  func = saturate;
  reset = 1;
  printf("saturate\n");
}

void 
set_contrast(void)
{
  func = contrast;
  reset = 1;
  printf("contrast\n");
}

void 
set_balance(void)
{
  func = balance;
  reset = 1;
  printf("balance\n");
}

void 
set_sharpen(void)
{
  func = sharpen;
  reset = 1;
  printf("sharpen\n");
}

void
help(void)
{
  printf("'h'   - help\n");
  printf("'b'   - brighten\n");
  printf("'s'   - saturate\n");
  printf("'c'   - contrast\n");
  printf("'z'   - sharpen\n");
  printf("'a'   - color balance\n");
  printf("left mouse     - increase alpha\n");
  printf("middle mouse   - decrease alpha\n");
}

void
init(char *filename)
{
  double l = 0;
  int i;

  func = brighten;

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
        if (i & 1)
          image[i + j * width] = 0xff;
        else
          image[i + j * width] = 0xff00;
        if (j & 1)
          image[i + j * width] |= 0xff0000;
      }

  }
  null = (unsigned *) malloc(width * height * sizeof *image);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glClearColor(.25, .25, .25, .25);

  /* compute luminance */
  for (i = 0; i < width * height; i++) {
    GLubyte *p = (GLubyte *) (image + i);
    double r = p[0] / 255.;
    double g = p[1] / 255.;
    double b = p[2] / 255.;
    l += r * .3086 + g * .0820 + b * .114;
  }
  luma = l / (width * height);
  printf("average luminance = %f\n", luma);
}

void
display(void)
{
  printf("alpha = %f\n", alpha);
  glClear(GL_COLOR_BUFFER_BIT);
  (*func) ();
  glutSwapBuffers();
}

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0., (GLdouble) width, 0., (GLdouble) height, -1., 1.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y)
{
  switch (key) {
  case 'b':
    set_brighten();
    break;
  case 's':
    set_saturate();
    break;
  case 'c':
    set_contrast();
    break;
  case 'z':
    set_sharpen();
    break;
  case 'a':
    set_balance();
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

/* ARGSUSED2 */
void
mouse(int button, int state, int x, int y)
{
  if (state == GLUT_DOWN) {
    switch (button) {
    case GLUT_LEFT_BUTTON:
      alpha += .1;
      break;
    case GLUT_MIDDLE_BUTTON:
      alpha -= .1;
      break;
    case GLUT_RIGHT_BUTTON:
      break;
    }
  }
  glutPostRedisplay();
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_RGBA | GLUT_ACCUM | GLUT_DOUBLE);
  (void) glutCreateWindow("imgproc");
  init(argv[1]);
  glutDisplayFunc(display);
  glutKeyboardFunc(key);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
