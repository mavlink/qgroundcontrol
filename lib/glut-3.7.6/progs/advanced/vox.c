/**
 *
 * vox.c : GLUT example for volume rendering
 *
 * Author : Yusuf Attarwala
 *          SGI - Applications
 *
 * Mods by Mark Kilgard.
 *
 * cc vox.c -o vox -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm
 *
 * Voxel Head is a simple volume rendering example using OpenGL 3d textures.
 * This version has limited features (intentional), to keep the code simple.
 *
 * i.e. - there is no control for changing alpha values.
 *      - it just deals with one block of texture (no tiling)
 *      - there are no clipping planes
 *      - it only works with one data set, so it assumes that texture data
 * 	  is in powers of 2.
 *
 * Some of these features will be implemented in future releases.
 *
 * The technique to create polygonal slices thru voxel space is as
 * follows:
 *
 * Instead of recomputing polygonal slices perpendicular to every
 * viewing vector, it uses the same set of polygonal slices for a
 * range of -45 to 45 degrees. Outside this range, it recomputes
 * another set of slices and so on.
 *
 * These slices are then rendered back to front with 3d texture and
 * blending enabled.
 *
 * It uses GLUT for handling events, windows etc.
 *
 * This program runs with good performance on RealityEngine, VTX,
 * InfiniteReality, or Maximum IMPACT.  In IRIX 6.2, a software
 * implementation of OpenGL's 3D texture mapping extension is
 * available, but performance is very poor.  If you are on a slow
 * machine, you can run "vox -sb" and still get a good feel for how
 * the program performs volume rendering since you can see the 3D
 * slices be rendered back to front with blending.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#else
#define R_OK 04  /* Win32 doesn't define this */
/* Win32 defines these, but with a leading _ */
#define pclose _pclose
#define popen _popen
/* Win32 doesn't have a re-entrand rand() */
#define rand_r(x) rand()
#endif
#include <string.h>
#include <math.h>

#include <GL/glut.h>

#define ABS(a)   (((a) >= 0) ? (a) : -(a))
#define _XY    1
#define _YZ    2
#define _ZX    3
#define _MXY   4
#define _MYZ   5
#define _MZX   6

/* pop up menu entries */
#define SPIN_ON   1
#define SPIN_OFF  2
#define MENU_HELP 3
#define MENU_EXIT 4

/* global variables */

int width, height;      /* window width, height */
float left, right, bottom, top, nnear, ffar;  /* ortho, view volume */
float vol_width, vol_height, vol_depth;  /* volume dimensions */
float bminx, bmaxx, bminy, bmaxy, bminz, bmaxz, bdiag;  /* bounding box */
int n_slices;           /* number of slices */
int tex3dSupported = 1;
float slice_poly[3][4][3];
float slice_tcoord[3][4][3];
float anglex, angley, anglez;

unsigned char *voxels;

