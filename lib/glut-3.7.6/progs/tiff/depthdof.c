
/**

   depthdof.c: depth-of-field simulator using stencilled convolutions
      Jon Brandt, 1/97
      converted to GLUT and libtiff by Mark Kilgard, 4/97

   Compile with:

     cc -o depthdof depthdof.c -ltiff -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm
    
 **/

#include <GL/glut.h>

#include <tiffio.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

const char *DefaultImage = "face.tif";
int winWidth = 512, winHeight = 512;
char *pixels = NULL;
int stencilMax;
int kSize = 5;
int kRadius = 5 / 2;
float sharpness = -1.5;
float kernel[] =
{.05, .1, .7, .1, .05};
int passes = 5;
int contour = 0;
char focus = 160;
float spread = 1;
int focusPending = FALSE;
int splitScreen = FALSE;
int smoothDepth = FALSE;
int frame = 0;
int running = TRUE;
int cursor_x, cursor_y;
int multisample, hasConvolve = 0;

TIFFRGBAImage img;
uint32 *raster;
uint32 *texture;
size_t npixels;
int tw, th;
int angle;

int hasABGR = 0;

void drawFrame(int frame);
void initScene(int argc, char *argv[]);
void drawScene(int frame);
void drawObject(float numRep);
void changePassCount(int delta);
void setBlurMap(void);
void setKernel(void);
void resize(int w, int h);
void toggle(int *param, char *msg);

void
redraw(void)
{
  drawFrame(frame);
}

void
idle(void)
{
  frame++;
  glutPostRedisplay();
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE)
    glutIdleFunc(idle);
  else
    glutIdleFunc(NULL);
}

/* ARGSUSED1 */
void
keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:
    exit(0);
    break;
  case 'g':
    running = !running;
    if (running) {
      glutIdleFunc(idle);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  case 'c':
    toggle(&contour, NULL);
    glutPostRedisplay();
    break;
  case 's':
    toggle(&splitScreen, NULL);
    glutPostRedisplay();
    break;
  case 'd':
    toggle(&smoothDepth, "depth smoothing");
    glutPostRedisplay();
    break;
  case ' ':
    frame++;
    glutPostRedisplay();
    break;
  case '-':
    frame--;
    glutPostRedisplay();
    break;
  case 'p':
    changePassCount(1);
    glutPostRedisplay();
    break;
  case 'P':
    changePassCount(-1);
    glutPostRedisplay();
    break;
  case 'a':
    spread *= 1 / 1.2;
    setBlurMap();
    glutPostRedisplay();
    break;
  case 'A':
    spread *= 1.2;
    setBlurMap();
    glutPostRedisplay();
    break;
  case 'k':
    sharpness *= 1 / 1.2;
    printf("sharpness == %g\n", sharpness);
    setKernel();
    glutPostRedisplay();
    break;
  case 'K':
    sharpness *= 1.2;
    printf("sharpness == %g\n", sharpness);
    setKernel();
    glutPostRedisplay();
    break;
  }
}

void
mouse(int b, int state, int x, int y)
{
  if (b == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    cursor_x = x;
    cursor_y = winHeight - y;
    focusPending = TRUE;
    glutPostRedisplay();
  }
}

void
main(int argc, char *argv[])
{
  int c;

  glutInit(&argc, argv);

  while ((c = getopt(argc, argv, "p:")) != -1) {
    switch (c) {
    case 'p':
      passes = atoi(optarg);
      break;
    }
  }

  glutInitDisplayString("samples depth stencil~8 double");
  glutCreateWindow("Depth of Field via Depth-varying Convolve");
  initScene(argc - optind + 1, argv + optind - 1);
  changePassCount(0);
  printf("Key commands:\n");
  printf("          c: show/hide outer blur limit\n");
  printf("          g: toggle free running animation\n");
  printf("          s: toggle split-screen\n");
  printf("          d: toggle depth smoothing\n");
  printf("    <space>: step one frame forward\n");
  printf("          -: step one frame backward\n");
  printf("       p(P): increment(decrement) number of passes\n");
  printf("       a(A): narrow(widen) lens aperture\n");
  printf("       k(K): increase(decrease) kernel bluriness\n");
  printf("      <esc>: exit\n");
  printf("Mouse clicking changes focus depth.\n");

  glutVisibilityFunc(visible);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutDisplayFunc(redraw);
  glutReshapeFunc(resize);

  multisample = glutGet(GLUT_WINDOW_NUM_SAMPLES) > 0;
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

  if (!hasConvolve) {
    printf("\nTHIS PROGRAM IS NOT VERY INTERESTING WITHOUT THE OPENGL CONVOLVUTION EXTENSION\n\n");
  }

  glutMainLoop();
}

