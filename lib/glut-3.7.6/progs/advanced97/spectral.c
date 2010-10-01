/* spectral.c - by Simon Hui, 3Dfx Interactive */

/* make a noise texture from multiple frequencies of noise */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture glBindTextureEXT
#endif

static int texxsize = 256, texysize = 256;
static int winxsize = 512, winysize = 512;

/* the highest and lowest octaves in the final noise */
static int minoctave = 1;
static int maxoctave = 6;

static GLenum ifmt = GL_LUMINANCE;

/* texture object names */
static GLuint basistex = 1;
static GLuint noisetex = 2;
static GLuint spectraltex = 6;
static GLuint abstex = 7;

int
logOf(int n) {
  int i=0;
  for (i=-1; n > 0; i++) {
    n >>= 1;
  }
  return i;
}

void
init_texture(void) {
  int i, j, n;
  int w, h;
  unsigned char *basis, *tex;
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  basis = (unsigned char *) malloc(texxsize * texysize);
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

  tex = (unsigned char *) malloc(4 * texxsize * texysize);
  for (n=0; n < 4; n++) {
    for (j=0; j < texysize; j++) {
      for (i=0; i < texxsize; i++) {
	int r = rand();
	
	/* mix it up a little more */
	r = ((r & 0xff) ^ ((r & 0xff00) >> 8)) & 0xff;

	/* For simplicity and because some opengl implementations offer    */
	/* more texture color depth for luminance textures than rgb ones,  */
	/* we use a luminance texture for the random noise.  However, you  */
	/* can make the texture rgb instead, and store different random    */
	/* values for r, g, and b; this is especially useful if using      */
	/* noise distortion below, because you'll get different distortion */
	/* values for s and t.                                             */
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
  glDisable(GL_DITHER);
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
  glTexCoord2f(0.f, 1.f); glVertex2f(xoff, yoff + 1.0); 
  glTexCoord2f(1.f, 0.f); glVertex2f(xoff + 1.0, yoff); 
  glTexCoord2f(1.f, 1.f); glVertex2f(xoff + 1.0, yoff + 1.0); 
  glEnd();
  glFlush();
}

static float wscale = 0.60;
static unsigned int **octbufs, *spectralbuf, *absbuf;

void
make_octaves(void) {
  int w, h, i;
  int octaves = maxoctave - minoctave + 1;
  float weight, sumweight;

  glBlendFunc(GL_ZERO, GL_SRC_COLOR);
  glEnable(GL_TEXTURE_2D);

  /* find the total weight */
  weight = 1.0;
  sumweight = 0;
  for (i=0; i < octaves; i++) {
    sumweight += weight;
    weight *= wscale;
  }
  octbufs = (unsigned int **) malloc(octaves * sizeof(unsigned int *));
  
  weight = 1.0;
  w = h = (1 << minoctave);
  for (i=0; i < octaves; i++) {
    octbufs[i] = (unsigned int *) malloc(sizeof(unsigned int) * 
					 winxsize * winysize);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    draw_basis(w, h, 0, 0);
    glEnable(GL_BLEND);
    draw_noise_texture(w, h, 0, 0, noisetex);
    glAccum(GL_LOAD, 1.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    draw_basis(w, h, 1, 0);
    glEnable(GL_BLEND);
    draw_noise_texture(w, h, 1, 0, noisetex + 1);
    glAccum(GL_ACCUM, 1.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    draw_basis(w, h, 0, 1);
    glEnable(GL_BLEND);
    draw_noise_texture(w, h, 0, 1, noisetex + 2);
    glAccum(GL_ACCUM, 1.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    draw_basis(w, h, 1, 1);
    glEnable(GL_BLEND);
    draw_noise_texture(w, h, 1, 1, noisetex + 3);
    glAccum(GL_ACCUM, 1.0);

    glDisable(GL_BLEND);
    glAccum(GL_RETURN, 1.0);
    glReadPixels(0, 0, winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
		 octbufs[i]);
    w <<= 1;
    h <<= 1;
  }
  glDisable(GL_TEXTURE_2D);
}

static GLboolean need_remake_octaves = GL_TRUE;
static GLboolean need_remake_spectral = GL_TRUE;
static GLboolean need_remake_abs_noise = GL_TRUE;

void
make_spectral_noise(void) {
  int i;
  int octaves = maxoctave - minoctave + 1;
  float weight, sumweight;

  if (need_remake_octaves) {
    make_octaves();
    need_remake_octaves = GL_FALSE;
  }
  /* find the total weight */
  weight = 1.0;
  sumweight = 0;
  for (i=0; i < octaves; i++) {
    sumweight += weight;
    weight *= wscale;
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
  weight = 1.0;
  for (i=0; i < octaves; i++) {
    glDrawPixels(winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
		 (GLvoid *) octbufs[i]);
    glAccum(GL_ACCUM, weight/sumweight);
    weight *= wscale;
  }

  /* save image in a texture */
  glAccum(GL_RETURN, 1.0);
  spectralbuf = (unsigned int *) malloc(4 * winxsize * winysize);
  glReadPixels(0, 0, winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
	       spectralbuf);
  glBindTexture(GL_TEXTURE_2D, spectraltex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, ifmt, winxsize, winysize, 0,
	       GL_RGBA, GL_UNSIGNED_BYTE, spectralbuf);
}

void
make_abs_noise(void) {
  unsigned int *negbuf = (unsigned int *) malloc(4 * winxsize * winysize);
  unsigned int *posbuf = (unsigned int *) malloc(4 * winxsize * winysize);

  if (need_remake_spectral) {
    make_spectral_noise();
    need_remake_spectral = GL_FALSE;
  }
  glDrawPixels(winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
	       (GLvoid *) spectralbuf);
  glAccum(GL_LOAD, 1.0);

  /* make it signed */
  glAccum(GL_ADD, -0.5);

  /* get the positive part of the noise */
  glAccum(GL_RETURN, 2.0);
  glReadPixels(0, 0, winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
	       (GLvoid *) negbuf);

  /* invert the negative part of the noise */
  glAccum(GL_RETURN, -2.0);
  glReadPixels(0, 0, winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
	       (GLvoid *) posbuf);

  /* add positive and inverted negative together, and you get abs() */
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glDrawPixels(winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
	       (GLvoid *) posbuf);
  glDrawPixels(winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE, 
	       (GLvoid *) negbuf);

  /* invert the colors so that peaks are bright instead of dark */
  glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
  glColor4f(1,1,1,1);
  glBegin(GL_TRIANGLE_STRIP);
  glVertex2f(0,0);
  glVertex2f(0,1);
  glVertex2f(1,0);
  glVertex2f(1,1);
  glEnd();
  glDisable(GL_BLEND);
  
  free(posbuf);
  free(negbuf);

  /* save image in a texture */
  absbuf = (unsigned int *) malloc(4 * winxsize * winysize);
  glReadPixels(0, 0, winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
	       (GLvoid *) absbuf);
  glBindTexture(GL_TEXTURE_2D, abstex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, ifmt, winxsize, winysize, 0,
	       GL_RGBA, GL_UNSIGNED_BYTE, absbuf);
}

static GLboolean show_abs = GL_FALSE;
static GLboolean show_distort = GL_FALSE;
static GLboolean mapcolors = GL_FALSE;
static float distfactor = 0.20;

void
display(void) {
  if (need_remake_spectral) {
    make_spectral_noise();
    need_remake_spectral = GL_FALSE;
  }
  if (show_abs) {
    if (need_remake_abs_noise) {
      make_abs_noise();
      need_remake_abs_noise = GL_FALSE;
    }
    glBindTexture(GL_TEXTURE_2D, abstex);
  } else {
    glBindTexture(GL_TEXTURE_2D, spectraltex);
  }

  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  if (show_distort) {
    /* Use values in the spectral texture to distort the texture being */
    /* viewed, by jittering the texture coordinates.                   */

    float x, y0, y1;
    float s, t0, t1;
    float ds, dt;
    unsigned int pix0, pix1;
    int vrows = 64, vcols = 64;
    int i, j;

    for (j=0; j < (vrows - 1); j++) {
      t0 = y0 = j / (vrows - 1.0);
      t1 = y1 = (j + 1) / (vrows - 1.0);

      glBegin(GL_TRIANGLE_STRIP);
      for (i=0; i < vcols; i++) {
	s = x = i / (vcols - 1.0);
	pix0 = spectralbuf[j * winxsize + i];
	pix1 = spectralbuf[(j+1) * winxsize + i];

	/* Use green component of noise to distort S coord, */
	/* and blue component to distort T coord.  Subtract */
	/* 127.5 to make it signed, and scale by distfactor.*/

	ds = ((pix0 & 0x00ff0000) >> 16);
	dt = ((pix0 & 0x0000ff00) >>  8);
	ds = (ds - 127.5) / 127.5 * distfactor;
	dt = (dt - 127.5) / 127.5 * distfactor;
	glTexCoord2f(s + ds, t0 + dt); glVertex2f(x, y0);
	ds = ((pix1 & 0x00ff0000) >> 16);
	dt = ((pix1 & 0x0000ff00) >>  8);
	ds = (ds - 127.5) / 127.5 * distfactor;
	dt = (dt - 127.5) / 127.5 * distfactor;
	glTexCoord2f(s + ds, t1 + dt); glVertex2f(x, y1);
      }
      glEnd();
    }
  } else {
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(0, 1); glVertex2f(0, 1);
    glTexCoord2f(1, 0); glVertex2f(1, 0);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glEnd();
  }
  glDisable(GL_TEXTURE_2D);

  if (mapcolors) {
    /* Map the gray values of the image into colors so that the texture   */
    /* looks like flames.  We do that by defining appropriate splines for */
    /* red, green, and blue.                                              */

    float r, g, b;
    float rt = 0.8;
    float gt = 0.3;
    float bt = 0.1;
    int i, j;
    unsigned char *c;
    unsigned int *mapbuf = (unsigned int *) malloc(4 * winxsize * winysize);
      
    glReadPixels(0, 0, winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
		 mapbuf);

    for (j=0; j < winysize; j++) {
      for (i=0; i < winxsize; i++) {
	c = (unsigned char *) &mapbuf[j * winxsize + i];
	r = c[0] / 255.0;
	g = c[1] / 255.0;
	b = c[2] / 255.0;

	if (r < (1-rt)) {
	  r = 0.0;
	} else {
	  float k = (r - (1.0 - rt)) / rt;
	  r = (3.0*k*k - 2.0*k*k*k) * 255.0;
	}
	if (g < (1-gt)) {
	  g = 0.0;
	} else {
	  float k = (g - (1.0 - gt)) / gt;
	  g = (3.0*k*k - 2.0*k*k*k) * 255.0;
	}
	if (b < (1-bt)) {
	  b = 0.0;
	} else {
	  float k = (b - (1.0 - bt)) / bt;
	  b = (3.0*k*k - 2.0*k*k*k) * 255.0;
	}
	c[0] = r;
	c[1] = g;
	c[2] = b;
      }
    }
    glDrawPixels(winxsize, winysize, GL_RGBA, GL_UNSIGNED_BYTE,
		 (GLvoid *) mapbuf);
    free(mapbuf);
  }
  glFlush();
}

void
reshape(int w, int h) {
  glViewport(0, 0, w, h);
  glutPostRedisplay();
}

enum {
  TOGGLE_ABS, TOGGLE_DISTORT, MORE_DISTORT, LESS_DISTORT, TOGGLE_MAP_COLORS,
  FIRE, QUIT
};

void
menu(int value) {
  switch (value) {
  case TOGGLE_ABS:
    show_abs = !show_abs;
    break;
  case TOGGLE_DISTORT:
    show_distort = !show_distort;
    break;
  case MORE_DISTORT:
    if (distfactor > 0.05) distfactor -= 0.05;
    break;
  case LESS_DISTORT:
    if (distfactor < 1.00) distfactor += 0.05;
    break;
  case TOGGLE_MAP_COLORS:
    mapcolors = !mapcolors;
    break;
  case FIRE:
    mapcolors = GL_TRUE;
    show_distort = GL_TRUE;
    show_abs = GL_TRUE;
    break;
  case QUIT:
    exit(0);
  }
  glutPostRedisplay();
}

int
main(int argc, char** argv) {
  glutInit(&argc, argv);
  glutInitWindowSize(winxsize, winysize);
  glutInitDisplayMode(GLUT_RGBA | GLUT_ACCUM);
  (void)glutCreateWindow("spectral noise function");
  init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutCreateMenu(menu);
  glutAddMenuEntry("Toggle Absolute Value", TOGGLE_ABS);
  glutAddMenuEntry("Toggle Noise Distortion", TOGGLE_DISTORT);
  glutAddMenuEntry("Decrease Distortion", MORE_DISTORT);
  glutAddMenuEntry("Increase Distortion", LESS_DISTORT);
  glutAddMenuEntry("Toggle Color Mapping", TOGGLE_MAP_COLORS);
  glutAddMenuEntry("Simulation of Fire", FIRE);
  glutAddMenuEntry("Quit", QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
