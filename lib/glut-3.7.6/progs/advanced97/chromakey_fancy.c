#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

static char defaultFile0[] = "../data/swamp.rgb";
static char defaultFile1[] = "../data/swamp.rgb";
static char defaultFile2[] = "../data/mandrill.rgb";
GLuint *img0, *img1, *img2;
GLsizei w, h;
GLsizei w0, w1, w2, h0, h1, h2;
GLint comp;
GLfloat key[3] = {0, 0, 0};

#define RW 0.3086
#define GW 0.6094
#define BW 0.0820

/* key values less than or equal to lower fudge map to totally 
 * transparent... */
GLfloat lowerfudge = .2; 
GLfloat upperfudge = .8;

void init(void)
{
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

  return img;
}

void reshape(GLsizei winW, GLsizei winH) 
{
    glViewport(0, 0, 2*w, 2*h);
    glLoadIdentity();
    glOrtho(0, 2*w, 0, 2*h, 0, 5);
}

void compute_matte(void)
{
  glClear(GL_ACCUM_BUFFER_BIT);

  /* draw rectangle in (key color + 1) / 2 */
  glBegin(GL_QUADS);
  glColor3f(key[0], key[1], key[2]);
  glVertex2f(0, 0);
  glVertex2f(w, 0);
  glVertex2f(w, h);
  glVertex2f(0, h);
  glEnd();
  glFlush();

  /* negate & accumulate  */
  glAccum(GL_LOAD, -1);

  /* compute & return (image - key) */
  glRasterPos2f(0, 0);
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img0);
  glAccum(GL_ACCUM, 1);
  glAccum(GL_RETURN, 1);

  /* move to right hand side of window */
  glRasterPos2f(w, 0);
  glCopyPixels(0, 0, w, h, GL_COLOR);

  /* compute & return (key - image) */
  glEnable(GL_SCISSOR_TEST);
  glScissor(0, 0, w, h);
  glAccum(GL_MULT, -1);
  glAccum(GL_RETURN, 1);
  glScissor(0, 0, 2*w, h);
  glDisable(GL_SCISSOR_TEST);

  /* assemble to get fabs(key - image) */
  glBlendFunc(GL_ONE, GL_ONE);
  glEnable(GL_BLEND);
  glRasterPos2i(0, 0);
  glCopyPixels(w, 0, w, h, GL_COLOR);
  glDisable(GL_BLEND);

  /* assemble into alpha channel */
  {
    GLfloat mat[] = {
      RW, RW, RW, RW,
      GW, GW, GW, GW,
      BW, BW, BW, BW,
      0, 0, 0, 0,
    };
    glMatrixMode(GL_COLOR);
    glLoadMatrixf(mat);

    glRasterPos2i(w, 0);
    glCopyPixels(0, 0, w, h, GL_COLOR);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    /* do a second copy because sbias comes after color matrix in the
     * transfer pipeline.  could avoid this by using the post color matrix
     * scale bias... */
    if (upperfudge - lowerfudge) {
      glPixelTransferf(GL_ALPHA_SCALE, 1./(upperfudge - lowerfudge));
      glPixelTransferf(GL_ALPHA_BIAS, -lowerfudge/(upperfudge - lowerfudge)); 
    } else {
      /* move such that upper/lowerfudge maps to .5, then quantize with
       * 2-entry pixel map. */
      GLushort quantize[] = {0, 0xffff};
      glPixelTransferf(GL_ALPHA_BIAS, .5 - upperfudge);
      glPixelMapusv(GL_PIXEL_MAP_A_TO_A, 2, quantize);
      glPixelTransferi(GL_MAP_COLOR, 1);
    }
    glRasterPos2i(w, 0);
    glCopyPixels(w, 0, w, h, GL_COLOR);
    glPixelTransferf(GL_ALPHA_SCALE,  1);
    glPixelTransferf(GL_ALPHA_BIAS, 0);
    glPixelTransferi(GL_MAP_COLOR, 0);
  }


  /* copy matte to right */
  glRasterPos2i(0, 0);
  glCopyPixels(w, 0, w, h, GL_COLOR);

  /* draw the third image */
  glColorMask(1, 1, 1, 0);
  glRasterPos2i(w, 0);
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img2);
  glColorMask(1, 1, 1, 1);

  glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
  glEnable(GL_BLEND);
  glRasterPos2i(w, 0);
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img1);

  /* this is for matte display... */
  glColor3f(1, 1, 1);
  glBegin(GL_QUADS);
  glVertex2f(0, 0);
  glVertex2f(w, 0);
  glVertex2f(w, h);
  glVertex2f(0, h);
  glEnd();

  glDisable(GL_BLEND);
}

