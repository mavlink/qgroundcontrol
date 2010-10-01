
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees and is
   provided without guarantee or warrantee expressed or implied. This
   program is -not- in the public domain. */

/* X compile line: cc -o scalebias scalebias.c -ltiff -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

/* This program requires Sam Leffler's libtiff library and GLUT. */

/* scalebias demonstrates how an "in place" scale & bias of pixels in the
   frame buffer can often be accomplished faster with OpenGL blending
   extensions instead of using the naive glCopyPixels with glPixelTransfer
   used to do the scale and bias.

   The blending approach requires the "blend subtract" EXT extension in
   order to perform negative biases.  You could use this approach without
   the "blend subtract" extension if you never need to do negative
   biases.

   NOTE: This blending approach does not allow negative scales.  The
   blending approach also fails if the partial scaling or biasing results
   leave the 0.0 to 1.0 range (example, scale=5.47, bias=-1.2).

   This technique can be valuable when you want to perform post-texture
   filtering scaling and biasing (say for volume rendering or image processing),
   but your hardware lacks texture lookup tables.

   To give you an idea of the speed advantage of this "in place" blending
   technique for doing scales and biases, on an SGI O2, this program
   runs 8 to 40 times faster with a greater than 1.0 scaling factor when
   using the blending mode instead of using glCopyPixels.  The performance
   improvement depends on the number of pixels scaled or biased. */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>
#include <tiffio.h>     /* Sam Leffler's libtiff library. */

TIFFRGBAImage img;
uint32 *raster, *texture;
tsize_t npixels;
int imgwidth, imgheight;
int tw, th;

int hasABGR = 0, hasBlendSubtract = 0;
int doubleBuffer = 1;
char *filename = NULL;
int ax = 10, ay = -10;
int luminance = 0;
int useBlend = 1;
int timing = 0;
int height;

GLfloat scale = 1.0, bias = 0.0, zoom = 1.0;

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  height = h;
}

void
drawImage(void)
{
  glPushMatrix();
  glTranslatef(ax, -ay + imgheight * zoom, 0);
  glScalef(zoom * imgwidth, zoom * imgheight, 1);
  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex2i(0, 0);
  glTexCoord2i(1, 0);
  glVertex2i(1, 0);
  glTexCoord2i(1, -1);
  glVertex2i(1, -1);
  glTexCoord2i(0, -1);
  glVertex2i(0, -1);
  glEnd();
  glPopMatrix();
}

