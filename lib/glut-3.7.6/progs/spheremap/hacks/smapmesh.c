
/* smapmesh.c - construct a cube map from sphere map via warp
   mesh */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>

#if defined(GL_EXT_texture_object) && !defined(GL_VERSION_1_1)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#endif

static int emphasize = 0;
static int clearToWhite = 0;

/* (x,y,z) reflection vector --> (s,t) sphere map coordinates */
void
rvec2st(float v[3], float st[2])
{
  double m;

  /** In Section 2.10.4 ("Generating texture coordinates") of
      the OpenGL 1.1 specification, you will find the
      GL_SPHERE_MAP equations:

      n' = normal after transformation to eye coordinates
      u  = unit vector from origin to vertex in eye coordinates

      (rx, ry, rz) = u - 2 * n' * transpose(n') * u

      m  = 2 * sqrt(rx^2 + ry^2 + (rz + 1)^2))

      s = rx/m + 0.5 t = ry/m + 0.5

      The equation for calculating (rx, ry, rz) is the equation
      for calculating the reflection vector for a surface and
      observer.  The explanation and derivation for this
      equation is found in Roger's "Procedural Elements for
      Computer Graphics" 2nd ed. in Section 5-5 ("Determining
      the Reflection Vector"). Note that Roger's convention has
      the Z axis in the opposite direction from the OpenGL
      convention. */

  m = 2 * sqrt(v[0] * v[0] + v[1] * v[1] + (v[2] + 1) * (v[2] + 1));

  st[0] = v[0] / m + 0.5;
  st[1] = v[1] / m + 0.5;
}

/* (s,t) sphere map coordinate --> reflection verctor (x,y,z) */
void
st2rvec(float s, float t, float *xp, float *yp, float *zp)
{
  double rx, ry, rz;
  double tmp1, tmp2;

  /** Using algebra to invert the sphere mapping equations shown 
      above in rvec2st, you get:

      rx = 2*sqrt(-4*s^2 + 4*s - 4*t^2 + 4*t - 1)*(2*s-1)
      ry = 2*sqrt(-4*s^2 + 4*s - 4*t^2 + 4*t - 1)*(2*t-1)
      rz = -8*s^2 + 8*s - 8*t^2 + 8*t - 3

      The C code below eliminates common subexpressions. */

  tmp1 = s * (1 - s) + t * (1 - t);
  tmp2 = 2 * sqrt(4 * tmp1 - 1);

  rx = tmp2 * (2 * s - 1);
  ry = tmp2 * (2 * t - 1);
  rz = 8 * tmp1 - 3;

  *xp = (float) rx;
  *yp = (float) ry;
  *zp = (float) rz;
}

/* For best results (ie, to avoid cracks in the sphere map
   construction, XSTEPS, YSTEPS, and SPOKES should all be
   equal. */

/* Increasing the nSTEPS and RINGS constants below will give
   you a better approximation to the sphere map image warp at
   the cost of more polygons to render the image warp.  My bet
   is that no will be able to the improved quality of a higher
   level of tessellation. */

#define XSTEPS  8
#define YSTEPS  8
#define SPOKES  8
#define RINGS   3

typedef struct _STXY {
  GLfloat s, t;
  GLfloat x, y;
} STXY;

STXY face[5][YSTEPS][XSTEPS];
STXY back[4][RINGS][SPOKES];

static struct {
  int xl;
  int yl;
  int zl;
  float dir;
} faceInfo[5] = {
  { 0, 1, 2, 1.0 } ,                     /* front */
  { 0, 2, 1, 1.0 } ,                     /* top */
  { 0, 2, 1, -1.0 } ,                     /* bottom */
  { 1, 2, 0, -1.0 } ,                     /* left */
  { 1, 2, 0, 1.0 } ,                     /* right */
};

static struct {
  int xl;
  int yl;
  float dir;
} edgeInfo[4] = {
  { 0, 1, -1.0 } ,
  { 0, 1, 1.0 } ,
  { 1, 0, -1.0 } ,
  { 1, 0, 1.0 }
};

