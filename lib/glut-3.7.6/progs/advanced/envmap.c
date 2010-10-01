
/* envmap.c - David Blythe, SGI */

/* Texture environment mapping demo. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef _WIN32
#include <unistd.h>
#else
#define drand48() (((float) rand())/((float) RAND_MAX))
#endif
#include <string.h>
#include <GL/glut.h>
#include "texture.h"

#if defined(GL_VERSION_1_1)
/* Routines called directly. */
#elif defined(GL_EXT_texture_object) && defined(GL_EXT_copy_texture) && defined(GL_EXT_subtexture)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#define glGenTextures(A,B)     glGenTexturesEXT(A,B)
#define glDeleteTextures(A,B)  glDeleteTexturesEXT(A,B)
#define glCopyTexSubImage2D(A,B,C,D,E,F,G,H) glCopyTexSubImage2DEXT(A,B,C,D,E,F,G,H)
#else
#define glBindTexture(A,B)
#define glGenTextures(A,B)
#define glDeleteTextures(A,B)
#define glCopyTexSubImage2D(A,B,C,D,E,F,G,H)
#endif

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define OB_CUBE    0
#define OB_SPHERE  1
#define OB_SQUARE  2
#define OB_CYL     3
#define OB_TORUS   4
#define OB_HSPHERE 5

#define NOBJS 5
#define TDRAW 10

#define LIST_BASE 100
#define TOBJ_BASE 200
#define TEXSIZE   256   /* default texture-size */

typedef struct _vector {
  float x, y, z;
} vector_t;

typedef struct _face {
  char *filename;
  unsigned *buf;
  int width;
  int height;
  int components;
  vector_t u, v, n, o;  /* plane equation */
  float angle1;
  vector_t axis1;       /* for rotation */
  float angle2;
  vector_t axis2;
} face_t;

typedef struct _color {
  float r, g, b;
} color_t;

struct {                /* command-line options */
  int use_spheremap;
  char *spheremap_file;
  int hw;
  int size;
  int samples;
  int object;
  char *outfile;
  int tessellation;
} opts = {

  0, 0, 0, TEXSIZE, 4, OB_TORUS, 0, 30
};                      /* default settings */

void display(void);
void init(void);
void build_lists(void);
unsigned *render_spheremap(int *width, int *height,
  int *components, int doalloc);

/* strdup is actually not a standard ANSI C or POSIX routine
   so implement a private one.  OpenVMS does not have a strdup; Linux's
   standard libc doesn't declare strdup by default (unless BSD or SVID
   interfaces are requested). */
static char *
stralloc(const char *string)
{
  char *copy;

  copy = malloc(strlen(string) + 1);
  if (copy == NULL)
    return NULL;
  strcpy(copy, string);
  return copy;
}

void
vadd(vector_t * a, vector_t * b, vector_t * sum)
{
  sum->x = a->x + b->x;
  sum->y = a->y + b->y;
  sum->z = a->z + b->z;
}

void
vsub(vector_t * a, vector_t * b, vector_t * diff)
{
  diff->x = a->x - b->x;
  diff->y = a->y - b->y;
  diff->z = a->z - b->z;
}

void
vscale(vector_t * v, float scale)
{
  v->x *= scale;
  v->y *= scale;
  v->z *= scale;
}

float
vdot(vector_t * u, vector_t * v)
{
  return (u->x * v->x + u->y * v->y + u->z * v->z);
}

void
vreflect(vector_t * axis, vector_t * v, vector_t * r)
{
  vector_t t = *axis;

  vscale(&t, 2 * vdot(axis, v));
  vsub(&t, v, r);
}

int
intersect(vector_t * v)
{
  int f;
  float x, y, z;

  x = fabs(v->x);
  y = fabs(v->y);
  z = fabs(v->z);
  if (x >= y && x >= z)
    f = (v->x > 0) ? 2 : 0;
  else if (y >= x && y >= z)
    f = (v->y > 0) ? 4 : 5;
  else
    f = (v->z > 0) ? 3 : 1;

  return f;
}

