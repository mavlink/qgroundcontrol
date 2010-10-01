
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* X compile line: cc -o textiff textiff.c -ltiff -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

/* textiff is a simple TIFF file viewer using Sam Leffler's libtiff, GLUT,
   and OpenGL.  Unlike the showtiff example that simply uses glDrawPixels to
   draw the image, textiff loads the texture into texture memory and then
   renders the image as a textured polygon.  This enables fast hardware
   accelerated image rotates.  Use the left and right arrow keys to rotate
   the texture arround. */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>
#include <tiffio.h>     /* Sam Leffler's libtiff library. */

TIFFRGBAImage img;
uint32 *raster;
uint32 *texture;
tsize_t npixels;
int tw, th;
int angle;

int hasABGR = 0;
int doubleBuffer = 1;
char *filename = NULL;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glPushMatrix();

  /* Rotate by angle. */
  glRotatef(angle, 0, 0, 1);

  /* Draw a rectangle onto which the TIFF image will get textured.  Notice
     that the texture coordinates are set to each corner of the texture
     image. */
  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex2i(-1, -1);
  glTexCoord2i(1, 0);
  glVertex2i(1, -1);
  glTexCoord2i(1, 1);
  glVertex2i(1, 1);
  glTexCoord2i(0, 1);
  glVertex2i(-1, 1);
  glEnd();

  glPopMatrix();
  if (doubleBuffer) {
    glutSwapBuffers();
  }
}

/* If the left or right arrows are pressed, rotate five degrees either
   direction and request a redraw of the window. */
/* ARGSUSED1 */
void
special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_LEFT:
    angle -= 5;
    angle %= 360;
    glutPostRedisplay();
    break;
  case GLUT_KEY_RIGHT:
    angle += 5;
    angle %= 360;
    glutPostRedisplay();
    break;
  }
}

int
main(int argc, char **argv)
{
  TIFF *tif;
  char emsg[1024];
  int i;

  glutInit(&argc, argv);
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-sb")) {
      doubleBuffer = 0;
    } else {
      filename = argv[i];
    }
  }
  if (filename == NULL) {
    fprintf(stderr, "usage: textiff [GLUT-options] [-sb] TIFF-file\n");
    exit(1);
  }
  tif = TIFFOpen(filename, "r");
  if (tif == NULL) {
    fprintf(stderr, "Problem showing %s\n", filename);
    exit(1);
  }
  if (TIFFRGBAImageBegin(&img, tif, 0, emsg)) {
    npixels = (tsize_t) (img.width * img.height);
    raster = (uint32 *) _TIFFmalloc(npixels * (tsize_t) sizeof(uint32));
    if (raster != NULL) {
      if (TIFFRGBAImageGet(&img, raster, img.width, img.height) == 0) {
        TIFFError(filename, emsg);
        exit(1);
      }
    }
    TIFFRGBAImageEnd(&img);
  } else {
    TIFFError(filename, emsg);
    exit(1);
  }
  if (doubleBuffer) {
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  } else {
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
  }
  glutInitWindowSize((int) img.width, (int) img.height);
  glutCreateWindow("textiff");
  glutDisplayFunc(display);
  glutSpecialFunc(special);
#ifdef GL_EXT_abgr
  if (glutExtensionSupported("GL_EXT_abgr"))
    hasABGR = 1;
#else
  hasABGR = 0;
#endif
  /* If cannot directly display ABGR format, we need to reverse the component
     ordering in each pixel. :-( */
  if (!hasABGR) {
    int i;

    for (i = 0; i < npixels; i++) {
      register unsigned char *cp = (unsigned char *) &raster[i];
      int t;

      t = cp[3];
      cp[3] = cp[0];
      cp[0] = t;
      t = cp[2];
      cp[2] = cp[1];
      cp[1] = t;
    }
  }
  /* OpenGL's default unpack (and pack) alignment is 4.  In the case of the
     data returned by libtiff which is already aligned on 32-bit boundaries,
     setting the pack to 1 isn't strictly necessary. */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  gluOrtho2D(-1, 1, -1, 1);

  /* Linear sampling within a mipmap level. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_NEAREST);

  glEnable(GL_TEXTURE_2D);

  /* A TIFF file could be any size; OpenGL textures are allowed to have a
     width and height that is a power of two (32, 64, 128, etc.). To maximize
     the use of available texture memory, we scale the image to gluScaleImage
     to the next larger power of 2 width or height dimension (not exceeding
     512, don't want to use too much texture memory!).  This rescaling can
     result in a bit of image bluring because of the resampling done by
     gluScaleImage.  An alternative would be to change the texture coordinates 
     to only use a portion texture area. */

  tw = 1 << (int) ceil(log(img.width) / log(2.0));
  th = 1 << (int) ceil(log(img.height) / log(2.0));
  if (tw > 512)
    tw = 512;
  if (th > 512)
    th = 512;
  texture = (uint32 *) malloc(sizeof(GLubyte) * 4 * tw * th);

#ifdef GL_EXT_abgr
#define APPROPRIATE_FORMAT (hasABGR ? GL_ABGR_EXT : GL_RGBA)
#else
#define APPROPRIATE_FORMAT GL_RGBA
#endif

  gluScaleImage(APPROPRIATE_FORMAT,
    (GLsizei) img.width, (GLsizei) img.height, GL_UNSIGNED_BYTE, raster,
    tw, th, GL_UNSIGNED_BYTE, texture);
  _TIFFfree(raster);

  /* Build mipmaps for the texture image.  Since we are not scaling the image
     (we easily could by calling glScalef), creating mipmaps is not really
     useful, but it is done just to show how easily creating mipmaps is. */
  gluBuild2DMipmaps(GL_TEXTURE_2D, 4, tw, th,
    APPROPRIATE_FORMAT, GL_UNSIGNED_BYTE,
    texture);

  /* Use a gray background so TIFF images with black backgrounds will
     show against textiff's background. */
  glClearColor(0.2, 0.2, 0.2, 1.0);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
