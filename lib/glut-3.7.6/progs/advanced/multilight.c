
/* Copyright (c) Mark J. Kilgard, 1997.  */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* This program demonstrates virtualization of OpenGL's lights.  The idea is
   that if an object is lit by many lights, it is computationally more
   efficient to calculate the approximate lighting contribution of the
   various lights per-object and only enable the "brightest" lights while
   rendering the object.  This also lets you render scenes with more lights
   than the OpenGL implementation light (usually 8).  Two approaches are
   used:  The "distance-based" approach only enables the 8 closest lights
   based purely on distance.  The "Lambertian-based" approach accounts for
   diffuse lighting contributions and approximates the diffuse contribution. */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MIN_VALUE(a,b) (((a)<(b))?(a):(b))
#define MAX_VALUE(a,b) (((a)>(b))?(a):(b))

enum {
  DL_LIGHT_SPHERE = 1,
  DL_BIG_SPHERE = 2,
  DL_ICO = 3
};

enum {
  M_SPHERE, M_ICO, M_LABELS, M_LINEAR, M_QUAD, M_REPORT_SIG,
  M_LAMBERTIAN, M_DISTANCE, M_TIME
};

typedef struct _LightInfo {
  GLfloat xyz[4];
  GLfloat *rgb;
  int enable;
} LightInfo;

typedef struct _LightBrightness {
  int num;
  GLfloat brightness;
} LightBrightness;

static int animation = 1;
static int labelLights = 1;
static int reportLightSignificance = 0;
static int brightnessModel = M_LAMBERTIAN;
static int numActiveLights;
static int timeFrames = 0;
static int singleBuffer = 0;
/* *INDENT-OFF* */

static GLfloat modelAmb[4] = {0.1, 0.1, 0.1, 1.0};
static GLfloat matAmb[4] = {0.2, 0.2, 0.2, 1.0};
static GLfloat matDiff[4] = {0.8, 0.8, 0.8, 1.0};
static GLfloat matSpec[4] = {0.4, 0.4, 0.4, 1.0};
static GLfloat matEmission[4] = {0.0, 0.0, 0.0, 1.0};

GLfloat red[] = {1.0, 0.0, 0.0, 1.0};
GLfloat green[] = {0.0, 1.0, 0.0, 1.0};
GLfloat blue[] = {0.0, 0.0, 1.0, 1.0};
GLfloat yellow[] = {1.0, 1.0, 0.0, 1.0};
GLfloat magenta[] = {1.0, 0.0, 1.0, 1.0};
GLfloat white[] = {1.0, 1.0, 1.0, 1.0};
GLfloat dim[] = {0.5, 0.5, 0.5, 1.0};

LightInfo linfo[] = {
  { {-4.0, 0.0, -10.0, 1.0}, yellow},
  { {4.0, 0.0, -10.0, 1.0}, green},
  { {-4.0, 0.0, -6.0, 1.0}, red},
  { {4.0, 0.0, -6.0, 1.0}, blue},
  { {-4.0, 0.0, -2.0, 1.0}, green},
  { {4.0, 0.0, -2.0, 1.0}, yellow},
  { {-4.0, 0.0, 2.0, 1.0}, blue},
  { {4.0, 0.0, 2.0, 1.0}, red},
  { {-4.0, 0.0, 6.0, 1.0}, yellow},
  { {4.0, 0.0, 6.0, 1.0}, green},
  { {-4.0, 0.0, 10.0, 1.0}, red},
  { {4.0, 0.0, 10.0, 1.0}, blue},
};

int lightState[8] = {1, 1, 1, 1, 1, 1, 1, 1};
/* *INDENT-ON* */

#define MAX_LIGHTS (sizeof(linfo)/sizeof(linfo[0]))

int moving = 0, begin;
GLfloat angle = 0.0;
int object = M_SPHERE;
int attenuation = M_QUAD;
GLfloat t = 0.0;

