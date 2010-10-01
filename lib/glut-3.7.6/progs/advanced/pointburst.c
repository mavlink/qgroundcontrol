
/* Copyright (c) Mark J. Kilgard, 1997, 1998.  */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This example demonstrates how to render particle effects
   with OpenGL.  A cloud of pinkish/orange particles explodes with the
   particles bouncing off the ground.  When the SGIS_point_parameters
   is present (supported on SGI's InfiniteReality hardware), the
   particle size is attenuated based on eye distance. */

/* Now pointburst.c is extended to support the multi-vendor
   EXT_point_parameters extension that has the same interface as the
   SGIS extension (modulo the SGIS suffix/prefix).  NVidia's Release 2
   OpenGL ICD driver supports the EXT_point_parameters extension. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#ifdef _WIN32
#include <windows.h>    /* for wglGetProcAddress */
#endif
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if 0  /* For debugging. */
#undef GL_SGIS_point_parameters
#endif

#if defined(GL_SGIS_point_parameters) && !defined(GL_EXT_point_parameters)
/* Use the EXT point parameters interface for the SGIS implementation. */
#define GL_POINT_SIZE_MIN_EXT GL_POINT_SIZE_MIN_SGIS
#define GL_POINT_SIZE_MAX_EXT GL_POINT_SIZE_MAX_SGIS
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT GL_POINT_FADE_THRESHOLD_SIZE_SGIS
#define GL_DISTANCE_ATTENUATION_EXT GL_DISTANCE_ATTENUATION_SGIS
#define glPointParameterfEXT glPointParameterfSGIS
#define glPointParameterfvEXT glPointParameterfvSGIS
#define GL_EXT_point_parameters 1
#endif

#if !defined(GL_EXT_point_parameters)
#define GL_POINT_SIZE_MIN_EXT               0x8126
#define GL_POINT_SIZE_MAX_EXT               0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT    0x8128
#define GL_DISTANCE_ATTENUATION_EXT         0x8129
#if defined(_WIN32) && !defined(MESA)
/* Curse Microsoft for the insanity of wglGetProcAddress. */
typedef void (APIENTRY * PFNGLPOINTPARAMETERFEXTPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLPOINTPARAMETERFVEXTPROC) (GLenum pname, const GLfloat *params);
#define GL_EXT_point_parameters 1
#endif
#endif

#if defined(_WIN32) && !defined(MESA)
PFNGLPOINTPARAMETERFEXTPROC glPointParameterfEXT;
PFNGLPOINTPARAMETERFVEXTPROC glPointParameterfvEXT;
#endif

int hasPointParameters;

static GLfloat angle = -150;   /* in degrees */
static int spin = 0;
static int moving, begin;
static int newModel = 1;
static float time;
static int repeat = 1;
int useMipmaps = 1;
int linearFiltering = 1;
int useTexture = 1;

#if GL_EXT_point_parameters
static GLfloat constant[3] = { 1/5.0, 0.0, 0.0 };
static GLfloat linear[3] = { 0.0, 1/5.0, 0.0 };
static GLfloat quadratic[3] = { 0.25, 0.0, 1/60.0 };
#endif

#define MAX_POINTS 2000

static int numPoints = 500;

static GLfloat pointList[MAX_POINTS][3];
static GLfloat pointTime[MAX_POINTS];
static GLfloat pointVelocity[MAX_POINTS][2];
static GLfloat pointDirection[MAX_POINTS][2];
static int colorList[MAX_POINTS];
static int animate = 1, motion = 0;

static GLfloat colorSet[][4] = {
  /* Shades of red. */
  { 0.7, 0.2, 0.4, 0.5 },
  { 0.8, 0.0, 0.7, 0.5 },
  { 1.0, 0.0, 0.0, 0.5 },
  { 0.9, 0.3, 0.6, 0.5 },
  { 1.0, 0.4, 0.0, 0.5 },
  { 1.0, 0.0, 0.5, 0.5 },
};

#define NUM_COLORS (sizeof(colorSet)/sizeof(colorSet[0]))

#define DEAD (NUM_COLORS+1)


#if 0  /* drand48 might be better on Unix machines */
#define RANDOM_RANGE(lo, hi) ((lo) + (hi - lo) * drand48())
#else
static float float_rand(void) { return rand() / (float) RAND_MAX; }
#define RANDOM_RANGE(lo, hi) ((lo) + (hi - lo) * float_rand())
#endif

#define MEAN_VELOCITY 3.0
#define GRAVITY 2.0
#define TIME_DELTA 0.025  /* The speed of time. */

/* Modeling units of ground extent in each X and Z direction. */
#define EDGE 12

void
makePointList(void)
{
  float angle, velocity, direction;
  int i;

  motion = 1;
  for (i=0; i<numPoints; i++) {
    pointList[i][0] = 0.0;
    pointList[i][1] = 0.0;
    pointList[i][2] = 0.0;
    pointTime[i] = 0.0;
    angle = (RANDOM_RANGE(60.0, 70.0)) * M_PI/180.0;
    direction = RANDOM_RANGE(0.0, 360.0) * M_PI/180.0;
    pointDirection[i][0] = cos(direction);
    pointDirection[i][1] = sin(direction);
    velocity = MEAN_VELOCITY + RANDOM_RANGE(-0.8, 1.0);
    pointVelocity[i][0] = velocity * cos(angle);
    pointVelocity[i][1] = velocity * sin(angle);
    colorList[i] = rand() % NUM_COLORS;
  }
  time = 0.0;
}

void
updatePointList(void)
{
  float distance;
  int i;

  motion = 0;
  for (i=0; i<numPoints; i++) {
    distance = pointVelocity[i][0] * time;

    /* X and Z */
    pointList[i][0] = pointDirection[i][0] * distance;
    pointList[i][2] = pointDirection[i][1] * distance;

    /* Z */
    pointList[i][1] =
      (pointVelocity[i][1] - 0.5 * GRAVITY * pointTime[i])*pointTime[i];

    /* If we hit the ground, bounce the point upward again. */
    if (pointList[i][1] <= 0.0) {
      if (distance > EDGE) {
        /* Particle has hit ground past the distance duration of
	   the particles.  Mark particle as dead. */
	colorList[i] = NUM_COLORS;  /* Not moving. */
	continue;
      }

      pointVelocity[i][1] *= 0.8;  /* 80% of previous up velocity. */
      pointTime[i] = 0.0;  /* Reset the particles sense of up time. */
    }
    motion = 1;
    pointTime[i] += TIME_DELTA;
  }
  time += TIME_DELTA;
  if (!motion && !spin) {
    if (repeat) {
      makePointList();
    } else {
      glutIdleFunc(NULL);
    }
  }
}

void
idle(void)
{
  updatePointList();
  if (spin) {
    angle += 0.3;
    newModel = 1;
  }
  glutPostRedisplay();
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    if (animate && (motion || spin)) {
      glutIdleFunc(idle);
    }
  } else {
    glutIdleFunc(NULL);
  }
}

