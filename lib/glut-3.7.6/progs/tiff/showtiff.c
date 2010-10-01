
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* X compile line: cc -o showtiff showtiff.c -ltiff -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

/* showtiff is a simple TIFF file viewer using Sam Leffler's libtiff, GLUT,
   and OpenGL.  If OpenGL the image processing extensions for convolution and
   color matrix are supported by your OpenGL implementation, showtiff will let
   you blur, sharpen, edge detect, and color convert to grayscale the TIFF
   file.  You can also move around the image within the window. */

#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <tiffio.h>     /* Sam Leffler's libtiff library. */

TIFFRGBAImage img;
uint32 *raster;
tsize_t npixels;
int imgwidth, imgheight;

int hasABGR = 0;
int hasConvolve = 0;
int hasColorMatrix = 0;
int doubleBuffer = 1;
char *filename = NULL;
int ax = 0, ay = 0;
int luminance = 0;

GLfloat rgbBlur[7][7][3];
GLfloat rgbEdgeDetect[3][3][3];
GLfloat rgbSharpen[3][3][3];

void
initKernels(void)
{
  int x, y, c;

  /* A 7x7 "blurring" convolution kernel.  This kernel will uniformly spread
     each pixel with its surrounding 7x7 pixels.  The sum of the kernel
     elements is 1.0. */
  for (x = 0; x < 7; x++) {
    for (y = 0; y < 7; y++) {
      for (c = 0; c < 3; c++) {
        rgbBlur[x][y][c] = 1.0 / 49.0;
      }
    }
  }

  /* A 3x3 edge detection covolution kernel.  The kernel is shown below.
     Notice how the elements of the kernel add up to zero. */
  /**********
    -1 -1 -1
    -1  8 -1
    -1 -1 -1
   **********/
  for (x = 0; x < 3; x++) {
    for (y = 0; y < 3; y++) {
      for (c = 0; c < 3; c++) {
        rgbEdgeDetect[x][y][c] = -1.0;
      }
    }
  }
  for (c = 0; c < 3; c++) {
    rgbEdgeDetect[1][1][c] = 8.0;
  }

  /* A 3x3 "sharpening" convolution kernel.  The kernel is shown below.
     Notice how surrounding information is subtracted out of the central
     pixel while the central pixel is weighted heavily.  The sum of the
     kernel elements is 1.0. */
  /*************
    -.5 -.5 -.5
    -.5  5  -.5
    -.5 -.5 -.5
   *************/
  for (x = 0; x < 3; x++) {
    for (y = 0; y < 3; y++) {
      for (c = 0; c < 3; c++) {
        rgbSharpen[x][y][c] = -0.5;
      }
    }
  }
  for (c = 0; c < 3; c++) {
    rgbSharpen[1][1][c] = 5.0;
  }
}

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

#ifdef GL_EXT_abgr
#define APPROPRIATE_FORMAT (hasABGR ? GL_ABGR_EXT : GL_RGBA)
#else
#define APPROPRIATE_FORMAT GL_RGBA
#endif

  /* Re-blit the image. */
  glDrawPixels(imgwidth, imgheight,
    APPROPRIATE_FORMAT, GL_UNSIGNED_BYTE,
    raster);

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

void
option(int value)
{
  /* RGB to NTSC luminance color conversion matrix. */
  static GLfloat rgb2luminance[16] =
  {
    0.30, 0.30, 0.30, 0.30,  /* 30% red. */
    0.59, 0.59, 0.59, 0.59,  /* 59% green. */
    0.11, 0.11, 0.11, 0.11,  /* 11% blue. */
    0.00, 0.00, 0.00, 0.00  /* 0% alpha. */
  };

  switch (value) {
  case 1:
#ifdef GL_EXT_convolution
    glDisable(GL_CONVOLUTION_2D_EXT);
    break;
  case 2:
    glEnable(GL_CONVOLUTION_2D_EXT);
    glConvolutionFilter2DEXT(GL_CONVOLUTION_2D_EXT,
      GL_RGB, 7, 7, GL_RGB, GL_FLOAT, rgbBlur);
    break;
  case 3:
    glEnable(GL_CONVOLUTION_2D_EXT);
    glConvolutionFilter2DEXT(GL_CONVOLUTION_2D_EXT,
      GL_RGB, 3, 3, GL_RGB, GL_FLOAT, rgbSharpen);
    break;
  case 4:
    glEnable(GL_CONVOLUTION_2D_EXT);
    glConvolutionFilter2DEXT(GL_CONVOLUTION_2D_EXT,
      GL_RGB, 3, 3, GL_RGB, GL_FLOAT, rgbEdgeDetect);
#endif
    break;
#ifdef GL_SGI_color_matrix
  case 5:
    luminance = 1 - luminance;  /* Toggle. */
    glMatrixMode(GL_COLOR);
    if (luminance) {
      glLoadMatrixf(rgb2luminance);
    } else {
      glLoadIdentity();
    }
    glMatrixMode(GL_MODELVIEW);
#endif
    break;
  case 666:
    exit(0);
    break;
  }
  glutPostRedisplay();
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
    fprintf(stderr, "usage: showtiff [GLUT-options] [-sb] TIFF-file\n");
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
  glutInitWindowSize(imgwidth, imgheight);
  glutCreateWindow("showtiff");
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
#ifdef GL_EXT_abgr
  if (glutExtensionSupported("GL_EXT_abgr")) {
    hasABGR = 1;
  }
#endif
#ifdef GL_EXT_convolution
  if (glutExtensionSupported("GL_EXT_convolution")) {
    hasConvolve = 1;
  } else {
    while (glGetError() != GL_NO_ERROR);  /* Clear any OpenGL errors. */

    /* The following glDisable would be a no-op whether done on a freshly
       initialized OpenGL context whether convolution is supported or not.
       The only difference should be an OpenGL error should be reported if
       the GL_CONVOLUTION_2D_EXT is not understood (ie, convolution is not
       supported at all). */
    glDisable(GL_CONVOLUTION_2D_EXT);

    if (glGetError() == GL_NO_ERROR) {
      /* RealityEngine only partially implements the convolve extension and
         hence does not advertise the extension in its extension string (See
         MACHINE DEPENDENCIES section of the glConvolutionFilter2DEXT man
         page). We limit this program to use only the convolve functionality
         supported by RealityEngine so we test if OpenGL lets us enable
         convolution without an error (the indication that convolution is
         partially supported). */
      hasConvolve = 1;
    }
    /* Clear any further OpenGL errors (hopefully there should have only been 

       one or zero though). */
    while (glGetError() != GL_NO_ERROR);
  }
#endif
#ifdef GL_SGI_color_matrix
  if (glutExtensionSupported("GL_SGI_color_matrix")) {
    hasColorMatrix = 1;
  }
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
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  initKernels();
  glutCreateMenu(option);
  glutAddMenuEntry("Normal", 1);
#ifdef GL_EXT_convolution
  if (hasConvolve) {
    glutAddMenuEntry("7x7 Blur", 2);
    glutAddMenuEntry("3x3 Sharpen", 3);
    glutAddMenuEntry("3x3 Edge Detect", 4);
  }
#endif
#ifdef GL_SGI_color_matrix
  if (hasColorMatrix) {
    glutAddMenuEntry("Toggle Luminance/RGB", 5);
  }
#endif
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  /* Use a gray background so TIFF images with black backgrounds will
     show against textiff's background. */
  glClearColor(0.2, 0.2, 0.2, 1.0);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