void
initLight(int num)
{
  glLightf(GL_LIGHT0 + num, GL_CONSTANT_ATTENUATION, 0.0);
  if (attenuation == M_LINEAR) {
    glLightf(GL_LIGHT0 + num, GL_LINEAR_ATTENUATION, 0.4);
    glLightf(GL_LIGHT0 + num, GL_QUADRATIC_ATTENUATION, 0.0);
  } else {
    glLightf(GL_LIGHT0 + num, GL_LINEAR_ATTENUATION, 0.0);
    glLightf(GL_LIGHT0 + num, GL_QUADRATIC_ATTENUATION, 0.1);
  }
  glLightfv(GL_LIGHT0 + num, GL_SPECULAR, dim);
}

/* Draw a sphere the same color as the light at the light position so it is
   easy to tell where the positional light sources are. */
void
drawLight(LightInfo * info)
{
  glPushMatrix();
  glTranslatef(info->xyz[0], info->xyz[1], info->xyz[2]);
  glColor3fv(info->rgb);
  glCallList(DL_LIGHT_SPHERE);
  glPopMatrix();
}

/* Place the light's OpenGL light number next to the light's sphere.  To
   ensure a readable number with good contrast, a black version of the number 
   is drawn shifted a pixel to the left and right of the actual white number. 
 */
void
labelLight(LightInfo * info, int num)
{
  GLubyte nothin = 0;
  void *font = GLUT_BITMAP_HELVETICA_18;
  int width = glutBitmapWidth(font, '0' + num);

  glPushMatrix();
  glColor3f(0.0, 0.0, 0.0);
  glRasterPos3f(info->xyz[0], info->xyz[1], info->xyz[2]);
  glBitmap(1, 1, 0, 0, 4, 5, &nothin);
  glutBitmapCharacter(font, '0' + num);

  glBitmap(1, 1, 0, 0, 2 - width, 0, &nothin);
  glutBitmapCharacter(font, '0' + num);

  if (lightState[num]) {
    glColor3fv(white);
  } else {
    /* Draw disabled lights dimmer. */
    glColor3fv(dim);
  }
  glRasterPos3f(info->xyz[0], info->xyz[1], info->xyz[2]);
  glBitmap(1, 1, 0, 0, 5, 5, &nothin);
  glutBitmapCharacter(font, '0' + num);
  glPopMatrix();
}

/* Comparison routine used by qsort. */
int
lightBrightnessCompare(const void *a, const void *b)
{
  LightBrightness *ld1 = (LightBrightness *) a;
  LightBrightness *ld2 = (LightBrightness *) b;
  GLfloat diff;

  /* The brighter lights get sorted close to top of the list. */
  diff = ld2->brightness - ld1->brightness;

  if (diff > 0)
    return 1;
  if (diff < 0)
    return -1;
  return 0;
}