void
display(void)
{
  int start, end;

  /* Clear the color buffer. */
  glClear(GL_COLOR_BUFFER_BIT);

  glColor3f(1.0, 1.0, 1.0);  /* Modulate texture with white. */
  glEnable(GL_TEXTURE_2D);
  drawImage();

  if (timing) {
    /* Avoid timing the clear and original draw image speed. */
    glFinish();
    start = glutGet(GLUT_ELAPSED_TIME);
  }

  /* Scale and bias via . */
  if (bias != 0.0 || scale != 1.0) {
    glDisable(GL_TEXTURE_2D);

    /* Other things you might want to make sure are disabled. */
    /* glDisable(GL_LIGHTING); */
    /* glDisable(GL_DEPTH_TEST); */

    if (useBlend && hasBlendSubtract) {

      /* NOTE: The blending approach does not allow negative
         scales.  The blending approach also fails if the
         partial scaling or biasing results leave the 0.0 to
         1.0 range (example, scale=5.47, bias=-1.2). */

      glEnable(GL_BLEND);
      if (scale > 1.0) {
        float remainingScale;

        remainingScale = scale;
#ifdef GL_EXT_blend_subtract
        glBlendEquationEXT(GL_FUNC_ADD_EXT);
#endif
        glBlendFunc(GL_DST_COLOR, GL_ONE);
        if (remainingScale > 2.0) {
          /* Clever cascading approach.  Example: if the
             scaling factor was 9.5, do 3 "doubling" blends
             (8x), then scale by the remaining 1.1875. */
          glColor4f(1, 1, 1, 1);
          while (remainingScale > 2.0) {
            drawImage();
            remainingScale /= 2.0;
          }
        }
        glColor4f(remainingScale - 1,
          remainingScale - 1, remainingScale - 1, 1);
        drawImage();
        glBlendFunc(GL_ONE, GL_ONE);
        if (bias != 0) {
          if (bias > 0) {
            glColor4f(bias, bias, bias, 0.0);
          } else {
#ifdef GL_EXT_blend_subtract
            glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
#endif
            glColor4f(-bias, -bias, -bias, 0.0);
          }
          drawImage();
        }
      } else {
        if (bias > 0) {
#ifdef GL_EXT_blend_subtract
          glBlendEquationEXT(GL_FUNC_ADD_EXT);
#endif
          glColor4f(bias, bias, bias, scale);
        } else {
#ifdef GL_EXT_blend_subtract
          glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
#endif
          glColor4f(-bias, -bias, -bias, scale);
        }
        glBlendFunc(GL_ONE, GL_SRC_ALPHA);
        drawImage();
      }
      glDisable(GL_BLEND);
    } else {
      glPixelTransferf(GL_RED_SCALE, scale);
      glPixelTransferf(GL_GREEN_SCALE, scale);
      glPixelTransferf(GL_BLUE_SCALE, scale);
      glPixelTransferf(GL_RED_BIAS, bias);
      glPixelTransferf(GL_GREEN_BIAS, bias);
      glPixelTransferf(GL_BLUE_BIAS, bias);
      glRasterPos2i(0, 0);
      glBitmap(0, 0, 0, 0, ax, -ay, NULL);
      glCopyPixels(ax, -ay,
        ceil(imgwidth * zoom), ceil(imgheight * zoom), GL_COLOR);
      glPixelTransferf(GL_RED_SCALE, 1.0);
      glPixelTransferf(GL_GREEN_SCALE, 1.0);
      glPixelTransferf(GL_BLUE_SCALE, 1.0);
      glPixelTransferf(GL_RED_BIAS, 0.0);
      glPixelTransferf(GL_GREEN_BIAS, 0.0);
      glPixelTransferf(GL_BLUE_BIAS, 0.0);
    }
  }
  if (timing) {
    glFinish();
    end = glutGet(GLUT_ELAPSED_TIME);
    printf("time = %d milliseconds\n", end - start);
  }
  /* Swap the buffers if necessary. */
  if (doubleBuffer) {
    glutSwapBuffers();
  } else {
    glFlush();
  }
}

static int moving = 0, ox, oy;

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {

      /* Left mouse button press.  Update last seen mouse
         position. And set "moving" true since button is
         pressed. */
      ox = x;
      oy = y;
      moving = 1;

    } else {

      /* Left mouse button released; unset "moving" since
         button no longer pressed. */
      moving = 0;

    }
  }
}

void
motion(int x, int y)
{
  /* If there is mouse motion with the left button held down. */
  if (moving) {

    /* Figure out offset from the last mouse position seen. */
    ax += (x - ox);
    ay += (y - oy);

    /* Request a window redraw. */
    glutPostRedisplay();

    /* Update last seen mouse position. */
    ox = x;
    oy = y;
  }
}

void
updateTitle(void)
{
  char title[200];

  sprintf(title, "Scale (%.2f) & Bias (%.1f) via %s", scale, bias,
    useBlend ? "Blend" : "Copy");
  glutSetWindowTitle(title);
}

