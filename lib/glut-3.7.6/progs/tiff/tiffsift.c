
/* Copyright (c) Mark J. Kilgard, 1997.  */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This program uses 3D texture coordinates to introduce
   "sifting" effects to warp a static mesh of textured
   geometry.  The third texture coordinate encodes a shifting
   quantity through the mesh.  By updating the texture matrix,
   the texture coordinates can be shifted based on this
   third texture coordinate.  You'll notice the image seems
   to have local vortexes scattered over the image that
   warp the image.  While the texture coordinates look dynamic,
   they are indeed quite static (frozen in a display list) and
   it is just the texture matrix that is changing to shift
   the final 2D texture coordinates. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#include <GL/glut.h>
#include <tiffio.h>     /* Sam Leffler's libtiff library. */

TIFFRGBAImage img;
uint32 *raster;
uint32 *texture;
tsize_t npixels;

float tick = 0;
float size = 0.6;
int set_timeout = 0;
int visible = 0;
int sifting = 1;
int interval = 100;

int hasABGR = 0;
int doubleBuffer = 1;
char *filename = NULL;

int tw, th;

void
animate(int value)
{
  if (visible) {
    if (sifting) {
      if (value) {
        if (sifting) {
          tick += 4 * (interval / 100.0);
        }
      }
      glutPostRedisplay();
      set_timeout = 1;
    }
  }
}

/* Setup display list with "frozen" 3D texture coordinates. */
void
generateTexturedSurface(void)
{
  static GLfloat data[8] =
  {0, 1, 0, -1, 0, -1, 0, 1};
  int i, j;

#define COLS 6
#define ROWS 6
#define TILE_TEX_W (1.0/COLS)
#define TILE_TEX_H (1.0/ROWS)

  glNewList(1, GL_COMPILE);
  glTranslatef(-COLS / 2.0 + .5, -ROWS / 2.0 + .5, 0);
  for (j = 0; j < ROWS; j++) {
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i < COLS; i++) {
      glTexCoord3f(i * TILE_TEX_W, j * TILE_TEX_H, data[(i + j) % 8]);
      glVertex2f(i - .5, j - .5);
      glTexCoord3f(i * TILE_TEX_W, (j + 1) * TILE_TEX_H, data[(i + j + 1) % 8]);
      glVertex2f(i - .5, j + .5);
    }
#if 1
    glTexCoord3f((i + 1) * TILE_TEX_W, j * TILE_TEX_H, data[(i + j) % 8]);
    glVertex2f(i + .5, j - .5);
    glTexCoord3f((i + 1) * TILE_TEX_W, (j + 1) * TILE_TEX_H, data[(i + j + 1) % 8]);
    glVertex2f(i + .5, j + .5);
    glEnd();
#endif
  }
  glEndList();
}

/* Construct an identity matrix except that the third coordinate
   can be used to "sift" the X and Y coordinates. */
void
makeSift(GLfloat m[16], float xsift, float ysift)
{
  m[0 + 4 * 0] = 1;
  m[0 + 4 * 1] = 0;
  m[0 + 4 * 2] = xsift;
  m[0 + 4 * 3] = 0;

  m[1 + 4 * 0] = 0;
  m[1 + 4 * 1] = 1;
  m[1 + 4 * 2] = ysift;
  m[1 + 4 * 3] = 0;

  m[2 + 4 * 0] = 0;
  m[2 + 4 * 1] = 0;
  m[2 + 4 * 2] = 1;
  m[2 + 4 * 3] = 0;

  m[3 + 4 * 0] = 0;
  m[3 + 4 * 1] = 0;
  m[3 + 4 * 2] = 0;
  m[3 + 4 * 3] = 1;
}

void
redraw(void)
{
  int begin, end, elapsed;
  GLfloat matrix[16];

  if (set_timeout) {
    begin = glutGet(GLUT_ELAPSED_TIME);
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();

  glScalef(size, size, size);

  glMatrixMode(GL_TEXTURE);
  makeSift(matrix, 0.02 * cos(tick / 15.0), 0.02 * sin(tick / 15.0));
  glLoadMatrixf(matrix);
  glMatrixMode(GL_MODELVIEW);

  glCallList(1);

  glPopMatrix();
  if (doubleBuffer) {
    glutSwapBuffers();
  } else {
    glFlush();
  }
  if (set_timeout) {
    set_timeout = 0;
    end = glutGet(GLUT_ELAPSED_TIME);
    elapsed = end - begin;
    if (elapsed > interval) {
      glutTimerFunc(0, animate, 1);
    } else {
      glutTimerFunc(interval - elapsed, animate, 1);
    }
  }
}

void
visibility(int state)
{
  if (state == GLUT_VISIBLE) {
    visible = 1;
    animate(0);
  } else {
    visible = 0;
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
  glutCreateWindow("tiffsift");
  glutDisplayFunc(redraw);
  glutVisibilityFunc(visibility);
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

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective( /* field of view in degree */ 40.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 70.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,30) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */

  generateTexturedSurface();

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