void
display(void)
{
  int i;
  GLfloat x, y, z;
  LightBrightness ld[MAX_LIGHTS];
  int start, end;

  if (timeFrames) {
    start = glutGet(GLUT_ELAPSED_TIME);
  }
  x = cos(t * 12.3) * 2.0;
  y = 0.0;
  z = sin(t) * 7.0;

  for (i = 0; i < MAX_LIGHTS; i++) {
    GLfloat dx, dy, dz;
    GLfloat quadraticAttenuation;

    /* Calculate object to light position vector. */
    dx = (linfo[i].xyz[0] - x);
    dy = (linfo[i].xyz[1] - y);
    dz = (linfo[i].xyz[2] - z);

    quadraticAttenuation = dx * dx + dy * dy + dz * dz;

    if (brightnessModel == M_LAMBERTIAN) {
      /* Lambertian surface-based brightness determination. */
      GLfloat ex, ey, ez;
      GLfloat nx, ny, nz;
      GLfloat distance;
      GLfloat diffuseReflection;

      /* Determine eye point location (remember we can rotate by angle). */
      ex = 16.0 * sin(angle * M_PI / 180.0);
      ey = 1.0;
      ez = 16.0 * -cos(angle * M_PI / 180.0);

      /* Calculated normalized object to eye position direction (nx,ny,nz). */
      nx = (ex - x);
      ny = (ey - y);
      nz = (ez - z);
      distance = sqrt(nx * nx + ny * ny + nz * nz);
      nx = nx / distance;
      ny = ny / distance;
      nz = nz / distance;

      /* True distance needed, take square root. */
      distance = sqrt(quadraticAttenuation);

      /* Calculate normalized object to light postition direction (dx,dy,dz). 
       */
      dx = dx / distance;
      dy = dy / distance;
      dz = dz / distance;

      /* Dot product of object->eye and object->light source directions.
         OpenGL's lighting equations actually force the diffuse contribution
         to be zero if the dot product is less than zero.  For our purposes,
         that's too strict since we are approximating the entire object with
         a single object-to-eye normal. */
      diffuseReflection = nx * dx + ny * dy + nz * dz;
      if (attenuation == M_QUAD) {
        /* Attenuate based on square of distance. */
        ld[i].brightness = diffuseReflection / quadraticAttenuation;
      } else {
        /* Attenuate based on linear distance. */
        ld[i].brightness = diffuseReflection / distance;
      }
    } else {
      /* Distance-based brightness determination. */

      /* In theory, we are really determining brightness based on just the
         linear distance of the light source, but since we are just doing
         comparisons, there is no reason to waste time doing a square root. */

      /* Negation makes sure closer distances are "bigger" than further
         distances for sorting. */
      ld[i].brightness = -quadraticAttenuation;
    }
    ld[i].num = i;
  }

  /* Sort the lights so that the "brightest" are listed first.  We really
     want to just determine the first numActiveLights so a full sort is
     overkill. */
  qsort(ld, MAX_LIGHTS, sizeof(ld[0]), lightBrightnessCompare);

  if (reportLightSignificance) {
    printf("\n");
    for (i = 0; i < MAX_LIGHTS; i++) {
      printf("%d: dist = %g\n", ld[i].num, ld[i].brightness);
    }
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();
  glRotatef(angle, 0.0, 1.0, 0.0);

  glDisable(GL_LIGHTING);
  for (i = 0; i < MAX_LIGHTS; i++) {
    drawLight(&linfo[i]);
  }

  /* After sorting, the first numActiveLights (ie, <8) light sources are the
     light sources with the biggest contribution to the object's lighting.
     Assign these "virtual lights of significance" to OpenGL's actual
     available light sources. */

  glEnable(GL_LIGHTING);
  for (i = 0; i < numActiveLights; i++) {
    if (lightState[i]) {
      int num = ld[i].num;

      glLightfv(GL_LIGHT0 + i, GL_POSITION, linfo[num].xyz);
      glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, linfo[num].rgb);
      glEnable(GL_LIGHT0 + i);
    } else {
      glDisable(GL_LIGHT0 + i);
    }
  }

  glPushMatrix();
  glTranslatef(x, y, z);
  switch (object) {
  case M_SPHERE:
    glCallList(DL_BIG_SPHERE);
    break;
  case M_ICO:
    glCallList(DL_ICO);
    break;
  }
  glPopMatrix();

  if (labelLights) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    for (i = 0; i < numActiveLights; i++) {
      labelLight(&linfo[ld[i].num], i);
    }
    glEnable(GL_DEPTH_TEST);
  }
  glPopMatrix();

  if (timeFrames) {
    glFinish();
    end = glutGet(GLUT_ELAPSED_TIME);
    printf("Speed %.3g frames/sec (%d ms)\n",
      1000.0 / (end - start), end - start);
  }
  if (!singleBuffer) {
    glutSwapBuffers();
  }
}

void
idle(void)
{
  t += 0.005;
  glutPostRedisplay();
}

/* When not visible, stop animating.  Restart when visible again. */
static void
visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    if (animation)
      glutIdleFunc(idle);
  } else {
    if (!animation)
      glutIdleFunc(NULL);
  }
}

/* Press any key to redraw; good when motion stopped and performance
   reporting on. */