void
makeSpheremapMapping(void)
{
  float st[2];          /* (s,t) coordinate  */
                        /* range=[0..1,0..1] */

  float v[3];           /* (x,y,z) location on cube map */
                        /* range=[-1..1,-1..1,-1..1] */

  float rv[3];          /* reflection vector, ie. cube map */
                        /* location normalized onto unit sphere */

  float len;            /* distance from v[3] to origin */
                        /* for converting to rv[3] */

  int side;             /* which of 5 faces (all but back face) */

  int i, j;
  int xl, yl, zl;       /* renamed X, Y, Z index */

  int edge;             /* which edge of back face */

  float sc, tc;

  /* for the front and four side faces */
  for (side = 0; side < 5; side++) {

    /* use faceInfo to parameterize face construction */
    xl = faceInfo[side].xl;
    yl = faceInfo[side].yl;
    zl = faceInfo[side].zl;
    /* cube map "Z" coordinate */
    v[zl] = faceInfo[side].dir;

    for (i = 0; i < YSTEPS; i++) {
      /* cube map "Y" coordinate */
      v[yl] = 2.0 / (YSTEPS - 1) * i - 1.0;
      for (j = 0; j < XSTEPS; j++) {
        /* cube map "X" coordinate */
        v[xl] = 2.0 / (XSTEPS - 1) * j - 1.0;

        /* normalize cube map location to construct */
        /* reflection vector */
        len = sqrt(1.0 + v[xl] * v[xl] + v[yl] * v[yl]);
        rv[0] = v[0] / len;
        rv[1] = v[1] / len;
        rv[2] = v[2] / len;

        /* map reflection vector to sphere map (s,t) */
        /* NOTE: face[side][i][j] (x,y) gets updated */
        rvec2st(rv, &face[side][i][j].x);

        /* update texture coordinate, */
        /* normalize [-1..1,-1..1] to [0..1,0..1] */
        face[side][i][j].s = (v[xl] + 1.0) / 2.0;
        face[side][i][j].t = (v[yl] + 1.0) / 2.0;
      }
    }
  }

  /* The back face must be specially handled.  The center point 
     in the back face of a cube map becomes a a singularity
     around the circular edge of a sphere map. */

  /* Carefully work from each edge of the back face to center
     of back face mapped to the outside of the sphere map. */

  /* cube map "Z" coordinate, always -1 since backface */
  v[2] = -1;

  /* for each edge */
  /* [x=-1, y=-1..1, z=-1] */
  /* [x= 1, y=-1..1, z=-1] */
  /* [x=-1..1, y=-1, z=-1] */
  /* [x=-1..1, y= 1, z=-1] */
  for (edge = 0; edge < 4; edge++) {
    /* cube map "X" coordinate */
    v[edgeInfo[edge].xl] = edgeInfo[edge].dir;
    for (j = 0; j < SPOKES; j++) {
      /* cube map "Y" coordinate */
      v[edgeInfo[edge].yl] = 2.0 / (SPOKES - 1) * j - 1.0;

      /* normalize cube map location to construct */
      /* reflection vector */
      len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
      rv[0] = v[0] / len;
      rv[1] = v[1] / len;
      rv[2] = v[2] / len;

      /* Map reflection vector to sphere map (s,t). */
      rvec2st(rv, st);

      /* determine distinance from the center of sphere */
      /* map (0.5,0.5) to (s,t) */
      len = sqrt((st[0] - 0.5) * (st[0] - 0.5) + (st[1] - 0.5) * (st[1] - 0.5));

      /* calculate (s,t) location extended to the singularity */
      /* at the center of the back face (ie, extend to */
      /* circle edge of the sphere map) */
      sc = (st[0] - 0.5) / len * 0.5 + 0.5;
      tc = (st[1] - 0.5) / len * 0.5 + 0.5;

      /* (s,t) at back face edge */
      back[edge][0][j].s = (v[0] + 1.0) / 2.0;
      back[edge][0][j].t = (v[1] + 1.0) / 2.0;
      back[edge][0][j].x = st[0];
      back[edge][0][j].y = st[1];

      /* If just two rings, we just generate a back face edge
         vertex and a center vertex (2 rings), but if there are 
         more rings, we carefully interpolate between the edge
         and center vertices.  Notice how st2rvec is used to
         map the interpolated (s,t) into a reflection vector
         that must then be extended to the back cube face (it
         is not correct to just interpolate the texture
         coordinates!). */
      if (RINGS > 2) {
        float s, t;     /* interpolated (s,t) */

        float ds, dt;   /* delta s and delta t */

        float x, y, z;

        /* Start interpolating from the edge. */
        s = st[0];
        t = st[1];

        /* Calculate delta s and delta t for interpolation. */
        ds = (sc - s) / (RINGS - 1);
        dt = (tc - t) / (RINGS - 1);

        for (i = 1; i < RINGS - 1; i++) {
          /* Incremental interpolation of (s,t). */
          s = s + ds;
          t = t + dt;

          /* Calculate reflection vector from interpolated */
          /* (s,t). */
          st2rvec(s, t, &x, &y, &z);
          /* Assert that z must be on the back cube face. */
          assert(z <= -sqrt(1.0 / 3.0));
          /* Extend reflection vector out of back cube face. */
          /* Note: z is negative value so negate z to avoid */
          /* inverting x and y! */
          x = x / -z;
          y = y / -z;

          back[edge][i][j].s = (x + 1.0) / 2.0;
          back[edge][i][j].t = (y + 1.0) / 2.0;
          back[edge][i][j].x = s;
          back[edge][i][j].y = t;
        }
      }
      /* (s,t) at circle edge of the sphere map is ALWAYS */
      /* at center of back cube map face */
      back[edge][RINGS - 1][j].s = 0.5;
      back[edge][RINGS - 1][j].t = 0.5;
      /* location of singularity at the edge of the sphere map */
      back[edge][RINGS - 1][j].x = sc;
      back[edge][RINGS - 1][j].y = tc;
    }
  }
}