void draw(void)
{
  GLenum err;
  static int first = 1;
  
  if (first) {
    printf("Scaling images to %d by %d\n", w, h);


    if (w0 != w || h0 != h) {
      img0 = resize_img(img0, w0, h0);

    }
    if (w1 != w || h1 != h) {
      img1 = resize_img(img1, w1, h1);
    }
    if (w2 != w || h2 != h) {
      img2 = resize_img(img2, w2, h2);
    }
    first = 0;
  }
  
  
  glClear(GL_COLOR_BUFFER_BIT);
  compute_matte();
  
  glRasterPos2i(w/2, h);
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img0);

  err = glGetError();
  if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));
}

/* ARGSUSED */
void button(int button, int state, int xpos, int ypos)
{
  if (state != GLUT_UP) return;

  ypos = 2*h - ypos;
  glReadPixels(xpos, ypos, 1, 1, GL_RGB, GL_FLOAT, key);
  printf("Key is (%f %f %f)\n", key[0], key[1], key[2]);
  draw();
}

/* ARGSUSED1 */
void keyPress(unsigned char whichKey, int x, int y)
{
  if (whichKey == 27) exit(0);
}

void change_lower_fudge(int val)
{
  lowerfudge = (float)val / 100.;
  if (upperfudge < lowerfudge) upperfudge = lowerfudge;
  draw();
}

void change_upper_fudge(int val)
{
  upperfudge = (float)val / 100.;
  if (lowerfudge > upperfudge) lowerfudge = upperfudge;
  draw();
}

void show_usage(void) 
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "chromakey mattefile file0 file1 [matteR matteG matteB]\n");
  fprintf(stderr, "chromakey mattefileAndfile0 file1 [matteR matteG matteB]\n");
}

main(int argc, char *argv[])
{
  char *fileName0 = defaultFile0, *fileName1 = defaultFile1, 
  *fileName2 = defaultFile2;
  
  glutInit(&argc, argv);
  if (argc > 1) {
    fileName0 = fileName1 = argv[1];
  }
  if (argc > 2) {
    fileName2 = argv[2];
  }
  if (argc > 3) {
    fileName1 = fileName2;
    fileName2 = argv[3];
  }
  if (argc > 4) {
    if (argc == 6 || argc == 7) {
      key[0] = atof(argv[argc-3]);
      key[1] = atof(argv[argc-2]);
      key[2] = atof(argv[argc-1]);
    } else {
      show_usage();
      exit(1);
    }
  }
  
  printf("Matte file is %s\n", fileName0);
  printf("Image file 1 is %s\n", fileName1);
  printf("Image file 2 is %s\n", fileName2);
  printf("Key is (%f %f %f)\n", key[0], key[1], key[2]);
  printf("Transparent boundary is %f\n", lowerfudge);
  printf("Opaque boundary is %f\n", upperfudge);
  img0 = load_img(fileName0, &w0, &h0);
  img1 = load_img(fileName1, &w1, &h1);
  img2 = load_img(fileName2, &w2, &h2);
  
#define MAX(a, b) ((a) > (b) ? (a) : (b))
  w = MAX(MAX(w0, w1), w2);
  h = MAX(MAX(h0, h1), h2);
  
  glutInitWindowSize(2*w, 2*h);
  glutInitWindowPosition(0, 0);
  glutInitDisplayMode(GLUT_RGBA | GLUT_ACCUM | GLUT_ALPHA);
  glutCreateWindow(argv[0]);
  glutDisplayFunc(draw);
  glutKeyboardFunc(keyPress);
  glutReshapeFunc(reshape);
  glutMouseFunc(button);

  {
    int lowerFudgeMenu, upperFudgeMenu;
    lowerFudgeMenu = glutCreateMenu(change_lower_fudge);
    glutAddMenuEntry("0", 0);
    glutAddMenuEntry(".1", 20);
    glutAddMenuEntry(".25", 20);
    glutAddMenuEntry(".5", 50);
    glutAddMenuEntry(".75", 75);
    upperFudgeMenu = glutCreateMenu(change_upper_fudge);
    glutAddMenuEntry(".25", 20);
    glutAddMenuEntry(".5", 50);
    glutAddMenuEntry(".75", 75);    
    glutAddMenuEntry(".9", 90);
    glutAddMenuEntry("1", 100);
    glutCreateMenu(0);
    glutAddSubMenu("Transparent boundary", lowerFudgeMenu);
    glutAddSubMenu("Opaque boundary", upperFudgeMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
  }

  init();
  
  reshape(w, h);
  glutMainLoop();
  return 0;
}