face_t face[6] =
{
  {"../data/00.rgb", 0, 0, 0, 0,
    {0, 0, -1},
    {0, 1, 0},
    {-1, 0, 0},
    {-0.5, -0.5, 0.5}, 90.0,
    {0, 1, 0}, 0,
    {0, 0, 0}},
  {"../data/01.rgb", 0, 0, 0, 0,
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, -1},
    {-0.5, -0.5, -0.5}, 180.0,
    {0, 1, 0}, 0,
    {0, 0, 0}},
  {"../data/02.rgb", 0, 0, 0, 0,
    {0, 0, 1},
    {0, 1, 0},
    {1, 0, 0},
    {0.5, -0.5, -0.5}, 270.0,
    {0, 1, 0}, 0,
    {0, 0, 0}},
  {"../data/03.rgb", 0, 0, 0, 0,
    {-1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
    {0.5, -0.5, 0.5}, 0.0,
    {0, 1, 0}, 0,
    {0, 0, 0}},
  {"../data/04.rgb", 0, 0, 0, 0,
    {1, 0, 0},
    {0, 0, 1},
    {0, 1, 0},
    {-0.5, 0.5, -0.5}, 90.0,
    {1, 0, 0}, 180.0,
    {0, 1, 0}},
  {"../data/05.rgb", 0, 0, 0, 0,
    {1, 0, 0},
    {0, 0, -1},
    {0, -1, 0},
    {-0.5, -0.5, 0.5}, -90.0,
    {1, 0, 0}, 180.0,
    {0, 1, 0}}
};

void
sample(int facenum, float s, float t, color_t * c)
{
  face_t *f = &face[facenum];
  int xpos, ypos;
  unsigned char *p;

  xpos = s * f->width;
  ypos = t * f->height;

  p = (unsigned char *) &f->buf[ypos * f->width + xpos];
  c->r = p[0] / 255.0;
  c->g = p[1] / 255.0;
  c->b = p[2] / 255.0;
}

unsigned *
construct_spheremap(int *width, int *height,
  int *components)
{
  int i, j, x, y, f;
  unsigned *spheremap;
  unsigned char *lptr;
  int size = opts.size;
  color_t c, texel;
  vector_t v, r, p;
  float s, t, temp, k;
  int samples;

  /* Read in the 6 faces of the environment */
  for (i = 0; i < 6; i++) {
    face[i].buf = read_texture(face[i].filename, &face[i].width,
      &face[i].height, &face[i].components);
    if (!face[i].buf) {
      fprintf(stderr, "Error: cannot load image %s\n", face[i].filename);
      exit(1);
    }
  }

  *components = face[0].components;
  *width = *height = size;
  samples = opts.samples;

  spheremap = (unsigned *) malloc(size * size * sizeof(unsigned));
  if (!spheremap) {
    perror("malloc");
    exit(1);
  }
  lptr = (unsigned char *) spheremap;

  /* Calculate sphere-map by rendering a perfectly reflective solid sphere. */

  for (y = 0; y < size; y++)
    for (x = 0; x < size; x++) {

      texel.r = texel.g = texel.b = 0.0;
      for (j = 0; j < samples; j++) {
        s = (x + (float) drand48()) / size - 0.5;
        t = (y + (float) drand48()) / size - 0.5;

        temp = s * s + t * t;
        if (temp >= 0.25) {  /* point not on sphere */
          c.r = c.g = c.b = 0;
          continue;
        }
        /* get point on sphere */
        p.x = s;
        p.y = t;
        p.z = sqrt(0.25 - temp);
        vscale(&p, 2.0);
        /* ray from infinity (eyepoint) to surface */
        v.x = 0.0;
        v.y = 0.0;
        v.z = 1.0;

        /* get reflected ray */
        vreflect(&p, &v, &r);

        /* Intersect reflected ray with cube */
        f = intersect(&r);
        k = vdot(&face[f].o, &face[f].n) / vdot(&r, &face[f].n);
        vscale(&r, k);
        vsub(&r, &face[f].o, &v);

        /* Get texture map-indices */
        s = vdot(&v, &face[f].u);
        t = vdot(&v, &face[f].v);

        /* Sample to get color */
        sample(f, s, t, &c);

        texel.r += c.r;
        texel.g += c.g;
        texel.b += c.b;
      }

      lptr[0] = 255 * texel.r / samples;
      lptr[1] = 255 * texel.g / samples;
      lptr[2] = 255 * texel.b / samples;
      lptr[3] = 0xff;
      lptr += 4;
    }

  return (unsigned *) spheremap;
}