void
recalcModelView(void)
{
  glPopMatrix();
  glPushMatrix();
  glRotatef(angle, 0.0, 1.0, 0.0);
  newModel = 0;
}

void
redraw(void)
{
  int i;

  glDepthMask(GL_TRUE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (newModel)
    recalcModelView();

  /* Draw the floor. */
  if (useTexture) {
    glEnable(GL_TEXTURE_2D);
  }
  glColor3f(0.5, 1.0, 0.5);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-EDGE, -0.05, -EDGE);
    glTexCoord2f(20.0, 0.0);
    glVertex3f(EDGE, -0.05, -EDGE);
    glTexCoord2f(20.0, 20.0);
    glVertex3f(EDGE, -0.05, EDGE);
    glTexCoord2f(0.0, 20.0);
    glVertex3f(-EDGE, -0.05, EDGE);
  glEnd();

  /* Allow particles to blend with each other. */
  glDepthMask(GL_FALSE);

  if (useTexture) {
    glDisable(GL_TEXTURE_2D);
  }
  glBegin(GL_POINTS);
    for (i=0; i<numPoints; i++) {
      /* Draw alive particles. */
      if (colorList[i] != DEAD) {
        glColor4fv(colorSet[colorList[i]]);
        glVertex3fv(pointList[i]);
      }
    }
  glEnd();

  glutSwapBuffers();
}

/* ARGSUSED2 */
void
mouse(int button, int state, int x, int y)
{
  /* Scene can be spun around Y axis using left
     mouse button movement. */
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
mouseMotion(int x, int y)
{
  if (moving) {
    angle = angle + (x - begin);
    begin = x;
    newModel = 1;
    glutPostRedisplay();
  }
}

void
menu(int option)
{
  switch (option) {
  case 0:
    makePointList();
    break;
#if GL_EXT_point_parameters
  case 1:
    if (hasPointParameters) {
      glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, constant);
    }
    break;
  case 2:
    if (hasPointParameters) {
      glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, linear);
    }
    break;
  case 3:
    if (hasPointParameters) {
      glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, quadratic);
    }
    break;
#endif
  case 4:
    glEnable(GL_BLEND);
    break;
  case 5:
    glDisable(GL_BLEND);
    break;
#if GL_EXT_point_parameters
  case 6:
    if (hasPointParameters) {
      glPointParameterfEXT(GL_POINT_FADE_THRESHOLD_SIZE_EXT, 1.0);
    }
    break;
  case 7:
    if (hasPointParameters) {
      glPointParameterfEXT(GL_POINT_FADE_THRESHOLD_SIZE_EXT, 10.0);
    }
    break;
#endif
  case 8:
    glEnable(GL_POINT_SMOOTH);
    break;
  case 9:
    glDisable(GL_POINT_SMOOTH);
    break;
  case 10:
    glPointSize(2.0);
    break;
  case 11:
    glPointSize(4.0);
    break;
  case 12:
    glPointSize(8.0);
    break;
  case 18:
    glPointSize(16.0);
    break;
  case 13:
    spin = 1 - spin;
    if (animate && (spin || motion)) {
      glutIdleFunc(idle);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  case 14:
    numPoints = 200;
    break;
  case 15:
    numPoints = 500;
    break;
  case 16:
    numPoints = 1000;
    break;
  case 17:
    numPoints = 2000;
    break;
  case 19:
    useTexture = !useTexture;
    break;
  case 666:
    exit(0);
  }
  glutPostRedisplay();
}