void
readVoxelData(void)
{
  FILE *file;
  int i, j, k, using_pipe;
  unsigned char *vptr;
  unsigned char *vp, *vx;

  /* see if the hardware supports 3d texture */
#ifdef GL_EXT_texture3D
  if (!glutExtensionSupported("GL_EXT_texture3D")) {
    printf("\n==================================================================\n");
    printf("This hardware (%s) does not support 3d texture extentions\n",
      (char *) (glGetString(GL_RENDERER)));
    printf("==================================================================\n");

    tex3dSupported = 0;
  }
#else
  printf("\n==================================================================\n");
  printf("Not API support for GL_EXT_texture3D extension when compiled.\n");
  printf("==================================================================\n");
#endif
  /* open vox.bin data file */
  if ((file = fopen("vox.bin", "r")) == NULL) {
#ifndef _WIN32
    if (!access("vox.bin.gz", R_OK)) {
      if ((file = popen("gzcat vox.bin.gz", "r")) == NULL) {
        fprintf(stderr, "cannot popen input file vox.bin.gz (missing gzcat?)\n");
        exit(1);
      }
    } else if (!access("vox.bin.Z", R_OK)) {
      if ((file = popen("zcat vox.bin.Z", "r")) == NULL) {
        fprintf(stderr, "cannot popen input file vox.bin.Z (missing zcat?)\n");
        exit(1);
      }
    } else {
      fprintf(stderr, "cannot find vox.bin, vox.bin.gz, or vox.bin.Z\n");
      exit(1);
    }
    using_pipe = 1;
#else
    fprintf(stderr, "cannot find vox.bin\n");
    exit(1);
#endif
  } else {
    using_pipe = 0;
  }
  vol_width = 128;      /* hard coded for demo */
  vol_height = 128;
  vol_depth = 64;
  n_slices = 128;

  if (tex3dSupported) {
    unsigned long size = (unsigned long) (vol_width * vol_height * vol_depth);

    vptr = (unsigned char *) malloc(size);

    fread(vptr, sizeof(char), size, file);
    if (using_pipe) {
      pclose(file);
    } else {
      fclose(file);
    }

    /* size of voxels is twice as the size of vptr, to duplicate alpha value
       = intensity */
    voxels = (unsigned char *) malloc(2 * size);

    /* for now duplicate, alpha value = intensity */
    vx = voxels;
    vp = vptr;
    for (i = 0; i < vol_width; i++)
      for (j = 0; j < vol_height; j++)
        for (k = 0; k < vol_depth; k++) {
          *vx++ = *vp;
          *vx++ = *vp++;
        }

    free(vptr);
  }
  /* compute bounding box extents */

  bminx = -(float) vol_width / 2.0;
  bmaxx = (float) vol_width / 2.0;
  bminy = -(float) vol_height / 2.0;
  bmaxy = (float) vol_height / 2.0;
  bminz = -(float) vol_depth / 2.0;
  bmaxz = (float) vol_depth / 2.0;

  bdiag = sqrt((bmaxx) * (bmaxx) + (bmaxy) * (bmaxy) + (bmaxz) * (bmaxz));

  /* compute view volume extents */

  left = -1.1 * bdiag;
  right = 1.1 * bdiag;
  bottom = -1.1 * bdiag;
  top = 1.1 * bdiag;
  nnear = -1.0 * bdiag;
  ffar = 2 * 1.1 * bdiag;

  /* define the polygon dimensions on which the texture will be mapped */
  /* xy plane */
  slice_poly[0][0][0] = -vol_width / 2.0;
  slice_poly[0][0][1] = -vol_height / 2.0;
  slice_poly[0][0][2] = 0.0;

  slice_poly[0][1][0] = vol_width / 2.0;
  slice_poly[0][1][1] = -vol_height / 2.0;
  slice_poly[0][1][2] = 0.0;

  slice_poly[0][2][0] = vol_width / 2.0;
  slice_poly[0][2][1] = vol_height / 2.0;
  slice_poly[0][2][2] = 0.0;

  slice_poly[0][3][0] = -vol_width / 2.0;
  slice_poly[0][3][1] = vol_height / 2.0;
  slice_poly[0][3][2] = 0.0;

  /* yz plane */
  slice_poly[1][0][0] = 0.0;
  slice_poly[1][0][1] = -vol_height / 2.0;
  slice_poly[1][0][2] = -vol_depth / 2.0;

  slice_poly[1][1][0] = 0.0;
  slice_poly[1][1][1] = vol_height / 2.0;
  slice_poly[1][1][2] = -vol_depth / 2.0;

  slice_poly[1][2][0] = 0.0;
  slice_poly[1][2][1] = vol_height / 2.0;
  slice_poly[1][2][2] = vol_depth / 2.0;

  slice_poly[1][3][0] = 0.0;
  slice_poly[1][3][1] = -vol_height / 2.0;
  slice_poly[1][3][2] = vol_depth / 2.0;

  /* zx plane */
  slice_poly[2][0][0] = -vol_width / 2.0;
  slice_poly[2][0][1] = 0.0;
  slice_poly[2][0][2] = -vol_depth / 2.0;

  slice_poly[2][1][0] = -vol_width / 2.0;
  slice_poly[2][1][1] = 0.0;
  slice_poly[2][1][2] = vol_depth / 2.0;

  slice_poly[2][2][0] = vol_width / 2.0;
  slice_poly[2][2][1] = 0.0;
  slice_poly[2][2][2] = vol_depth / 2.0;

  slice_poly[2][3][0] = vol_width / 2.0;
  slice_poly[2][3][1] = 0.0;
  slice_poly[2][3][2] = -vol_depth / 2.0;

  /* texture coordinates */

  slice_tcoord[0][0][0] = 0.0;
  slice_tcoord[0][0][1] = 0.0;
  slice_tcoord[0][1][0] = 1.0;
  slice_tcoord[0][1][1] = 0.0;
  slice_tcoord[0][2][0] = 1.0;
  slice_tcoord[0][2][1] = 1.0;
  slice_tcoord[0][3][0] = 0.0;
  slice_tcoord[0][3][1] = 1.0;

  slice_tcoord[1][0][1] = 0.0;
  slice_tcoord[1][0][2] = 1.0;
  slice_tcoord[1][1][1] = 1.0;
  slice_tcoord[1][1][2] = 1.0;
  slice_tcoord[1][2][1] = 1.0;
  slice_tcoord[1][2][2] = 0.0;
  slice_tcoord[1][3][1] = 0.0;
  slice_tcoord[1][3][2] = 0.0;

  slice_tcoord[2][0][2] = 0.0;
  slice_tcoord[2][0][0] = 0.0;
  slice_tcoord[2][1][2] = 1.0;
  slice_tcoord[2][1][0] = 0.0;
  slice_tcoord[2][2][2] = 1.0;
  slice_tcoord[2][2][0] = 1.0;
  slice_tcoord[2][3][2] = 0.0;
  slice_tcoord[2][3][0] = 1.0;

}