void
texture_init(void)
{
  unsigned *buf;
  int width, height, components;
  char filename[80];

  if (opts.use_spheremap) {
    strcpy(filename, opts.spheremap_file);
    buf = read_texture(filename, &width, &height, &components);
    if (components == 3)
      components++;
    if (!buf) {
      fprintf(stderr, "Error: cannot load image %s\n", filename);
      exit(1);
    }
  } else {
    buf = (opts.hw) ?
      render_spheremap(&width, &height, &components, 1) :
      construct_spheremap(&width, &height, &components);

    if (!buf) {
      fprintf(stderr, "Error: Cannot construct spheremap\n");
      exit(1);
    }
  }

  glBindTexture(GL_TEXTURE_2D, TOBJ_BASE + TDRAW);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  gluBuild2DMipmaps(GL_TEXTURE_2D, components, width, height,
    GL_RGBA, GL_UNSIGNED_BYTE, buf);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR);

  glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);

  free(buf);
}

void
texture_init_from_spheremap(void)
{
  int w, h, components;

  render_spheremap(&w, &h, &components, 0);

  glReadBuffer(GL_BACK);
  glBindTexture(GL_TEXTURE_2D, TOBJ_BASE + TDRAW);
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
}

void
parse(int argc, char *argv[])
{
  int i = 1;
  char *usage =
  "Usage: map [-size n] [-samples n] [-help] \n\
\t[-sphere filename | -cubemap [0-5].rgb]\n\
\n\
\t-size n : specify size of sphere-map (when generated from cubemap)\n\
\t-samples n : #samples to use per pixel of the spheremap\n\
\t-sphere file.rgb  : specify spheremap-image\n\
\t-cubemap [0-5].rgb: specify 6 cubemap files\n\
\t-out file.rgb: save generated spheremap to file\n\
\t-hw   : Use hardware texture mapping to create sphere-map\n\
\t-help : print this message\n";

#define check_arg(i, n, str) \
  if (argc < i+n) {       \
    fprintf(stderr, "%s needs an argument\n", str);  \
    fprintf(stderr, usage); \
    exit(1); \
  }

  while (i < argc) {
    if (!strcmp(argv[i], "-size")) {
      check_arg(i, 1, "-size");
      opts.size = atoi(argv[++i]);

    } else if (!strcmp(argv[i], "-samples")) {
      check_arg(i, 1, "-samples");
      opts.samples = atoi(argv[++i]);

    } else if (!strcmp(argv[i], "-out")) {
      check_arg(i, 1, "-out");
      opts.outfile = stralloc(argv[++i]);

    } else if (!strcmp(argv[i], "-sphere")) {
      opts.use_spheremap = 1;

    } else if (!strcmp(argv[i], "-cubemap")) {
      int j;
      check_arg(i, 6, "-cubemap");
      for (j = 0; j < 6; j++)
        face[j].filename = stralloc(argv[++i]);

    } else if (!strcmp(argv[i], "-help")) {
      fprintf(stderr, usage);
      exit(0);

    } else if (!strcmp(argv[i], "-hw")) {
      opts.hw = 1;

    } else {
      if (opts.use_spheremap && !opts.spheremap_file)
        opts.spheremap_file = stralloc(argv[i]);
      else {
        fprintf(stderr, "Error: unrecognized option %s\n", argv[i]);
        fprintf(stderr, usage);
        exit(1);
      }
    }
    i++;
  }                     /* end-while */
}

#ifdef use_copytex
static int currwidth = TEXSIZE;
static int currheight = TEXSIZE;
#else
static int currwidth = 400;
static int currheight = 400;
#endif

static int do_spheremap = 0, do_alloc = 0;
static GLfloat rotv[] =
{0., 0., 0.};
static GLfloat rots[] =
{0., 0., 0.};
static GLfloat plane[4][3] =
{
  {1.0, -1.0, 0.0},
  {1.0, 1.0, 0.0},
  {-1.0, 1.0, 0.0},
  {-1.0, -1.0, 0.0}
};

