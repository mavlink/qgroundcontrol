/* noise.c - by Simon Hui, 3Dfx Interactive */

/* create an octave by filtering randomly generated noise */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture glBindTextureEXT
#endif


static GLint texxsize = 256, texysize = 256;
static GLint winxsize = 512, winysize = 512;
static GLint freq = 4;

/* texture object names */
static GLuint basistex = 1;
static GLuint noisetex = 2;

void
init_texture(void) {
  int i, j, n;
  int w, h;
  GLubyte *basis, *tex;
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  basis = (GLubyte *) malloc(texxsize * texysize);
  w = texxsize / 2;
  h = texysize / 2;
  for (j=0; j < h; j++) {
    for (i=0; i < w; i++) {
      GLint r;
      float u = i / (w - 1.0);
      float v = j / (h - 1.0);
      float f = 3 * u * u - 2 * u * u * u;
      float g = 3 * v * v - 2 * v * v * v;
      
      /* basis is a bicubic spline */
      r = f * g * 0xff;

      /* reflect around x and y axes */
      basis[j * texxsize + i] = r;
      basis[j * texxsize + texxsize-i-1] = r;
      basis[(texysize-j-1) * texxsize + i] = r;
      basis[(texysize-j-1) * texxsize + texxsize-i-1] = r;
    }
  }
  glBindTexture(GL_TEXTURE_2D, basistex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, texxsize, texysize, 0,
	       GL_RED, GL_UNSIGNED_BYTE, basis);
  free(basis);

  tex = (GLubyte *) malloc(4 * texxsize * texysize);
  for (n=0; n < 4; n++) {
    for (j=0; j < texysize; j++) {
      for (i=0; i < texxsize; i++) {
	int r = rand();
	
	/* mix it up a little more */
	r = ((r & 0xff) + ((r & 0xff00) >> 8)) & 0xff;

	tex[j*texxsize + i] = r;
      }
    }
    glBindTexture(GL_TEXTURE_2D, noisetex + n);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, texxsize, texysize, 0,
		 GL_RED, GL_UNSIGNED_BYTE, tex);
  }
  free(tex);
}

void
init(void) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glMatrixMode(GL_PROJECTION);
  glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glViewport(0, 0, winxsize, winysize);
  glBlendFunc(GL_DST_COLOR, GL_ZERO);

  glEnable(GL_TEXTURE_2D);
  init_texture();
}

void
draw_basis(int tsize, int ssize, int xadj, int yadj) {
  float tilessize = 1.0 / ssize;
  float tiletsize = 1.0 / tsize;
  float xoff = (xadj - 0.5) * 0.5 * tilessize;
  float yoff = (yadj - 0.5) * 0.5 * tiletsize;
  float xo, yo;
  int i, j;

  glBindTexture(GL_TEXTURE_2D, basistex);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  /* draw as many copies of the basis function as needed for this frequency */
  for (j=0; j < tsize; j++) {
    for (i=0; i < ssize; i++) {
      xo = xoff + i * tilessize;
      yo = yoff + j * tiletsize;
      glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(0.f, 0.f); glVertex2f(xo, yo); 
      glTexCoord2f(0.f, 1.f); glVertex2f(xo, yo + tiletsize); 
      glTexCoord2f(1.f, 0.f); glVertex2f(xo + tilessize, yo); 
      glTexCoord2f(1.f, 1.f); glVertex2f(xo + tilessize, yo + tiletsize); 
      glEnd();
    }
  }
  glFinish();
}

void
draw_noise_texture(int tsize, int ssize, int xadj, int yadj, int texname) {
  float tilessize = 1.0 / ssize;
  float tiletsize = 1.0 / tsize;
  float xoff = (xadj - 0.5) * 0.5 * tilessize;
  float yoff = (yadj - 0.5) * 0.5 * tiletsize;
  float scale = 1.0 / (texxsize / ssize);

  glBindTexture(GL_TEXTURE_2D, texname);

  /* scale the texture matrix to get a noise pattern of desired frequency */
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glScalef(scale,scale,scale);

  glBegin(GL_TRIANGLE_STRIP);
  glTexCoord2f(0.f, 0.f); glVertex2f(xoff, yoff); 
  glTexCoord2f(0.f, 1.f); glVertex2f(xoff, yoff+1.0); 
  glTexCoord2f(1.f, 0.f); glVertex2f(xoff + 1.0, yoff); 
  glTexCoord2f(1.f, 1.f); glVertex2f(xoff + 1.0, yoff + 1.0); 
  glEnd();
  glFlush();
}

/* menu choices */
enum {
  BASIS, NOISE, BASIS_TIMES_NOISE, OCTAVE, HIGHER_FREQ, LOWER_FREQ, QUIT=27
};

GLint showmode = BASIS_TIMES_NOISE;

void
display(void) {
  switch (showmode) {

  case BASIS:
    glClear(GL_COLOR_BUFFER_BIT);
    draw_basis(freq, freq, 0, 0);
    break;

  case NOISE:
    glClear(GL_COLOR_BUFFER_BIT);
    draw_noise_texture(freq, freq, 0, 0, noisetex);
    break;

  case BASIS_TIMES_NOISE:
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    draw_basis(freq, freq, 0, 0);
    glEnable(GL_BLEND);
    draw_noise_texture(freq, freq, 0, 0, noisetex);
    glDisable(GL_BLEND);
    break;

  case OCTAVE:

    /* put four sets together to get the final octave */

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    draw_basis(freq, freq, 0, 0);
    glEnable(GL_BLEND);
    draw_noise_texture(freq, freq, 0, 0, noisetex);
    glAccum(GL_LOAD, 1.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    draw_basis(freq, freq, 1, 0);
    glEnable(GL_BLEND);
    draw_noise_texture(freq, freq, 1, 0, noisetex + 1);
    glAccum(GL_ACCUM, 1.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    draw_basis(freq, freq, 0, 1);
    glEnable(GL_BLEND);
    draw_noise_texture(freq, freq, 0, 1, noisetex + 2);
    glAccum(GL_ACCUM, 1.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    draw_basis(freq, freq, 1, 1);
    glEnable(GL_BLEND);
    draw_noise_texture(freq, freq, 1, 1, noisetex + 3);
    glAccum(GL_ACCUM, 1.0);

    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT);
    glAccum(GL_RETURN, 1.0);
    break;
  }
  glFlush();
}

void
reshape(int w, int h) {
  glViewport(0, 0, w, h);
  glutPostRedisplay();
}

void
menu(int value) {
  switch (value) {
  case BASIS:
  case NOISE:
  case BASIS_TIMES_NOISE:
  case OCTAVE:
    showmode = value;
    break;
  case HIGHER_FREQ:
    if (freq < texxsize) freq *= 2;
    break;
  case LOWER_FREQ:
    freq /= 2;
    if (freq < 2) freq = 2;
    break;
  case QUIT:
    exit(0);
  }
  glutPostRedisplay();
}

int
main(int argc, char** argv) {
  glutInitWindowSize(winxsize, winysize);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_ACCUM);
  (void)glutCreateWindow("filtered noise function");
  init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutCreateMenu(menu);
  glutAddMenuEntry("Show One Basis", BASIS);
  glutAddMenuEntry("Show One Noise", NOISE);
  glutAddMenuEntry("Show One Basis x Noise", BASIS_TIMES_NOISE);
  glutAddMenuEntry("Show Octave", OCTAVE);
  glutAddMenuEntry("Higher Frequency", HIGHER_FREQ);
  glutAddMenuEntry("Lower Frequency", LOWER_FREQ);
  glutAddMenuEntry("Quit", QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