void
randomTick(void)
{
  static unsigned int seed = 0;
  static int changeSeed = 25;
  float fltran;

  if (changeSeed++ >= 25) {
    seed++;
    if (seed > 256)
      seed = 0;
    changeSeed = 0;
  }
  fltran = (float) (rand_r(&seed) / 30000.0);

  anglex = (anglex > 360.0) ? 0.0 : (anglex + fltran);
  angley = (angley > 360.0) ? 0.0 : (angley + fltran);
  anglez = (anglez > 360.0) ? 0.0 : (anglez + fltran);
}

void
animate(void)
{
  randomTick();
  glutPostRedisplay();
}

void
printCheatSheet(void)
{
  printf("\n\n-------------------------\n");
  printf("OpenGL 3d texture example\n\n");

  printf("Keyboard shortcuts\n");
  printf("s key   : zoom out (small)\n");
  printf("l key   : zoom in  (large)\n");
  printf("x key   : rotate about screen x\n");
  printf("y key   : rotate about screen y\n");
  printf("z key   : rotate about screen z\n");
  printf("esc key : quit\n");
}

void
menu(int choice)
{
  /* simple GLUT popup menu stuff */
  switch (choice) {
  case SPIN_ON:
    glutChangeToMenuEntry(1, "Random Spin OFF", SPIN_OFF);
    glutIdleFunc(animate);
    break;
  case SPIN_OFF:
    glutChangeToMenuEntry(1, "Random Spin ON", SPIN_ON);
    glutIdleFunc(NULL);
    break;
  case MENU_HELP:
    printCheatSheet();
    break;
  case MENU_EXIT:
    exit(0);
    break;
  }
}

void
setMatrix(void)
{
  /* feel like using ortho projection */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(left, right, bottom, top, nnear, ffar);

  /* boring view matrix */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void
load3DTexture(void)
{
  printf("setting up 3d textures...\n");

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

#ifdef GL_EXT_texture3D
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_R_EXT, GL_REPEAT);

  glTexImage3DEXT(GL_TEXTURE_3D_EXT, 0, GL_LUMINANCE8_ALPHA8_EXT,
    vol_width, vol_height, vol_depth,
    0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, voxels);
#endif

  if (tex3dSupported) {
    /* enable texturing, blending etc */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
  }
  setMatrix();
}