static GLfloat cube[6][4][3] =
{
  {
    {1.0, -1.0, -1.0},  /* counter-clockwise faces */
    {-1.0, -1.0, -1.0},
    {-1.0, 1.0, -1.0},
    {1.0, 1.0, -1.0}
  },
  {
    {1.0, -1.0, 1.0},
    {1.0, -1.0, -1.0},
    {1.0, 1.0, -1.0},
    {1.0, 1.0, 1.0}
  },
  {
    {-1.0, -1.0, 1.0},
    {1.0, -1.0, 1.0},
    {1.0, 1.0, 1.0},
    {-1.0, 1.0, 1.0}
  },
  {
    {-1.0, -1.0, -1.0},
    {-1.0, -1.0, 1.0},
    {-1.0, 1.0, 1.0},
    {-1.0, 1.0, -1.0}
  },
  {
    {1.0, 1.0, 1.0},
    {1.0, 1.0, -1.0},
    {-1.0, 1.0, -1.0},
    {-1.0, 1.0, 1.0}
  },
  {
    {1.0, -1.0, -1.0},
    {1.0, -1.0, 1.0},
    {-1.0, -1.0, 1.0},
    {-1.0, -1.0, -1.0}
  }
};

static float norm[6][3] =
{
  {0.0, 0.0, -1.0},
  {1.0, 0.0, 0.0},
  {0.0, 0.0, 1.0},
  {-1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, -1.0, 0.0}
};

void
reshape(int w, int h)
{
  currwidth = w;
  currheight = h;
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(40.0, (GLfloat) w / (GLfloat) h, 1.0, 20.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0, 0, 6,
    0, 0, 0,
    0, 1, 0);
}

/* ARGSUSED1 */
void
keys(unsigned char key, int x, int y)
{
  switch (key) {

  case 'o':            /* switch between objects */
    opts.object = (opts.object + 1) % NOBJS;
    glutPostRedisplay();
    break;

  case '+':            /* change tessellation */
    opts.tessellation += 2;
    build_lists();
    glutPostRedisplay();
    break;

  case '-':
    opts.tessellation -= 2;
    if (opts.tessellation < 4)
      opts.tessellation = 4;
    build_lists();
    glutPostRedisplay();
    break;

  case 's':            /* toggle between spheremap generation */
    /* mode and display mode */
    do_spheremap ^= 1;
    if (!do_spheremap)  /* switch back to normal mode */
      do_alloc = 1;
    glutPostRedisplay();
    break;

  case 'h':
  case 'H':
    printf("\nKey functions\n");
    printf("\to: switch objects\n");
    printf("\ts: switch to spheremap generation mode\n");
    printf("\t+: increase tessellation\n");
    printf("\t-: decrease tessellation\n");
    printf("\th: help\n");
    printf("\tESC: quit\n");
    printf("\tleft/right arrow-keys: Rotate around X axis\n");
    printf("\tup/down arrow-keys: Rotate around Y axis\n");
    printf("\tpage-up/pgdown arrow-keys: Rotate around Z axis\n");
    break;

  case 27:
    exit(0);
  }
}

/* ARGSUSED1 */
void
special_keys(int key, int x, int y)
{
  GLfloat *vect;

  if (do_spheremap)
    vect = rots;
  else
    vect = rotv;

  switch (key) {
  case GLUT_KEY_LEFT:
    vect[1] -= 0.5;
    glutPostRedisplay();
    break;
  case GLUT_KEY_RIGHT:
    vect[1] += 0.5;
    glutPostRedisplay();
    break;
  case GLUT_KEY_UP:
    vect[0] -= 0.5;
    glutPostRedisplay();
    break;
  case GLUT_KEY_DOWN:
    vect[0] += 0.5;
    glutPostRedisplay();
    break;
  case GLUT_KEY_PAGE_UP:
    vect[2] -= 0.5;
    glutPostRedisplay();
    break;
  case GLUT_KEY_PAGE_DOWN:
    vect[2] += 0.5;
    glutPostRedisplay();
    break;
  }
}

/* Use mouse buttons to generate spheremap on the fly while the objects are
   being displayed.  */