void
initScene(int argc, char *argv[])
{
  TIFF *tif;
  char emsg[1024];
  int bits;
  const char *fname = argc > 1 ? argv[1] : DefaultImage;

  glViewport(0, 0, winWidth, winHeight);

  glEnable(GL_TEXTURE_2D);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  tif = TIFFOpen(fname, "r");
  if (tif == NULL) {
    fprintf(stderr, "Problem showing %s\n", fname);
    exit(1);
  }
  if (TIFFRGBAImageBegin(&img, tif, 0, emsg)) {
    npixels = img.width * img.height;
    raster = (uint32 *) _TIFFmalloc((long) (npixels * sizeof(uint32)));
    if (raster != NULL) {
      if (TIFFRGBAImageGet(&img, raster, img.width, img.height) == 0) {
        TIFFError(fname, emsg);
        exit(1);
      }
    }
    TIFFRGBAImageEnd(&img);
  } else {
    TIFFError(fname, emsg);
    exit(1);
  }

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

  tw = 1 << (int) ceil(log((float) img.width) / log(2.0));
  th = 1 << (int) ceil(log((float) img.height) / log(2.0));
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
    (int) img.width, (int) img.height, GL_UNSIGNED_BYTE, raster,
    tw, th, GL_UNSIGNED_BYTE, texture);
  _TIFFfree(raster);

  if (gluBuild2DMipmaps(GL_TEXTURE_2D, 4, tw, th,
      APPROPRIATE_FORMAT, GL_UNSIGNED_BYTE, texture)) {
    fprintf(stderr, "couldn't build mip map");
    exit(1);
  }
  glGetIntegerv(GL_STENCIL_BITS, &bits);
  stencilMax = (1 << bits) - 1;
  printf("stencil max = %d\n", stencilMax);

  setBlurMap();
  setKernel();

  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  glLineStipple(4, 0xaaaa);
  glEnable(GL_LINE_STIPPLE);
  pixels = realloc(pixels, (winWidth + 4) * (winHeight + 4));
}

void
drawFrame(int frame)
{
  int pass;
  int activeWidth = splitScreen ? winWidth / 2 : winWidth;

  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1, 1, -1, 1, 1.5, 20);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

#ifdef GL_MULTISAMPLE_SGIS
  if (multisample)
    glEnable(GL_MULTISAMPLE_SGIS);
#endif
  drawScene(frame);
#ifdef GL_MULTISAMPLE_SGIS
  if (multisample)
    glDisable(GL_MULTISAMPLE_SGIS);
#endif

  if (passes > 0) {
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, winWidth, 0, winHeight, -1, 1);
    glReadPixels(0, 0, activeWidth, winHeight, GL_DEPTH_COMPONENT,
      GL_UNSIGNED_BYTE, pixels);

    if (focusPending) {
      glReadPixels(cursor_x, cursor_y, 1, 1, GL_DEPTH_COMPONENT,
        GL_UNSIGNED_BYTE, &focus);
      printf("focussing on depth == %d\n", focus);
      setBlurMap();
      focusPending = FALSE;
    }
    if (smoothDepth && hasConvolve) {
#ifdef GL_EXT_convolution
      /* convolve the depth map into alpha and read it back */
      glEnable(GL_SEPARABLE_2D_EXT);
      glRasterPos2f(kRadius, kRadius);
      glColorMask(0, 0, 0, 1);
      glDrawPixels(activeWidth, winHeight, GL_ALPHA,
        GL_UNSIGNED_BYTE, pixels);
      glColorMask(1, 1, 1, 1);
      glDisable(GL_SEPARABLE_2D_EXT);
      glReadPixels(0, 0, activeWidth, winHeight, GL_ALPHA,
        GL_UNSIGNED_BYTE, pixels);
#endif
    }
    glRasterPos2f(0, 0);
    glPixelTransferi(GL_MAP_STENCIL, GL_TRUE);
    glDrawPixels(activeWidth, winHeight, GL_STENCIL_INDEX,
      GL_UNSIGNED_BYTE, pixels);

    glPixelTransferi(GL_MAP_STENCIL, GL_FALSE);
    glEnable(GL_STENCIL_TEST);