void
init(void)
{
  /* angle of rotation about coordinate axes */
  anglex = angley = anglez = 0.0;
}

void
invert4d(float from[4][4], float to[4][4])
{

  /* 4x4 matrix inversion routine */

  float wtemp[4][8];
  register float m0, m1, m2, m3, s;
  register float *r0, *r1, *r2, *r3, *rtemp;

  r0 = wtemp[0];
  r1 = wtemp[1];
  r2 = wtemp[2];
  r3 = wtemp[3];
  r0[0] = from[0][0];   /* build up [A][I] */
  r0[1] = from[0][1];
  r0[2] = from[0][2];
  r0[3] = from[0][3];
  r0[4] = 1.0;
  r0[5] = 0.0;
  r0[6] = 0.0;
  r0[7] = 0.0;
  r1[0] = from[1][0];
  r1[1] = from[1][1];
  r1[2] = from[1][2];
  r1[3] = from[1][3];
  r1[4] = 0.0;
  r1[5] = 1.0;
  r1[6] = 0.0;
  r1[7] = 0.0;
  r2[0] = from[2][0];
  r2[1] = from[2][1];
  r2[2] = from[2][2];
  r2[3] = from[2][3];
  r2[4] = 0.0;
  r2[5] = 0.0;
  r2[6] = 1.0;
  r2[7] = 0.0;
  r3[0] = from[3][0];
  r3[1] = from[3][1];
  r3[2] = from[3][2];
  r3[3] = from[3][3];
  r3[4] = 0.0;
  r3[5] = 0.0;
  r3[6] = 0.0;
  r3[7] = 1.0;
  if (r0[0] == 0.0) {   /* swap rows if needed */
    if (r1[0] == 0.0) {
      if (r2[0] == 0.0) {
        if (r3[0] == 0.0)
          goto singular;
        rtemp = r0;
        r0 = r3;
        r3 = rtemp;
      } else {
        rtemp = r0;
        r0 = r2;
        r2 = rtemp;
      }
    } else {
      rtemp = r0;
      r0 = r1;
      r1 = rtemp;
    }
  }
  m1 = r1[0] / r0[0];   /* eliminate first variable */
  m2 = r2[0] / r0[0];
  m3 = r3[0] / r0[0];
  s = r0[1];
  r1[1] = r1[1] - m1 * s;
  r2[1] = r2[1] - m2 * s;
  r3[1] = r3[1] - m3 * s;
  s = r0[2];
  r1[2] = r1[2] - m1 * s;
  r2[2] = r2[2] - m2 * s;
  r3[2] = r3[2] - m3 * s;
  s = r0[3];
  r1[3] = r1[3] - m1 * s;
  r2[3] = r2[3] - m2 * s;
  r3[3] = r3[3] - m3 * s;
  s = r0[4];
  if (s != 0.0) {
    r1[4] = r1[4] - m1 * s;
    r2[4] = r2[4] - m2 * s;
    r3[4] = r3[4] - m3 * s;
  }
  s = r0[5];
  if (s != 0.0) {
    r1[5] = r1[5] - m1 * s;
    r2[5] = r2[5] - m2 * s;
    r3[5] = r3[5] - m3 * s;
  }
  s = r0[6];
  if (s != 0.0) {
    r1[6] = r1[6] - m1 * s;
    r2[6] = r2[6] - m2 * s;
    r3[6] = r3[6] - m3 * s;
  }
  s = r0[7];
  if (s != 0.0) {
    r1[7] = r1[7] - m1 * s;
    r2[7] = r2[7] - m2 * s;
    r3[7] = r3[7] - m3 * s;
  }
  if (r1[1] == 0.0) {   /* swap rows if needed */
    if (r2[1] == 0.0) {
      if (r3[1] == 0.0)
        goto singular;
      rtemp = r1;
      r1 = r3;
      r3 = rtemp;
    } else {
      rtemp = r1;
      r1 = r2;
      r2 = rtemp;
    }
  }
  m2 = r2[1] / r1[1];   /* eliminate second variable */
  m3 = r3[1] / r1[1];
  r2[2] = r2[2] - m2 * r1[2];
  r3[2] = r3[2] - m3 * r1[2];
  r3[3] = r3[3] - m3 * r1[3];
  r2[3] = r2[3] - m2 * r1[3];
  s = r1[4];
  if (s != 0.0) {
    r2[4] = r2[4] - m2 * s;
    r3[4] = r3[4] - m3 * s;
  }
  s = r1[5];
  if (s != 0.0) {
    r2[5] = r2[5] - m2 * s;
    r3[5] = r3[5] - m3 * s;
  }
  s = r1[6];
  if (s != 0.0) {
    r2[6] = r2[6] - m2 * s;
    r3[6] = r3[6] - m3 * s;
  }
  s = r1[7];
  if (s != 0.0) {
    r2[7] = r2[7] - m2 * s;
    r3[7] = r3[7] - m3 * s;
  }
  if (r2[2] == 0.0) {   /* swap last 2 rows if needed */
    if (r3[2] == 0.0)
      goto singular;
    rtemp = r2;
    r2 = r3;
    r3 = rtemp;
  }
  m3 = r3[2] / r2[2];   /* eliminate third variable */
  r3[3] = r3[3] - m3 * r2[3];
  r3[4] = r3[4] - m3 * r2[4];
  r3[5] = r3[5] - m3 * r2[5];
  r3[6] = r3[6] - m3 * r2[6];
  r3[7] = r3[7] - m3 * r2[7];
  if (r3[3] == 0.0)
    goto singular;
  s = 1.0 / r3[3];      /* now back substitute row 3 */
  r3[4] = r3[4] * s;
  r3[5] = r3[5] * s;
  r3[6] = r3[6] * s;
  r3[7] = r3[7] * s;
  m2 = r2[3];           /* now back substitute row 2 */
  s = 1.0 / r2[2];
  r2[4] = s * (r2[4] - r3[4] * m2);
  r2[5] = s * (r2[5] - r3[5] * m2);
  r2[6] = s * (r2[6] - r3[6] * m2);
  r2[7] = s * (r2[7] - r3[7] * m2);
  m1 = r1[3];
  r1[4] = (r1[4] - r3[4] * m1);
  r1[5] = (r1[5] - r3[5] * m1);
  r1[6] = (r1[6] - r3[6] * m1);
  r1[7] = (r1[7] - r3[7] * m1);
  m0 = r0[3];
  r0[4] = (r0[4] - r3[4] * m0);
  r0[5] = (r0[5] - r3[5] * m0);
  r0[6] = (r0[6] - r3[6] * m0);
  r0[7] = (r0[7] - r3[7] * m0);
  m1 = r1[2];           /* now back substitute row 1 */
  s = 1.0 / r1[1];
  r1[4] = s * (r1[4] - r2[4] * m1);
  r1[5] = s * (r1[5] - r2[5] * m1);
  r1[6] = s * (r1[6] - r2[6] * m1);
  r1[7] = s * (r1[7] - r2[7] * m1);
  m0 = r0[2];
  r0[4] = (r0[4] - r2[4] * m0);
  r0[5] = (r0[5] - r2[5] * m0);
  r0[6] = (r0[6] - r2[6] * m0);
  r0[7] = (r0[7] - r2[7] * m0);
  m0 = r0[1];           /* now back substitute row 0 */
  s = 1.0 / r0[0];
  r0[4] = s * (r0[4] - r1[4] * m0);
  r0[5] = s * (r0[5] - r1[5] * m0);
  r0[6] = s * (r0[6] - r1[6] * m0);
  r0[7] = s * (r0[7] - r1[7] * m0);
  to[0][0] = r0[4];     /* copy results back */
  to[0][1] = r0[5];
  to[0][2] = r0[6];
  to[0][3] = r0[7];
  to[1][0] = r1[4];
  to[1][1] = r1[5];
  to[1][2] = r1[6];
  to[1][3] = r1[7];
  to[2][0] = r2[4];
  to[2][1] = r2[5];
  to[2][2] = r2[6];
  to[2][3] = r2[7];
  to[3][0] = r3[4];
  to[3][1] = r3[5];
  to[3][2] = r3[6];
  to[3][3] = r3[7];
  return;
singular:
  printf("ERROR : non_invertable transform\n");
  return;
}