static void
motion(int x, int y)
{
  rots[1] = 180.0 * x / currwidth - 90.0;
  rots[0] = 180.0 * y / currheight - 90.0;
#ifdef use_copytex
  if (!do_spheremap)
    texture_init_from_spheremap();
#endif
  glutPostRedisplay();
}

void
menu(int value)
{
  keys((unsigned char) value, 0, 0);
}

#if defined(GL_VERSION_1_1)

static int
supportsOneDotOne(void)
{
  const char *version;
  int major, minor;

  version = (char *) glGetString(GL_VERSION);
  if (sscanf(version, "%d.%d", &major, &minor) == 2)
    return major >= 1 && minor >= 1;
  return 0;            /* OpenGL version string malformed! */
}

#endif

int
main(int argc, char *argv[])
{
  int hasExtendedTextures;

  parse(argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  glutInitWindowSize(currwidth, currheight);
  glutCreateWindow("Environment map");

#if defined(GL_VERSION_1_1)
  hasExtendedTextures = supportsOneDotOne();
  if(!hasExtendedTextures) {
    fprintf(stderr,
      "envmap: This example requires OpenGL 1.1.\n");
    exit(1);
  }
#elif defined(GL_EXT_texture_object) && defined(GL_EXT_copy_texture) && defined(GL_EXT_subtexture)
  hasExtendedTextures = glutExtensionSupported("GL_EXT_subtexture")
    && glutExtensionSupported("GL_EXT_texture_object")
    && glutExtensionSupported("GL_EXT_copy_texture");
  if(!hasExtendedTextures) {
    fprintf(stderr,
      "envmap: This example requires the OpenGL EXT_subtexture, EXT_texture_object, and EXT_copy_texture extensions.\n");
    exit(1);
  }
#else
  hasExtendedTextures = 0;
  if(!hasExtendedTextures) {
    fprintf(stderr,
      "envmap: This example must be compiled with either OpenGL 1.1 or the OpenGL EXT_subtexture, EXT_texture_object, and EXT_copy_texture extensions.\n");
    exit(1);
  }
#endif

  init();

  glutReshapeFunc(reshape);
  glutKeyboardFunc(keys);
  glutSpecialFunc(special_keys);
  if (opts.hw)
    glutMotionFunc(motion);
  glutDisplayFunc(display);
  glutCreateMenu(menu);
  glutAddMenuEntry("Switch object", 'o');
  glutAddMenuEntry("Up tessellation", '+');
  glutAddMenuEntry("Lower tessellation", '-');
  glutAddMenuEntry("Quit", 27);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}

void
build_cube(void)
{
  int i;

  glNewList(LIST_BASE + OB_CUBE, GL_COMPILE);
  for (i = 0; i < 6; i++) {
    glBegin(GL_QUADS);
    glNormal3fv(norm[i]);
    glVertex3fv(cube[i][0]);
    glVertex3fv(cube[i][1]);
    glVertex3fv(cube[i][2]);
    glVertex3fv(cube[i][3]);
    glEnd();
  }
  glEndList();
}

void
build_sphere(int tess)
{
  float r = 1.0, r1, r2, z1, z2;
  float theta, phi;
  int nlon = tess, nlat = tess;
  int i, j;

  glNewList(LIST_BASE + OB_SPHERE, GL_COMPILE);
  glBegin(GL_TRIANGLE_FAN);
  theta = M_PI * 1.0 / nlat;
  r2 = r * sin(theta);
  z2 = r * cos(theta);
  glNormal3f(0.0, 0.0, 1.0);
  glVertex3f(0.0, 0.0, r);
  for (j = 0, phi = 0.0; j <= nlon; j++, phi = 2 * M_PI * j / nlon) {
    glNormal3f(r2 * cos(phi), r2 * sin(phi), z2);
    glVertex3f(r2 * cos(phi), r2 * sin(phi), z2);  /* top */
  }
  glEnd();

  for (i = 2; i < nlat; i++) {
    theta = M_PI * i / nlat;
    r1 = r * sin(M_PI * (i - 1) / nlat);
    z1 = r * cos(M_PI * (i - 1) / nlat);
    r2 = r * sin(theta);
    z2 = r * cos(theta);
    glBegin(GL_QUAD_STRIP);
    for (j = 0, phi = 0; j <= nlat; j++, phi = 2 * M_PI * j / nlon) {
      glNormal3f(r1 * cos(phi), r1 * sin(phi), z1);
      glVertex3f(r1 * cos(phi), r1 * sin(phi), z1);
      glNormal3f(r2 * cos(phi), r2 * sin(phi), z2);
      glVertex3f(r2 * cos(phi), r2 * sin(phi), z2);
    }
    glEnd();
  }

  glBegin(GL_TRIANGLE_FAN);
  theta = M_PI * (nlat - 1) / nlat;
  r2 = r * sin(theta);
  z2 = r * cos(theta);
  glNormal3f(0.0, 0.0, -1.0);
  glVertex3f(0.0, 0.0, -r);
  for (j = nlon, phi = 0.0; j >= 0; j--, phi = 2 * M_PI * j / nlon) {
    glNormal3f(r2 * cos(phi), r2 * sin(phi), z2);
    glVertex3f(r2 * cos(phi), r2 * sin(phi), z2);  /* bottom */
  }
  glEnd();
  glEndList();
}

/* Same as above routine except that we use homogeneous co-ordinates. Each
   component (including w) is multiplied by z */
void
build_special_sphere(int tess)
{
  float r = 1.0, r1, r2, z1, z2;
  float theta, phi;
  int nlon = tess, nlat = tess;
  int i, j;

  glNewList(LIST_BASE + OB_HSPHERE, GL_COMPILE);
  glBegin(GL_TRIANGLE_FAN);
  theta = M_PI * 1.0 / nlat;
  r2 = r * sin(theta);
  z2 = r * cos(theta);
  glNormal3f(0.0, 0.0, 1.0);
  glVertex4f(0.0, 0.0, r * r, r);
  for (j = 0, phi = 0.0; j <= nlon; j++, phi = 2 * M_PI * j / nlon) {
    glNormal3f(r2 * cos(phi), r2 * sin(phi), z2);
    glVertex4f(r2 * cos(phi) * z2, r2 * sin(phi) * z2, z2 * z2, z2);  /* top */
  }
  glEnd();

  for (i = 2; i < nlat; i++) {
    theta = M_PI * i / nlat;
    r1 = r * sin(M_PI * (i - 1) / nlat);
    z1 = r * cos(M_PI * (i - 1) / nlat);
    r2 = r * sin(theta);
    z2 = r * cos(theta);

    if (fabs(z1) < 0.01 || fabs(z2) < 0.01)
      break;

    glBegin(GL_QUAD_STRIP);
    for (j = 0, phi = 0; j <= nlat; j++, phi = 2 * M_PI * j / nlon) {
      glNormal3f(r1 * cos(phi), r1 * sin(phi), z1);
      glVertex4f(r1 * cos(phi) * z1, r1 * sin(phi) * z1, z1 * z1, z1);
      glNormal3f(r2 * cos(phi), r2 * sin(phi), z2);
      glVertex4f(r2 * cos(phi) * z2, r2 * sin(phi) * z2, z2 * z2, z2);
    }
    glEnd();
  }

  glBegin(GL_TRIANGLE_FAN);
  theta = M_PI * (nlat - 1) / nlat;
  r2 = r * sin(theta);
  z2 = r * cos(theta);
  glNormal3f(0.0, 0.0, -1.0);
  glVertex4f(0.0, 0.0, -r * -r, -r);
  for (j = nlon, phi = 0.0; j >= 0; j--, phi = 2 * M_PI * j / nlon) {
    glNormal3f(r2 * cos(phi), r2 * sin(phi), z2);
    glVertex4f(r2 * cos(phi) * z2, r2 * sin(phi) * z2, z2 * z2, z2);  /* bottom 

                                                                       */
  }
  glEnd();
  glEndList();
}

void
build_square(void)
{
  glNewList(LIST_BASE + OB_SQUARE, GL_COMPILE);
  glBegin(GL_POLYGON);
  glNormal3f(0.0, 0.0, 1.0);
  glVertex3fv(plane[0]);
  glVertex3fv(plane[1]);
  glVertex3fv(plane[2]);
  glVertex3fv(plane[3]);
  glEnd();
  glEndList();
}

void
build_cylinder(int tess)
{
  int slices = tess, stacks = tess;
  int i, j;
  GLfloat phi, z1, r, z2;

  glNewList(LIST_BASE + OB_CYL, GL_COMPILE);
  z1 = 2.0;
  r = 1.0;
  glBegin(GL_TRIANGLE_FAN);
  glNormal3f(0.0, 0.0, 1.0);
  glVertex3f(0.0, 0.0, z1);
  for (i = 0; i <= slices; i++) {
    phi = M_PI * 2.0 * i / slices;
    glVertex3f(r * cos(phi), r * sin(phi), z1);
  }
  glEnd();

  for (i = 0, z2 = 0.0, z1 = 2.0 / stacks; i < stacks; i++) {
    glBegin(GL_QUAD_STRIP);
    for (j = 0, phi = 0; j <= slices; j++, phi = M_PI * 2.0 * j / slices) {
      glNormal3f(r * cos(phi), r * sin(phi), 0);
      glVertex3f(r * cos(phi), r * sin(phi), z1);
      glVertex3f(r * cos(phi), r * sin(phi), z2);
    }
    glEnd();
    z1 += 2.0 / stacks;
    z2 += 2.0 / stacks;
  }

  z2 = 0.0;
  glBegin(GL_TRIANGLE_FAN);
  glNormal3f(0.0, 0.0, -1.0);
  glVertex3f(0.0, 0.0, z2);
  for (i = slices; i >= 0; i--) {
    phi = M_PI * 2.0 * i / slices;
    glVertex3f(r * cos(phi), r * sin(phi), z2);
  }
  glEnd();
  glEndList();
}

void
build_torus(int tess)
{
  int i, j, k, l;
  int numg = 1.2 * tess;
  int nums = tess;
  GLfloat x, y, z;
  GLfloat theta, phi;
  const GLfloat twopi = 2.0 * M_PI;
  const GLfloat rg = 2.0, rs = 1.0;

  glNewList(LIST_BASE + OB_TORUS, GL_COMPILE);
  for (i = 0; i < numg; i++) {

    glBegin(GL_QUAD_STRIP);
    for (j = 0; j <= nums; j++) {
      phi = twopi * j / nums;

      for (k = 0; k <= 1; k++) {
        l = (i + k) % numg;
        theta = twopi * l / numg;

        glNormal3f(rs * cos(phi) * cos(theta),
          rs * cos(phi) * sin(theta),
          rs * sin(phi));

        x = (rg + rs * cos(phi)) * cos(theta);
        y = (rg + rs * cos(phi)) * sin(theta);
        z = rs * sin(phi);
        glVertex3f(x, y, z);
      }
    }
    glEnd();
  }
  glEndList();
}

void
display(void)
{
  static int once = 0;

  if (!once && opts.hw) {
    texture_init();
    once = 1;
  }
  if (do_spheremap || do_alloc) {
    int w, h, comp;
    if (do_alloc) {
      texture_init();
      do_alloc = 0;
    } else
      render_spheremap(&w, &h, &comp, 0);

  } else {

    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, TOBJ_BASE + TDRAW);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glRotatef(rotv[0], 1, 0, 0);
    glRotatef(rotv[1], 0, 1, 0);
    glRotatef(rotv[2], 0, 0, 1);

    switch (opts.object) {

    case OB_SQUARE:
    case OB_CUBE:
    case OB_SPHERE:
      glCallList(LIST_BASE + opts.object);
      break;

    case OB_CYL:
      glTranslatef(0.0, 0.0, -1.0);
      glCallList(LIST_BASE + OB_CYL);
      break;

    case OB_TORUS:
      glScalef(0.6, 0.6, 0.6);
      glEnable(GL_NORMALIZE);
      glCallList(LIST_BASE + OB_TORUS);
      glDisable(GL_NORMALIZE);
      break;

    default:
      printf("Eh?\n");
    }

    glLineWidth(2.0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(4.0, 0.0, 0.0);

    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 4.0, 0.0);

    glColor3f(1.0, 1.0, 1.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 4.0);
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
  }

  glutSwapBuffers();
}