void
option(int value)
{
  switch (value) {
  case 6:
    bias += 0.1;
    break;
  case 7:
    bias -= 0.1;
    break;
  case 8:
    scale *= 1.1;
    break;
  case 9:
    scale *= 0.9;
    break;
  case 10:
    scale = 1.0;
    bias = 0.0;
    break;
  case 11:
    if (hasBlendSubtract) {
      useBlend = 1 - useBlend;
    }
    break;
  case 12:
    zoom += 0.2;
    break;
  case 13:
    zoom -= 0.2;
    break;
  case 14:
    timing = 1 - timing;
    break;
  case 666:
    exit(0);
    break;
  }
  updateTitle();
  glutPostRedisplay();
}

/* ARGSUSED1 */
void
special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_UP:
    option(6);
    break;
  case GLUT_KEY_DOWN:
    option(7);
    break;
  case GLUT_KEY_LEFT:
    option(9);
    break;
  case GLUT_KEY_RIGHT:
    option(8);
    break;
  case GLUT_KEY_HOME:
    option(10);
    break;
  case GLUT_KEY_INSERT:
    option(11);
    break;
  case GLUT_KEY_PAGE_UP:
    option(12);
    break;
  case GLUT_KEY_PAGE_DOWN:
    option(13);
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
    fprintf(stderr, "usage: scalebias [GLUT-options] [-sb] TIFF-file\n");
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
  imgwidth = (int) img.width;
  imgheight = (int) img.height;
  glutInitWindowSize(imgwidth * 1.5, imgheight * 1.5);
  glutCreateWindow("");
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutSpecialFunc(special);
#ifdef GL_EXT_abgr
  if (glutExtensionSupported("GL_EXT_abgr")) {
    hasABGR = 1;
  }
#endif
#ifdef GL_EXT_blend_subtract
  if (glutExtensionSupported("GL_EXT_blend_subtract")) {
    hasBlendSubtract = 1;
  }
#endif
  if (!hasBlendSubtract) {
    printf("\nThis program needs the blend subtract extension for\n");
    printf("fast blending-base in-place scaling & biasing.  Since\n");
    printf("the extension is not available, using the slower\n");
    printf("glCopyPixels approach.\n\n");
    useBlend = 0;
  }
  /* If cannot directly display ABGR format, we need to reverse
     the component ordering in each pixel. :-( */
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
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  /* Linear sampling within a mipmap level. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_NEAREST);

  /* A TIFF file could be any size; OpenGL textures are allowed
     to have a width and height that is a power of two (32, 64,
     128, etc.). To maximize the use of available texture
     memory, we scale the image to gluScaleImage to the next
     larger power of 2 width or height dimension (not exceeding
     512, don't want to use too much texture memory!).  This
     rescaling can result in a bit of image bluring because of
     the resampling done by gluScaleImage.  An alternative would
     be to change the texture coordinates  to only use a portion
     texture area. */

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

  /* Build mipmaps for the texture image.  Since we are not
     scaling the image (we easily could by calling glScalef),
     creating mipmaps is not really useful, but it is done just
     to show how easily creating mipmaps is. */
  gluBuild2DMipmaps(GL_TEXTURE_2D, 4, tw, th,
    APPROPRIATE_FORMAT, GL_UNSIGNED_BYTE,
    texture);

  glutCreateMenu(option);
  glutAddMenuEntry("Increase bias (Up)", 6);
  glutAddMenuEntry("Decrease bias (Down)", 7);
  glutAddMenuEntry("Increase scale (Right)", 8);
  glutAddMenuEntry("Decrease scale (Left)", 9);
  glutAddMenuEntry("Reset scale & bias (Home)", 10);
  if (hasBlendSubtract) {
    glutAddMenuEntry("Toggle blend/copy (Insert)", 11);
  }
  glutAddMenuEntry("Zoom up (PageUp)", 12);
  glutAddMenuEntry("Zoom down (PageDown)", 13);
  glutAddMenuEntry("Toggle timing", 14);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  /* Use a gray background so TIFF images with black
     backgrounds will show against textiff's background. */
  glClearColor(0.2, 0.2, 0.2, 1.0);
  updateTitle();
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