/* ARGSUSED1 */
void
key(unsigned char c, int x, int y)
{
  switch (c) {
  case 13:
    animate = 1 - animate;  /* toggle. */
    if (animate && (motion || spin)) {
      glutIdleFunc(idle);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  case ' ':
    animate = 1;
    makePointList();
    glutIdleFunc(idle);
    break;
  case 27:
    exit(0);
  }
}

/* Nice floor texture tiling pattern. */
static char *circles[] = {
  "....xxxx........",
  "..xxxxxxxx......",
  ".xxxxxxxxxx.....",
  ".xxx....xxx.....",
  "xxx......xxx....",
  "xxx......xxx....",
  "xxx......xxx....",
  "xxx......xxx....",
  ".xxx....xxx.....",
  ".xxxxxxxxxx.....",
  "..xxxxxxxx......",
  "....xxxx........",
  "................",
  "................",
  "................",
  "................",
};

static void
makeFloorTexture(void)
{
  GLubyte floorTexture[16][16][3];
  GLubyte *loc;
  int s, t;

  /* Setup RGB image for the texture. */
  loc = (GLubyte*) floorTexture;
  for (t = 0; t < 16; t++) {
    for (s = 0; s < 16; s++) {
      if (circles[t][s] == 'x') {
        /* Nice blue. */
        loc[0] = 0x1f;
        loc[1] = 0x1f;
        loc[2] = 0x8f;
      } else {
        /* Light gray. */
        loc[0] = 0xca;
        loc[1] = 0xca;
        loc[2] = 0xca;
      }
      loc += 3;
    }
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (useMipmaps) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
      GL_LINEAR_MIPMAP_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 16, 16,
      GL_RGB, GL_UNSIGNED_BYTE, floorTexture);
  } else {
    if (linearFiltering) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 16, 16, 0,
      GL_RGB, GL_UNSIGNED_BYTE, floorTexture);
  }
}

int
main(int argc, char **argv)
{
  int i;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);

  for (i=1; i<argc; i++) {
    if(!strcmp("-noms", argv[i])) {
      glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
      printf("forcing no multisampling\n");
    } else if(!strcmp("-nomipmaps", argv[i])) {
      useMipmaps = 0;
    } else if(!strcmp("-nearest", argv[i])) {
      linearFiltering = 0;
    }
  }

  glutCreateWindow("point burst");

  hasPointParameters = glutExtensionSupported("GL_SGIS_point_parameters") ||
    glutExtensionSupported("GL_EXT_point_parameters");
  if (hasPointParameters) {
#if defined(_WIN32) && !defined(MESA)
    glPointParameterfEXT = (PFNGLPOINTPARAMETERFEXTPROC)
      wglGetProcAddress("glPointParameterfEXT");
    glPointParameterfvEXT = (PFNGLPOINTPARAMETERFVEXTPROC)
      wglGetProcAddress("glPointParameterfvEXT");
    if (!glPointParameterfEXT || !glPointParameterfvEXT) {
      hasPointParameters = 0;
    }
#endif
  }
  if (hasPointParameters) {
    printf("has point parameters extension!\n");
  } else {
    printf("does NOT have point parameters extension!\n");
  }

  glutDisplayFunc(redraw);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotion);
  glutVisibilityFunc(visible);
  glutKeyboardFunc(key);
  glutCreateMenu(menu);
  glutAddMenuEntry("Reset time", 0);
  glutAddMenuEntry("Constant", 1);
  glutAddMenuEntry("Linear", 2);
  glutAddMenuEntry("Quadratic", 3);
  glutAddMenuEntry("Blend on", 4);
  glutAddMenuEntry("Blend off", 5);
  glutAddMenuEntry("Threshold 1", 6);
  glutAddMenuEntry("Threshold 10", 7);
  glutAddMenuEntry("Point smooth on", 8);
  glutAddMenuEntry("Point smooth off", 9);
  glutAddMenuEntry("Point size 2", 10);
  glutAddMenuEntry("Point size 4", 11);
  glutAddMenuEntry("Point size 8", 12);
  glutAddMenuEntry("Point size 16", 18);
  glutAddMenuEntry("Toggle spin", 13);
  glutAddMenuEntry("200 points", 14);
  glutAddMenuEntry("500 points", 15);
  glutAddMenuEntry("1000 points", 16);
  glutAddMenuEntry("2000 points", 17);
  glutAddMenuEntry("Toggle texture", 19);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(8.0);
#if GL_EXT_point_parameters
  if (hasPointParameters) {
    glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, quadratic);
  }
#endif

  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 0.5, /* Z far */ 40.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 1.0, 8.0, /* eye location */
    0.0, 1.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in postivie Y direction */
  glPushMatrix();       /* dummy push so we can pop on model
                           recalc */

  makePointList();
  makeFloorTexture();

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