void
build_lists(void)
{
  build_square();
  build_cube();
  build_sphere(opts.tessellation);
  build_cylinder(opts.tessellation);
  build_torus(opts.tessellation);
  build_special_sphere(opts.tessellation + 10);
}

void
init(void)
{
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  glClearColor(0.0, 0.0, 0.0, 1.0);
  build_lists();
  texture_init();
}

void
err(void)
{
  printf("error=%#x\n", glGetError());
}

/* Use projective textures to generate sphere-map */
unsigned *
render_spheremap(int *width, int *height,
  int *components, int doalloc)
{
  int i, j, k;
  GLfloat p[4];
  static unsigned *spheremap = NULL;
  static int texread_done = 0;

  if (!opts.hw)         /* shouldn't come here */
    return NULL;

  for (i = 0; i < 6; i++) {
    if (!texread_done) {
      face[i].buf = read_texture(face[i].filename, &face[i].width,
        &face[i].height, &face[i].components);
      if (!face[i].buf) {
        fprintf(stderr, "Error: cannot load image %s\n", face[i].filename);
        exit(1);
      }
      for (j = 0; j < face[i].height; j++)  /* texture border hack!! */
        for (k = 0; k < face[i].width; k++)
          if (j < 1 || k < 1 ||
            j > face[i].height - 2 || k > face[i].width - 2) {
            unsigned char *p = (unsigned char *) &face[i].buf[face[i].width * j + k];
            p[3] = 0;   /* zero out alpha */
          }
    }
    glBindTexture(GL_TEXTURE_2D, TOBJ_BASE + i);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexImage2D(GL_TEXTURE_2D, 0, 4,
      face[i].width, face[i].height, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, face[i].buf);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    p[0] = 2.0;
    p[1] = p[2] = p[3] = 0.0;  /* 2zx */
    glTexGenfv(GL_S, GL_OBJECT_PLANE, p);

    glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    p[0] = 0.0;
    p[1] = 2.0;
    p[2] = p[3] = 0.0;  /* 2zy */
    glTexGenfv(GL_T, GL_OBJECT_PLANE, p);

    glTexGenf(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    p[0] = p[1] = 0.0;
    p[2] = 0.0;
    p[3] = 2.0;         /* 2z */
    glTexGenfv(GL_R, GL_OBJECT_PLANE, p);

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
  }

  texread_done = 1;

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();

  glColor4f(1.0, 1.0, 1.0, 1.0);

  /* Initialize sphere-map colors, and viewing transformations */

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1, 1, -1, 1, 1.0, 100);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0, 0, 6,
    0, 0, 0,
    0, 1, 0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClearDepth(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Look at the sphere from the chosen viewing direction and render the
     sphere at origin.  */

  for (i = 0; i < 6; i++) {  /* for all faces */
    glBindTexture(GL_TEXTURE_2D, TOBJ_BASE + i);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(0.5, 0.5, 1.0);
    glTranslatef(1.0, 1.0, 0.0);
    glFrustum(-1.01, 1.01, -1.01, 1.01, 1.0, 100.0);
    if (face[i].angle2 != 0.) {
      glRotatef(face[i].angle2,
        face[i].axis2.x, face[i].axis2.y, face[i].axis2.z);
    }
    glRotatef(face[i].angle1,
      face[i].axis1.x, face[i].axis1.y, face[i].axis1.z);
    glRotatef(rots[0], 1, 0, 0);
    glRotatef(rots[1], 0, 1, 0);
    glRotatef(rots[2], 0, 0, 1);
    glTranslatef(0.0, 0.0, -1.00);

    glMatrixMode(GL_MODELVIEW);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCallList(LIST_BASE + OB_HSPHERE);
  }

  glDisable(GL_BLEND);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  if (doalloc) {
    /* read in current image and return it to be used as the spheremap */
    unsigned *temp;

    temp = (unsigned *)
      malloc(currwidth * currheight * sizeof(unsigned));
    spheremap = (unsigned *)
      malloc(opts.size * opts.size * sizeof(unsigned));

    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, currwidth, currheight, GL_RGBA,
      GL_UNSIGNED_BYTE, temp);
    gluScaleImage(GL_RGBA,
      currwidth, currheight, GL_UNSIGNED_BYTE, temp,
      opts.size, opts.size, GL_UNSIGNED_BYTE, spheremap);
    free(temp);
  }
  *width = *height = opts.size;
  *components = 4;
  return (unsigned *) spheremap;
}