void
normalize(float *xn, float *yn, float *zn)
{
  double denom;

  denom = sqrt((double) ((*xn * *xn) + (*yn * *yn) + (*zn * *zn)));

  *xn = *xn / denom;
  *yn = *yn / denom;
  *zn = *zn / denom;
}

int
getViewAxis(void)
{
  float viewDir[3];
  float mat[4][4], vinv[4][4];
  float maxf, xy, yz, zx;
  int im;

  /* out of 3 orthogonal set of planes in world coords,  find out which one
     has maximum angle from the line of  sight.

     we will use these set of planes for creating polygonal slices thru the
     voxel space */

  glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) mat);
  invert4d(mat, vinv);

  viewDir[0] = -vinv[2][0];
  viewDir[1] = -vinv[2][1];
  viewDir[2] = -vinv[2][2];

  normalize(&viewDir[0], &viewDir[1], &viewDir[2]);

  xy = viewDir[2];      /* simplified because 0*xx + 0*yy + 1*zz */
  yz = viewDir[0];
  zx = viewDir[1];

  maxf = ABS(xy);
  im = (xy < 0.0) ? _XY : _MXY;

  if (maxf <= ABS(yz)) {
    maxf = ABS(yz);
    im = (yz < 0.0) ? _YZ : _MYZ;
  }
  if (maxf <= ABS(zx)) {
    maxf = ABS(zx);
    im = (zx < 0.0) ? _ZX : _MZX;
  }
  return (im);
}