#ifdef GL_EXT_convolution
    if (hasConvolve) {
      glEnable(GL_SEPARABLE_2D_EXT);
      for (pass = 0; pass < passes; pass++) {
        glStencilFunc(GL_LEQUAL, (pass + 1), 255);
        glRasterPos2f(kRadius, kRadius);
        glCopyPixels(0, 0, activeWidth, winHeight, GL_COLOR);
      }
      glDisable(GL_SEPARABLE_2D_EXT);
    }
#endif

    if (contour) {
      /* mark edge of in-focus region with green contour */
      glStencilFunc(GL_EQUAL, pass, 255);
      glColor3f(0, 1, 0);
      glBegin(GL_TRIANGLE_STRIP);
      glVertex2f(0, 0);
      glVertex2f(0, winHeight);
      glVertex2f(winWidth, 0);
      glVertex2f(winWidth, winHeight);
      glEnd();
    }
    glDisable(GL_STENCIL_TEST);

    if (splitScreen) {
      /* yellow divider line */
      glColor3f(1, 1, 0);
      glBegin(GL_LINE_STRIP);
      glVertex2f(activeWidth, 0);
      glVertex2f(activeWidth, winHeight);
      glEnd();
    }
  }
  glutSwapBuffers();
}

void
drawScene(int frame)
{
  glPushMatrix();
  glTranslatef(0, 0, -18);
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_TRIANGLE_STRIP);
  glColor3ub(40, 0, 140);
  glVertex2f(-15, -15);
  glVertex2f(15, -15);
  glColor3ub(52, 202, 226);
  glVertex2f(-15, 15);
  glVertex2f(15, 15);
  glEnd();
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0, -1, 0);
  glScalef(20, 20, 20);
  glRotatef(90, 1, 0, 0);
  drawObject(20);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0, 0, -5 + 3 * sinf(frame * M_PI / 180));
  glRotatef(frame * 3, 0, 1, 0);
  drawObject(1);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(cosf(frame * M_PI / 120), 0, -6 + 4 * sinf(frame * M_PI / 210));
  glRotatef(frame * 3, 1, 0, cosf(frame * M_PI / 360));
  drawObject(1);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0, 0, -5 + 3 * sinf(frame * M_PI / 300));
  drawObject(1);
  glPopMatrix();
}

void
drawObject(float n)
{
  glEnable(GL_TEXTURE_2D);
  glColor3f(1, 1, 1);
  glBegin(GL_TRIANGLE_STRIP);
  glTexCoord2f(0, 0);
  glVertex2f(-.5, -.5);
  glTexCoord2f(n, 0);
  glVertex2f(.5, -.5);
  glTexCoord2f(0, n);
  glVertex2f(-.5, .5);
  glTexCoord2f(n, n);
  glVertex2f(.5, .5);
  glEnd();
}

void
setBlurMap(void)
{
  int i;
  unsigned short blur[256];

  for (i = 0; i < 256; i++) {
    float del = (i - focus) / 255.;
    del *= del * spread * stencilMax;
    blur[i] = del < stencilMax ? del : stencilMax;
  }
  glPixelMapusv(GL_PIXEL_MAP_S_TO_S, 256, blur);
}

void
setKernel(void)
{
#ifdef GL_EXT_convolution
  float s1 = expf(sharpness), s2 = expf(2 * sharpness);
  kernel[0] = kernel[4] = s2;
  kernel[1] = kernel[3] = s1;
  kernel[2] = 1 - 2 * (s1 + s2);
  glSeparableFilter2DEXT(GL_SEPARABLE_2D_EXT, GL_LUMINANCE,
    kSize, kSize, GL_RED,
    GL_FLOAT, kernel, kernel);
#endif
}

void
toggle(int *param, char *msg)
{
  *param = !*param;
  if (msg)
    printf("%s == %d\n", msg, *param);
  drawFrame(frame);
}

void
changePassCount(int delta)
{
  passes += delta;
  if (passes < 0)
    passes = 0;
  printf("number of passes == %d\n", passes);
}

void
resize(int w, int h)
{
  winWidth = w;
  winHeight = h;
  pixels = realloc(pixels, (winWidth + 4) * (winHeight + 4));
  glViewport(0, 0, winWidth, winHeight);
}
