#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __sgi
#define trunc(x) ((double)((int)(x)))
#endif

#define RW 0.3086
#define GW 0.6094
#define BW 0.0820

static char defaultFile0[] = "../data/mandrill.rgb";
static char defaultFile1[] = "../data/sgi.bw";
static char defaultBrushFile[] = "../data/brush.rgb";
GLuint *img0, *img1, *brush;
GLsizei w0, w1, wbrush, h0, h1, hbrush;
GLsizei w, h;

GLint comp;

void init(void)
{
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

GLuint *load_img(const char *fname, GLsizei *imgW, GLsizei *imgH)
{
  GLuint *img;

  img = read_texture(fname, imgW, imgH, &comp);
  if (!img) {
    fprintf(stderr, "Could not open %s\n", fname);
    exit(1);
  }

  return img;
}

GLuint *
resize_img(GLuint *img, GLsizei curW, GLsizei curH)
{
  /* save & set buffer settings */
  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_PIXEL_MODE_BIT);
  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);

  glPixelZoom((float)w / (float)curW, (float)h / (float)curH);
  glRasterPos2i(0, 0);
  glDrawPixels(curW, curH, GL_RGBA, GL_UNSIGNED_BYTE, img);
  free(img);
  img = (GLuint *)malloc(w * h * sizeof(GLuint));
  if (!img) {
    fprintf(stderr, "Malloc of %d bytes failed.\n", 
	    curW * curH * sizeof(GLuint));
    exit(1);
  }
  glPixelZoom(1, 1);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img);

  glPopAttrib();

  return img;
}

GLuint *
convert_to_luminance(GLuint *img, GLsizei w, GLsizei h)
{
  GLubyte *newImg, *src, *dst;
  GLfloat val;
  int i;

  newImg = (GLubyte *)malloc(w * h);
  if (!newImg) {
    fprintf(stderr, "malloc of %d bytes failed\n", w*h);
    exit(1);
  }

  src = (GLubyte *)img;
  dst = newImg;
  for (i = 0; i < w*h; i++) {
    val = ((float)(*src++) * RW + 
	   (float)(*src++) * GW +
	   (float)(*src++) * BW);
    src++;
    if (val > 255) val = 255;
    *dst++ = val;
  }
  free(img);

  /* casting a ubyte ptr to a uint pointer is sloppy since it can
   * lead to alignment errors, but since the pointer came from
   * malloc we know it's legal in this case... */
  return (GLuint *)newImg;
}

void reshape(GLsizei winW, GLsizei winH) 
{
    glViewport(0, 0, w, h);
    glLoadIdentity();
    glOrtho(0, winW, 0, winH, 0, 5);
}

void draw(void)
{
  static int first = 1;
  GLenum err;

  if (first) {
    printf("Scaling images to %d by %d\n", w, h);

    if (w0 != w || h0 != h) {
      img0 = resize_img(img0, w0, h0);

    }
    if (w1 != w || h1 != h) {
      img1 = resize_img(img1, w1, h1);
    }

    first = 0;
  }
  
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2i(0, 0);
  glDrawBuffer(GL_BACK);
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img1);
  glDrawBuffer(GL_FRONT);
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img0);
  
  err = glGetError();
  if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));
}

int lastX, lastY, curX, curY;

int get_msecs(void)
{
  return glutGet(GLUT_ELAPSED_TIME) / 1000.0;
}

void idle(void)
{
  int x = curX - (wbrush/2);
  int y = h - (curY + (hbrush/2));
  int msecs;
  static int last_msecs = -1;

  /* do not do this more than 60 times a second.  Otherwise it's
   * to fast for use on high-end systems */
  msecs = get_msecs();
  if (fabs(last_msecs - msecs) < 1000./60.) {
    return;
  }
  last_msecs = msecs;

  /* we draw the brush using a drawpixels command.  on systems with
   * hardware-accelerated texture mapping it would be better to use
   * that. 
   * 
   * we use the bitmap hack to set the rasterpos because we don't
   * know that the position will be within the window. 
   */
  glRasterPos2i(0, 0);
  glBitmap(0, 0, 0, 0, x, y, 0);
  glColorMask(0, 0, 0, 1);
  glDrawBuffer(GL_BACK);
  glDrawPixels(wbrush, hbrush, GL_ALPHA, GL_UNSIGNED_BYTE, brush);
  glColorMask(1, 1, 1, 1);
  
  glReadBuffer(GL_BACK);
  glDrawBuffer(GL_FRONT);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glCopyPixels(x, y, wbrush, hbrush, GL_COLOR);
  glDisable(GL_BLEND);
  
  glColorMask(1, 1, 1, 1);
}

void motion(int xpos, int ypos)
{
  curX = xpos;
  curY = ypos;
}

/* ARGSUSED */
void button(int button, int state, int xpos, int ypos)
{
  if (state == GLUT_DOWN) {
    glutIdleFunc(idle); 
    lastX = lastY = -1;
    curX = xpos;
    curY = ypos;
    return;
  } else {
    glutIdleFunc(0);
  }
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
  if (key == 27) exit(0);
}

void 
show_usage(void)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "paint [imagefile0] [imagefile1] [brush]\n");
}

main(int argc, char *argv[])
{
  const char *fileName0 = defaultFile0, *fileName1 = defaultFile1,
  *brushName = defaultBrushFile;
  
  glutInit(&argc, argv);
  if (argc > 1) {
    fileName0 = argv[1];
  } 
  if (argc > 2) {
    fileName1 = argv[2];
  } 
  if (argc > 3) {
    brushName = argv[3];
  } 
  if (argc > 4) {
    show_usage();
    exit(1);
  }
  printf("Image file 1 is %s\n", fileName0);
  printf("Image file 2 is %s\n", fileName1);
  printf("Brush file is %s\n", brushName);

  img0 = load_img(fileName0, &w0, &h0);
  img1 = load_img(fileName1, &w1, &h1);
  brush = load_img(brushName, &wbrush, &hbrush);
  brush = convert_to_luminance(brush, wbrush, hbrush);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
  w = MAX(w0, w1);
  h = MAX(h0, h1);

  glutInitWindowSize(w, h);
  glutInitWindowPosition(0, 0);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);
  glutCreateWindow(argv[0]);
  glutDisplayFunc(draw);
  glutKeyboardFunc(key);
  glutReshapeFunc(reshape);
  glutMouseFunc(button);
  glutMotionFunc(motion);
  init();

  glutMainLoop();
  return 0;
}