#define DRAW_SLICE \
            if (tex3dSupported) {\
              glBegin(GL_POLYGON);\
	      for (p=0;p<4;p++) {\
		glTexCoord3fv(slice_tcoord[myaxis][p]);\
		glVertex3fv(slice_poly[myaxis][p]);\
	      }\
	      glEnd();\
	    } \
            else {\
              glBegin(GL_LINE_LOOP);\
	      for (p=0;p<4;p++){\
		glVertex3fv(slice_poly[myaxis][p]);\
	      }\
	      glEnd();\
	    }

void
drawScene(void)
{

  int i, p;
  float tc;
  int viewAxis, myaxis;
  int sign;

  static int myaxis_lut[] =
  {0, 0, 1, 2, 0, 1, 2};

  /* clear background, z buffer etc */
  glClear(GL_COLOR_BUFFER_BIT);

  glPushMatrix();

  /* apply all the modeling transformations */
  glTranslatef(0.0, 0.0, -bdiag);
  glRotatef(anglex, 1.0, 0.0, 0.0);
  glRotatef(angley, 0.0, 1.0, 0.0);
  glRotatef(anglez, 0.0, 0.0, 1.0);

  /* getViewAxis(), determines which set of polygons need to be used for
     texturing, depending upon the viewing direction */

  viewAxis = getViewAxis();
  myaxis = myaxis_lut[viewAxis];

  glColor3f(1.0, 1.0, 1.0);

#ifdef GL_EXT_texture3D
  if (tex3dSupported)
    glEnable(GL_TEXTURE_3D_EXT);
#endif

  switch (viewAxis) {
  case _XY:
  case _MXY:

    sign = (viewAxis == _XY) ? 1 : -1;
    tc = (viewAxis == _XY) ? 0.0 : 1.0;

    glTranslatef(0.0, 0.0, -sign * vol_depth / 2.0);
    for (i = 0; i < n_slices; i++) {

      slice_tcoord[0][0][2] = slice_tcoord[0][1][2] =
        slice_tcoord[0][2][2] = slice_tcoord[0][3][2] = tc;

      tc += sign * 1.0 / n_slices;
      glTranslatef(0.0, 0.0, sign * vol_depth / (n_slices + 1.0));

      DRAW_SLICE;
    }
    break;
  case _YZ:
  case _MYZ:

    sign = (viewAxis == _YZ) ? 1 : -1;
    tc = (viewAxis == _YZ) ? 0.0 : 1.0;

    glTranslatef(-sign * vol_width / 2.0, 0.0, 0.0);
    for (i = 0; i < n_slices; i++) {

      slice_tcoord[1][0][0] = slice_tcoord[1][1][0] =
        slice_tcoord[1][2][0] = slice_tcoord[1][3][0] = tc;

      tc += sign * 1.0 / n_slices;
      glTranslatef(sign * vol_width / (n_slices + 1.0), 0.0, 0.0);

      DRAW_SLICE;
    }
    break;
  case _ZX:
  case _MZX:

    sign = (viewAxis == _ZX) ? 1 : -1;
    tc = (viewAxis == _ZX) ? 0.0 : 1.0;

    glTranslatef(0.0, -sign * vol_height / 2.0, 0.0);
    for (i = 0; i < n_slices; i++) {

      slice_tcoord[2][0][1] = slice_tcoord[2][1][1] =
        slice_tcoord[2][2][1] = slice_tcoord[2][3][1] = tc;

      tc += sign * 1.0 / n_slices;
      glTranslatef(0.0, sign * vol_height / (n_slices + 1.0), 0.0);

      DRAW_SLICE;

    }
    break;
  }

#ifdef GL_EXT_texture3D
  if (tex3dSupported)
    glDisable(GL_TEXTURE_3D_EXT);
#endif

  glPopMatrix();

  glutSwapBuffers();
}