void
drawSphereMapping(GLuint texobj[6])
{
  int side, i, j;

  /* five front and side faces */
  for (side = 0; side < 5; side++) {

    if (emphasize == side + 1) {
      glLineWidth(3.0);
    }
    /* bind to texture for given face of cube map */
    glBindTexture(GL_TEXTURE_2D, texobj[side]);
    for (i = 0; i < YSTEPS - 1; i++) {
      glBegin(GL_QUAD_STRIP);
      for (j = 0; j < XSTEPS; j++) {
        glTexCoord2fv(&face[side][i][j].s);
        glVertex2fv(&face[side][i][j].x);
        glTexCoord2fv(&face[side][i + 1][j].s);
        glVertex2fv(&face[side][i + 1][j].x);
      }
      glEnd();
    }

    if (emphasize == side + 1) {
      glLineWidth(1.0);
    }
  }

  /* Back face specially rendered for its singularity! */

  if (emphasize == 6) {
    glLineWidth(3.0);
  }
  /* Bind to texture for back face of cube map. */
  glBindTexture(GL_TEXTURE_2D, texobj[side]);
  for (side = 0; side < 4; side++) {
    for (j = 0; j < RINGS - 1; j++) {
      glBegin(GL_QUAD_STRIP);
      for (i = 0; i < SPOKES; i++) {
        glTexCoord2fv(&back[side][j][i].s);
        glVertex2fv(&back[side][j][i].x);
        glTexCoord2fv(&back[side][j + 1][i].s);
        glVertex2fv(&back[side][j + 1][i].x);
      }
      glEnd();
    }
  }

  if (emphasize == 6) {
    glLineWidth(1.0);
  }
}

static void
textureInit(void)
{
  static int width = 8, height = 8;
  static GLubyte tex1[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0};

  GLubyte tex[64][3];
  GLint i, j;

  /* setup first texture object */
  glBindTexture(GL_TEXTURE_2D, 1);
  /* red on white */
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      int p = i * width + j;
      if (tex1[(height - i - 1) * width + j]) {
        tex[p][0] = 255;
        tex[p][1] = 0;
        tex[p][2] = 0;
      } else {
        tex[p][0] = 255;
        tex[p][1] = 255;
        tex[p][2] = 255;
      }
    }
  }
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height,
    GL_RGB, GL_UNSIGNED_BYTE, tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR);
}

static void
display(void)
{
  /* All faces get the same texture! */
  GLuint texobjs[6] = {1, 1, 1, 1, 1, 1};

  /* Clear to gray. */
  if (clearToWhite) {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glColor3f(0.0, 0.0, 0.0);
    emphasize = 0;
    glLineWidth(3.0);
  } else {
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glColor3f(1.0, 1.0, 1.0);
    glLineWidth(1.0);
  }
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, 1, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  drawSphereMapping(texobjs);

  glutSwapBuffers();
}

static void
menu(int value)
{
  switch (value) {
  case 1:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case 2:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  case 3:
    glEnable(GL_TEXTURE_2D);
    break;
  case 4:
    glDisable(GL_TEXTURE_2D);
    break;
  case 5:
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    break;
  case 6:
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    break;
  case 7:
    emphasize++;
    emphasize %= 7;
    break;
  case 8:
    clearToWhite = 1;
    break;
  case 9:
    clearToWhite = 0;
    break;
  }
  glutPostRedisplay();
}

void
keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:
    exit(0);
    break;
  case ' ':
    menu(7);
    break;
  case 'a':
    menu(5);
    break;
  case 'w':
    menu(2);
    break;
  case 'f':
    menu(1);
    break;
  case 't':
    menu(3);
    break;
  case 'n':
    menu(4);
    break;
  }
}

int
main(int argc, char **argv)
{
  makeSpheremapMapping();

  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow("smapmesh");

  textureInit();
  glEnable(GL_TEXTURE_2D);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glutDisplayFunc(display);

  glutKeyboardFunc(keyboard);

  glutCreateMenu(menu);
  glutAddMenuEntry("fill", 1);
  glutAddMenuEntry("wireframe", 2);
  glutAddMenuEntry("texture", 3);
  glutAddMenuEntry("no texture", 4);
  glutAddMenuEntry("antialias lines", 5);
  glutAddMenuEntry("aliased lines", 6);
  glutAddMenuEntry("switch emphasis", 7);
  glutAddMenuEntry("clear to white", 8);
  glutAddMenuEntry("clear to gray", 9);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glutMainLoop();

  return 0;
}