/* ARGSUSED */
static void
key(unsigned char c, int x, int y)
{
  int i;

  switch (c) {
  case 27:
    exit(0);            /* IRIS GLism, Escape quits. */
    break;
  case ' ':
    animation = 1 - animation;
    if (animation)
      glutIdleFunc(idle);
    else
      glutIdleFunc(NULL);
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
    lightState[c - '0'] = 1 - lightState[c - '0'];
    break;
  case 13:
    for (i = 0; i < numActiveLights; i++) {
      lightState[i] = 1;
    }
    break;
  }
  glutPostRedisplay();
}

/* ARGSUSED3 */
void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    moving = 1;
    begin = x;
  }
  if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
    moving = 0;
  }
}

/* ARGSUSED1 */
void
motion(int x, int y)
{
  if (moving) {
    angle = angle + (x - begin);
    begin = x;
    glutPostRedisplay();
  }
}

void
menu(int value)
{
  int i;

  switch (value) {
  case M_SPHERE:
    object = M_SPHERE;
    break;
  case M_ICO:
    object = M_ICO;
    break;
  case M_LABELS:
    labelLights = 1 - labelLights;
    break;
  case M_LINEAR:
  case M_QUAD:
    attenuation = value;
    for (i = 0; i < numActiveLights; i++) {
      initLight(i);
    }
    break;
  case M_REPORT_SIG:
    reportLightSignificance = 1 - reportLightSignificance;
    break;
  case M_LAMBERTIAN:
    brightnessModel = M_LAMBERTIAN;
    glutSetWindowTitle("multilight (Lambertian-based)");
    break;
  case M_DISTANCE:
    brightnessModel = M_DISTANCE;
    glutSetWindowTitle("multilight (Distance-based)");
    break;
  case M_TIME:
    timeFrames = 1 - timeFrames;
    break;
  case 666:
    exit(0);
  }
  glutPostRedisplay();
}

int
main(int argc, char **argv)
{
  int i;

  glutInitWindowSize(400, 200);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  glutInit(&argc, argv);

  for (i = 1; i < argc; i++) {
    if (!strcmp("-sb", argv[i])) {
      glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
      singleBuffer = 1;
    }
  }

  glutCreateWindow("multilight");

  glClearColor(0.0, 0.0, 0.0, 0.0);

  glMatrixMode(GL_PROJECTION);
  gluPerspective(50.0, 2.0, 0.1, 100.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(
    0.0, 1.0, -16.0,
    0.0, 0.0, 0.0,
    0.0, 1.0, 0.);

  numActiveLights = MIN_VALUE(MAX_LIGHTS, 8);
  for (i = 0; i < numActiveLights; i++) {
    initLight(i);
  }

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, modelAmb);
  glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
  glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  glMaterialfv(GL_FRONT, GL_AMBIENT, matAmb);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiff);
  glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
  glMaterialfv(GL_FRONT, GL_EMISSION, matEmission);
  glMaterialf(GL_FRONT, GL_SHININESS, 10.0);

  glNewList(DL_LIGHT_SPHERE, GL_COMPILE);
  glutSolidSphere(0.2, 4, 4);
  glEndList();

  glNewList(DL_BIG_SPHERE, GL_COMPILE);
  glutSolidSphere(1.5, 20, 20);
  glEndList();

  glNewList(DL_ICO, GL_COMPILE);
  glutSolidIcosahedron();
  glEndList();

  glutDisplayFunc(display);
  glutVisibilityFunc(visible);
  glutKeyboardFunc(key);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  glutCreateMenu(menu);
  glutAddMenuEntry("Sphere", M_SPHERE);
  glutAddMenuEntry("Icosahedron", M_ICO);
  glutAddMenuEntry("Linear attenuation", M_LINEAR);
  glutAddMenuEntry("Quadratic attenuation", M_QUAD);
  glutAddMenuEntry("Toggle Light Number Labels", M_LABELS);
  glutAddMenuEntry("Report Light Significance", M_REPORT_SIG);
  glutAddMenuEntry("Lambertian-based Significance", M_LAMBERTIAN);
  glutAddMenuEntry("Distance-based Significance", M_DISTANCE);
  glutAddMenuEntry("Time Frames", M_TIME);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