void
resize(int w, int h)
{
  /* things you do, when the user resizes the window */

  width = w;
  height = h;

  glViewport(0, 0, w, h);
  setMatrix();
}

/* ARGSUSED1 */
void
keyboard(unsigned char c, int x, int y)
{
  /* handle key board input */

  switch (c) {
  case 27:
    exit(0);
    break;
  case 'x':
    anglex += 1.0;
    drawScene();
    break;
  case 'y':
    angley += 1.0;
    drawScene();
    break;
  case 'z':
    anglez += 1.0;
    drawScene();
    break;
  case 'm':
    n_slices++;
    drawScene();
    break;
  case 'e':
    n_slices--;
    if (n_slices < 4)
      n_slices = 4;
    drawScene();
    break;
  case 's':
    if (left < right + 10.0) {
      left -= 1.0;
      right += 1.0;
      bottom -= 1.0;
      top += 1.0;
      setMatrix();
      drawScene();
    }
    break;
  case 'l':
    left += 1.0;
    right -= 1.0;
    bottom += 1.0;
    top -= 1.0;
    setMatrix();
    drawScene();
    break;
  default:
    break;
  }
}

void
main(int argc, char **argv)
{
  int i, mode = GLUT_DOUBLE;

  glutInit(&argc, argv);

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-no3Dtex")) {
      tex3dSupported = 0;
    } else if (!strcmp(argv[i], "-sb")) {
      mode = GLUT_SINGLE;
    }
  }

  /* let glut do all the X Stuff */
  glutInitDisplayMode(GLUT_RGB | mode);
  glutCreateWindow("Voxel Head");

  /* init our variables, etc */
  init();

  /* read texture data from a file */
  readVoxelData();

  /* set up OpenGL texturing */
  if (tex3dSupported)
    load3DTexture();

  /* register specific routines to glut */
  glutDisplayFunc(drawScene);
  glutReshapeFunc(resize);
  glutKeyboardFunc(keyboard);

  /* create popup menu for glut */
  glutCreateMenu(menu);

  glutAddMenuEntry("Random Spin ON", SPIN_ON);
  glutAddMenuEntry("Help", MENU_HELP);
  glutAddMenuEntry("Exit", MENU_EXIT);

  glutAttachMenu(GLUT_RIGHT_BUTTON);

  /* loop for ever */
  glutMainLoop();
}
