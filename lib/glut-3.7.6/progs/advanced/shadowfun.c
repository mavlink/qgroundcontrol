
/* Copyright (c) Mark J. Kilgard, 1997.  */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* Unix compile line:  cc -o shadowfun shadowfun.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

/* THIS PROGRAM REQUIRES GLU 1.2. If you have IRIX 5.3, you need patch 1449
   (the IRIX 5.3 GLU 1.2 functionality patch) or its successor to use this
   program.  GLU 1.2 is standard on IRIX 6.2 and later. */

/* This program demonstrates a light source and object of arbitrary geometry
   casing a shadow on arbitary geometry.  The program uses OpenGL's feedback, 
   stencil, and boundary tessellation support. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#ifdef GLU_VERSION_1_2

/* Win32 calling conventions. */
#ifndef CALLBACK
#define CALLBACK
#endif

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float uniquePassThroughValue = 34567.0;

#define SmallerOf(a,b) ((a) < (b) ? (a) : (b))

int stencilBits;

/* Display list names. */
enum {
  /* Display lists should start at 1, not 0. */
  DL_BALL = 1, DL_CONE, DL_LIGHT, DL_SHADOW_VOLUME, DL_SPHERE,
  DL_ICO, DL_TORUS, DL_CUBE, DL_SHADOW_VOLUME_TOP, DL_BASE_SHADOW_VOLUME
};

/* Menu option names. */
enum {
  /* Important for objectMaxRadius array that the shape enums appear first in
     this list. */
  M_TORUS, M_CUBE, M_SPHERE, M_ICO, M_DOUBLE_TORUS, M_ANGLE, M_BOUNDARY,
  M_NO_SHADOW, M_NO_LIGHT, M_FRONT_VOLUME, M_BACK_VOLUME, M_SHADOW,
  M_LIGHT_SOURCE_VIEW, M_NORMAL_VIEW, M_SPIN, M_SWING, M_STOP
};

/* Coordinates. */
enum {
  X, Y, Z
};

const int TEXDIM = 64;

int shape;
GLfloat maxRadius;
int renderMode = M_SHADOW;
int view = M_NORMAL_VIEW;
int renderBoundary = 0;
GLfloat angle = 0.0;
int frontFace = 1;
int rotatingObject = 1;
int swingingLight = 1;
float swingTime = M_PI / 2.0;

GLfloat lightDiffuse[4] =
{1.0, 0.0, 0.0, 1.0};
GLfloat lightPos[4] =
{60.0, 50.0, -350.0, 1.0};
GLfloat objectPos[4] =
{40.0, 30.0, -360.0, 1.0};
GLfloat sceneEyePos[4] =
{0.0, 0.0, 0.0, 0.0};

struct VertexHolder {
  struct VertexHolder *next;
  GLfloat v[2];
};

typedef struct _ShadowVolumeMemoryPool {
  /* Reference count because ShadowVolumeMemoryPool's can be shared between
     multiple ShadowVolumeState's. */
  int refcnt;

  GLUtesselator *tess;
  GLfloat viewScale;

  /* Memory used for GLU tessellator combine callbacks. */
  GLfloat *combineList;
  int combineListSize;
  int combineNext;
  struct VertexHolder *excessList;
} ShadowVolumeMemoryPool;

typedef struct _ShadowVolumeState {
  ShadowVolumeMemoryPool *pool;

  GLfloat shadowProjectionDistance;
  GLfloat extentScale;

  /* Scratch variables used during GLU tessellator callbacks. */
  int saveFirst;
  GLfloat *firstVertex;
} ShadowVolumeState;

ShadowVolumeState *svs;

static void CALLBACK
begin(GLenum type, void *shadowVolumeState)
{
  ShadowVolumeState *svs = (ShadowVolumeState *) shadowVolumeState;

  assert(type == GL_LINE_LOOP);
  if (renderBoundary) {
    glBegin(type);
  } else {
    svs->saveFirst = 1;
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0, 1, 0);
    glVertex3f(0.0, 0.0, 0.0);
  }
}

static void CALLBACK
vertex(void *data, void *shadowVolumeState)
{
  ShadowVolumeState *svs = (ShadowVolumeState *) shadowVolumeState;
  GLfloat *v = data;

  if (renderBoundary) {
    glVertex2fv(v);
  } else {
    if (svs->saveFirst) {
      svs->firstVertex = v;
      svs->saveFirst = 0;
    }
    glColor3f(0, 0, 1);
    glVertex3f(svs->extentScale * v[X], svs->extentScale * v[Y],
      svs->shadowProjectionDistance);
  }
}

static void CALLBACK
end(void *shadowVolumeState)
{
  ShadowVolumeState *svs = (ShadowVolumeState *) shadowVolumeState;

  if (!renderBoundary) {
    glColor3f(0, 0, 1);
    glVertex3f(svs->extentScale * svs->firstVertex[X], svs->extentScale * svs->firstVertex[Y],
      svs->shadowProjectionDistance);
  }
  glEnd();
}

static void
freeExcessList(ShadowVolumeMemoryPool * pool)
{
  struct VertexHolder *holder, *next;

  holder = pool->excessList;
  while (holder) {
    next = holder->next;
    free(holder);
    holder = next;
  }
  pool->excessList = NULL;
}

/* ARGSUSED1 */
static void CALLBACK
combine(GLdouble coords[3], void *d[4], GLfloat w[4], void **dataOut, void *shadowVolumeState)
{
  ShadowVolumeState *svs = (ShadowVolumeState *) shadowVolumeState;
  ShadowVolumeMemoryPool *pool = svs->pool;
  struct VertexHolder *holder;
  GLfloat *newCoords;

  if (pool->combineNext >= pool->combineListSize) {
    holder = (struct VertexHolder *) malloc(sizeof(struct VertexHolder));
    holder->next = pool->excessList;
    pool->excessList = holder;
    newCoords = holder->v;
  } else {
    newCoords = &pool->combineList[pool->combineNext * 2];
  }

  newCoords[0] = coords[0];
  newCoords[1] = coords[1];
  *dataOut = newCoords;

  pool->combineNext++;
}

static void CALLBACK
error(GLenum errno)
{
  printf("ERROR: %s\n", gluErrorString(errno));
}

static void
processFeedback(GLint size, GLfloat * buffer, ShadowVolumeState * svs)
{
  ShadowVolumeMemoryPool *pool = svs->pool;
  GLfloat *loc, *end, *eyeLoc;
  GLdouble v[3];
  int token, nvertices, i;
  GLfloat passThroughToken;
  int watchingForEyePos;

  if (pool->combineNext > pool->combineListSize) {
    freeExcessList(pool);
    pool->combineListSize = pool->combineNext;
    pool->combineList = realloc(pool->combineList, sizeof(GLfloat) * 2 * pool->combineListSize);
  }
  pool->combineNext = 0;

  watchingForEyePos = 0;
  eyeLoc = NULL;

  glColor3f(1, 1, 1);
  gluTessBeginPolygon(pool->tess, svs);
  loc = buffer;
  end = buffer + size;
  while (loc < end) {
    token = *loc;
    loc++;
    switch (token) {
    case GL_POLYGON_TOKEN:
      nvertices = *loc;
      loc++;
      assert(nvertices >= 3);
      gluTessBeginContour(pool->tess);
      for (i = 0; i < nvertices; i++) {
        v[0] = loc[0];
        v[1] = loc[1];
        v[2] = 0.0;
        gluTessVertex(pool->tess, v, loc);
        loc += 2;
      }
      gluTessEndContour(pool->tess);
      break;
    case GL_PASS_THROUGH_TOKEN:
      passThroughToken = *loc;
      if (passThroughToken == uniquePassThroughValue) {
        watchingForEyePos = !watchingForEyePos;
      } else {
        /* Ignore everything else. */
        fprintf(stderr, "ERROR: Unexpected feedback token 0x%x (%d).\n", token, token);
      }
      loc++;
      break;
    case GL_POINT_TOKEN:
      if (watchingForEyePos) {
        fprintf(stderr, "WARNING: Eye point possibly within the shadow volume.\n");
        fprintf(stderr, "         Program should be improved to handle this.\n");
        /* XXX Write code to handle this case.  You would need to determine
           if the point was instead any of the returned boundary polyons.
           Once you found that you were really in the clipping volume, then I
           haven't quite thought about what you do. */
        eyeLoc = loc;
        watchingForEyePos = 0;
      } else {
        /* Ignore everything else. */
        fprintf(stderr, "ERROR: Unexpected feedback token 0x%x (%d).\n",
          token, token);
      }
      loc += 2;
      break;
    default:
      /* Ignore everything else. */
      fprintf(stderr, "ERROR: Unexpected feedback token 0x%x (%d).\n",
        token, token);
    }
  }
  gluTessEndPolygon(pool->tess);

  if (eyeLoc && renderBoundary) {
    glColor3f(0, 1, 0);
    glPointSize(7.0);
    glBegin(GL_POINTS);
    glVertex2fv(eyeLoc);
    glEnd();
  }
}

/* Three element vector dot product. */
static GLfloat
vdot(const GLfloat * v1, const GLfloat * v2)
{
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

/* Three element vector cross product. */
static void
vcross(const GLfloat * v1, const GLfloat * v2, GLfloat * cross)
{
  assert(v1 != cross && v2 != cross);
  cross[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
  cross[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
  cross[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
}

void
svsFreeShadowVolumeState(ShadowVolumeState * svs)
{
  if (svs->pool) {
    svs->pool->refcnt--;
    if (svs->pool->refcnt == 0) {
      if (svs->pool->excessList) {
        freeExcessList(svs->pool);
      }
      if (svs->pool->combineList) {
        free(svs->pool->combineList);
      }
      if (svs->pool->tess) {
        gluDeleteTess(svs->pool->tess);
      }
      free(svs->pool);
    }
  }
  free(svs);
}

ShadowVolumeState *
svsCreateShadowVolumeState(GLfloat shadowProjectionDistance,
  ShadowVolumeState * shareSVS)
{
  ShadowVolumeState *svs;
  ShadowVolumeMemoryPool *pool;
  GLUtesselator *tess;

  svs = (ShadowVolumeState *) malloc(sizeof(ShadowVolumeState));
  if (svs == NULL) {
    return NULL;
  }
  svs->pool = NULL;

  if (shareSVS == NULL) {
    pool = (ShadowVolumeMemoryPool *) malloc(sizeof(ShadowVolumeMemoryPool));
    if (pool == NULL) {
      svsFreeShadowVolumeState(svs);
      return NULL;
    }
    pool->refcnt = 1;
    pool->excessList = NULL;
    pool->combineList = NULL;
    pool->combineListSize = 0;
    pool->combineNext = 0;
    pool->tess = NULL;
    svs->pool = pool;

    tess = gluNewTess();
    if (tess == NULL) {
      svsFreeShadowVolumeState(svs);
      return NULL;
    }
    gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_TRUE);
    gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
    gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (void (CALLBACK*)()) begin);
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (void (CALLBACK*)()) vertex);
    gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (void (CALLBACK*)()) combine);
    gluTessCallback(tess, GLU_TESS_END_DATA, (void (CALLBACK*)()) end);
    gluTessCallback(tess, GLU_TESS_ERROR, (void (CALLBACK*)()) error);
    pool->tess = tess;
  } else {
    pool = shareSVS->pool;
    pool->refcnt++;
  }

  svs->pool = pool;
  svs->shadowProjectionDistance = shadowProjectionDistance;

  return svs;
}

int
svsGenerateShadowVolume(ShadowVolumeState * svs,
  void (*renderFunc) (void), int feedbackBufferSizeGuess,
  GLfloat maxRadius,
  GLfloat lightPos[3], GLfloat objectPos[3], GLfloat eyePos[3])
{
  static GLfloat unit[3] =
  {0.0, 0.0, 1.0};
  static GLfloat *feedbackBuffer = NULL;
  static int bufferSize = 0;
  GLfloat axis[3], lightDelta[3], eyeDelta[3];
  GLfloat nnear, ffar;  /* Avoid Intel C keywords.  Grumble. */
  GLfloat lightDistance, eyeDistance, angle, fieldOfViewRatio, fieldOfViewAngle,
    topScale, viewScale;
  GLint returned;

  if (svs->pool->viewScale == 0.0) {
    GLfloat maxViewSize[2];

    glGetFloatv(GL_MAX_VIEWPORT_DIMS, maxViewSize);
    printf("max viewport = %gx%g\n", maxViewSize[0], maxViewSize[1]);
    svs->pool->viewScale = SmallerOf(maxViewSize[0], maxViewSize[1]) / 2.0;
  }
  viewScale = svs->pool->viewScale;

  if (bufferSize > feedbackBufferSizeGuess) {
    feedbackBufferSizeGuess = bufferSize;
  }
  /* Calculate the light's distance from the object being shadowed. */
  lightDelta[X] = objectPos[X] - lightPos[X];
  lightDelta[Y] = objectPos[Y] - lightPos[Y];
  lightDelta[Z] = objectPos[Z] - lightPos[Z];
  lightDistance = sqrt(lightDelta[X] * lightDelta[X] +
    lightDelta[Y] * lightDelta[Y] + lightDelta[Z] * lightDelta[Z]);

  /* Determine the appropriate field of view.  We want to use as narrow a
     field of view as possible to not waste resolution, but not narrower than
     the object.  Add 50% extra slop. */
  fieldOfViewRatio = maxRadius / lightDistance;
  if (fieldOfViewRatio > 0.99) {
    fprintf(stderr, "WARNING: Clamping FOV to 164 degrees for determining shadow boundary.\n");
    fprintf(stderr, "         Light distance = %g, object maxmium radius = %g\n",
      lightDistance, maxRadius);

    /* 2*asin(0.99) ~= 164 degrees. */
    fieldOfViewRatio = 0.99;
  }
  /* Pre-compute scaling factors for the near and far extent of the shadow
     volume. */
  svs->extentScale = svs->shadowProjectionDistance * fieldOfViewRatio / viewScale;

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  nnear = 0.5 * (lightDistance - maxRadius);
  if (nnear < 0.0001) {
    fprintf(stderr, "WARNING: Clamping near clip plane to 0.0001 because light source too near.\n");
    fprintf(stderr, "         Light distance = %g, object maxmium radius = %g\n",
      lightDistance, maxRadius);
    nnear = 0.0001;
  }
  ffar = 2.0 * (lightDistance + maxRadius);
  if (eyePos) {
    eyeDelta[X] = eyePos[X] - lightPos[X];
    eyeDelta[Y] = eyePos[Y] - lightPos[Y];
    eyeDelta[Z] = eyePos[Z] - lightPos[Z];
    eyeDistance = 1.05 * sqrt(eyeDelta[X] * eyeDelta[X] + eyeDelta[Y] * eyeDelta[Y] + eyeDelta[Z] * eyeDelta[Z]);
    if (eyeDistance > ffar) {
      ffar = eyeDistance;
    }
  }
  fieldOfViewAngle = 2.0 * asin(fieldOfViewRatio) * 180 / M_PI;
  gluPerspective(fieldOfViewAngle, 1.0, nnear, ffar);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  /* XXX Need to update "up vector".  Degenerate when light directly above or 
     below the object. */
  gluLookAt(lightPos[X], lightPos[Y], lightPos[Z],
    objectPos[X], objectPos[Y], objectPos[Z],
    0.0, 1.0, 0.0);     /* up is in positive Y direction */

  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(-viewScale, -viewScale, 2 * viewScale, 2 * viewScale);

doFeedback:

  /* XXX Careful, some systems still don't understand realloc of NULL. */
  if (bufferSize < feedbackBufferSizeGuess) {
    bufferSize = feedbackBufferSizeGuess;
    /* XXX Add 32 words of slop (an extra cache line) to end for buggy
       hardware that uses DMA to return feedback results but that sometimes
       overrun the buffer.  Yuck. */
    feedbackBuffer = realloc(feedbackBuffer, bufferSize * sizeof(GLfloat) + 32 * 4);
  }
  glFeedbackBuffer(bufferSize, GL_2D, feedbackBuffer);

  (void) glRenderMode(GL_FEEDBACK);

  (*renderFunc) ();

  /* Render the eye position.  The eye position is "bracketed" by unique pass 
     through tokens.  These bracketing pass through tokens let us determine
     if the eye position was clipped or not.  This helps us determine whether 
     the eye position is possibly within the shadow volume or not.  If the
     point is clipped, the eye position is not in the shadow volume.  If the
     point is not clipped, a more complicated test is necessary to determine
     if the eye position is really in the shadow volume or not.  See
     processFeedback. */
  if (eyePos) {
    glPassThrough(uniquePassThroughValue);
    glBegin(GL_POINTS);
    glVertex3fv(eyePos);
    glEnd();
    glPassThrough(uniquePassThroughValue);
  }
  returned = glRenderMode(GL_RENDER);
#if 0
  if (returned == -1) {
#else
  /* XXX RealityEngine workaround. */
  if (returned == -1 || returned == feedbackBufferSizeGuess) {
#endif
    feedbackBufferSizeGuess = feedbackBufferSizeGuess + (feedbackBufferSizeGuess >> 1);
    goto doFeedback;    /* Try again with larger feedback buffer. */
  }
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();        /* Restore viewport. */

  if (renderBoundary) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-viewScale, viewScale, -viewScale, viewScale);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    processFeedback(returned, feedbackBuffer, svs);
  } else {
    glNewList(DL_BASE_SHADOW_VOLUME, GL_COMPILE);
    vcross(unit, lightDelta, axis);
    angle = acos(vdot(unit, lightDelta) / lightDistance) * 180.0 / M_PI;
    glRotatef(angle, axis[X], axis[Y], axis[Z]);
    processFeedback(returned, feedbackBuffer, svs);
    glEndList();

    glNewList(DL_SHADOW_VOLUME, GL_COMPILE);
    glPushMatrix();
    glTranslatef(lightPos[X], lightPos[Y], lightPos[Z]);
    glCallList(DL_BASE_SHADOW_VOLUME);
    glPopMatrix();
    glEndList();

    glNewList(DL_SHADOW_VOLUME_TOP, GL_COMPILE);
    glPushMatrix();
    glTranslatef(lightPos[X], lightPos[Y], lightPos[Z]);
    topScale = (lightDistance + maxRadius) / svs->shadowProjectionDistance;
    glScalef(topScale, topScale, topScale);
    glCallList(DL_BASE_SHADOW_VOLUME);
    glPopMatrix();
    glEndList();
  }
  return returned;
}

GLfloat objectMaxRadius[] =
{
  8.0 + 2.0,            /* M_TORUS */
  12.0 / 2.0 * 1.142,   /* M_CUBE */
  8.0,                  /* M_SPHERE */
  8.0,                  /* M_ICO */
  8.0 + 2.0,            /* M_DOUBLE_TORUS */
};

void
renderShadowingObject(void)
{
  static int torusList = 0, cubeList = 0, sphereList = 0, icoList = 0;

  glPushMatrix();
  glTranslatef(objectPos[X], objectPos[Y], objectPos[Z]);
  glRotatef(angle, 1.0, 0.3, 0.0);
  switch (shape) {
  case M_TORUS:
    if (torusList) {
      glCallList(torusList);
    } else {
      torusList = DL_TORUS;
      glNewList(torusList, GL_COMPILE_AND_EXECUTE);
      glutSolidTorus(2.0, 8.0, 8, 15);
      glEndList();
    }
    break;
  case M_CUBE:
    if (cubeList) {
      glCallList(cubeList);
    } else {
      cubeList = DL_CUBE;
      glNewList(cubeList, GL_COMPILE_AND_EXECUTE);
      glutSolidCube(12.0);
      glEndList();
    }
    break;
  case M_SPHERE:
    if (sphereList) {
      glCallList(sphereList);
    } else {
      sphereList = DL_SPHERE;
      glNewList(sphereList, GL_COMPILE_AND_EXECUTE);
      glutSolidSphere(8.0, 10, 10);
      glEndList();
    }
    break;
  case M_ICO:
    if (icoList) {
      glCallList(icoList);
    } else {
      icoList = DL_ICO;
      glNewList(icoList, GL_COMPILE_AND_EXECUTE);
      glEnable(GL_NORMALIZE);
      glPushMatrix();
      glScalef(8.0, 8.0, 8.0);
      glutSolidIcosahedron();
      glPopMatrix();
      glDisable(GL_NORMALIZE);
      glEndList();
    }
    break;
  case M_DOUBLE_TORUS:
    if (torusList) {
      glCallList(torusList);
    } else {
      torusList = DL_TORUS;
      glNewList(torusList, GL_COMPILE_AND_EXECUTE);
      glutSolidTorus(2.0, 8.0, 8, 15);
      glEndList();
    }
    glRotatef(90, 0, 1, 0);
    glCallList(torusList);
    break;
  }
  glPopMatrix();
}

void
sphere(void)
{
  glPushMatrix();
  glTranslatef(60.0, -50.0, -400.0);
  glCallList(DL_BALL);
  glPopMatrix();
}

void
cone(void)
{
  glPushMatrix();
  glTranslatef(-40.0, -40.0, -400.0);
  glCallList(DL_CONE);
  glPopMatrix();
}

void
scene(void)
{
  /* material properties for objects in scene */
  static GLfloat wall_mat[] =
  {1.0, 1.0, 1.0, 1.0};
  static GLfloat shad_mat[] =
  {1.0, 0.1, 0.1, 1.0};

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (view == M_LIGHT_SOURCE_VIEW) {
    gluPerspective(45.0, 1.0, 0.5, 600.0);
  } else {
    gluPerspective(33.0, 1.0, 10.0, 600.0);
  }
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  if (view == M_LIGHT_SOURCE_VIEW) {
    gluLookAt(lightPos[X], lightPos[Y], lightPos[Z],
      objectPos[X], objectPos[Y], objectPos[Z],
      0.0, 1.0, 0.);    /* up is in positive Y direction */
  } else {
    gluLookAt(0.0, 0.0, 0.0,
      0.0, 0.0, -100.0,
      0.0, 1.0, 0.);    /* up is in positive Y direction */
  }
  /* Place light 0 in the right place. */
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

  /* Note: wall verticies are ordered so they are all front facing this lets
     me do back face culling to speed things up.  */

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

  /* Floor with checkerboard texture.  */
  glEnable(GL_TEXTURE_2D);

  glColor3f(0, 0, 0);

  /* Since we want to turn texturing on for floor only, we have to make floor
     a separate glBegin()/glEnd() sequence. You can't turn texturing on and
     off between begin and end calls */
  glBegin(GL_QUADS);
  glNormal3f(0.0, 1.0, 0.0);
  glTexCoord2i(0, 0);
  glVertex3f(-100.0, -100.0, -320.0);
  glTexCoord2i(4, 0);
  glVertex3f(100.0, -100.0, -320.0);
  glTexCoord2i(4, 4);
  glVertex3f(100.0, -100.0, -520.0);
  glTexCoord2i(0, 4);
  glVertex3f(-100.0, -100.0, -520.0);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  /* Walls. */

  glBegin(GL_QUADS);
  /* Left wall. */
  glNormal3f(1.0, 0.0, 0.0);
  glVertex3f(-100.0, -100.0, -320.0);
  glVertex3f(-100.0, -100.0, -520.0);
  glVertex3f(-100.0, 100.0, -520.0);
  glVertex3f(-100.0, 100.0, -320.0);

  /* Right wall. */
  glNormal3f(-1.0, 0.0, 0.0);
  glVertex3f(100.0, -100.0, -320.0);
  glVertex3f(100.0, 100.0, -320.0);
  glVertex3f(100.0, 100.0, -520.0);
  glVertex3f(100.0, -100.0, -520.0);

  /* Ceiling. */
  glNormal3f(0.0, -1.0, 0.0);
  glVertex3f(-100.0, 100.0, -320.0);
  glVertex3f(-100.0, 100.0, -520.0);
  glVertex3f(100.0, 100.0, -520.0);
  glVertex3f(100.0, 100.0, -320.0);

  /* Back wall. */
  glNormal3f(0.0, 0.0, 1.0);
  glVertex3f(-100.0, -100.0, -520.0);
  glVertex3f(100.0, -100.0, -520.0);
  glVertex3f(100.0, 100.0, -520.0);
  glVertex3f(-100.0, 100.0, -520.0);
  glEnd();

  cone();

  sphere();

  glPushMatrix();
  glTranslatef(lightPos[X], lightPos[Y], lightPos[Z]);
  glCallList(DL_LIGHT);
  glPopMatrix();

  /* Draw shadowing object. */
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, shad_mat);

  renderShadowingObject();
}

void
generateShadowVolume(void)
{
  GLfloat *eyePos;

  if (view == M_LIGHT_SOURCE_VIEW) {
    eyePos = lightPos;
  } else {
    eyePos = sceneEyePos;
  }
  /* XXX The 2048 feedbackBufferGuessSize is large enough to
     workaround the Octane/Impact bug where if the feedback
     buffer is under 2048 entries, a buggy hardware feedback
     path is used.  2048 forces the (bug free) software path.
     This bug is fixed in IRIX 6.5. */
  svsGenerateShadowVolume(svs, renderShadowingObject, 2048, maxRadius,
    lightPos, objectPos, eyePos);
}

void
display(void)
{
  if (renderBoundary) {
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    generateShadowVolume();
    glEnable(GL_LIGHTING);
  } else {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    switch (renderMode) {
    case M_NO_SHADOW:
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_LIGHT0);
      scene();
      break;
    case M_NO_LIGHT:
      /* Render scene without the light source enabled (conceptually, the
         entire scene is "in the shadow"). */
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_LIGHT0);
      scene();
      break;
    case M_FRONT_VOLUME:
    case M_BACK_VOLUME:
      generateShadowVolume();

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_LIGHT0);
      scene();
      if (frontFace) {
        glFrontFace(GL_CW);
      } else {
        glFrontFace(GL_CCW);
      }
      glCallList(DL_SHADOW_VOLUME);
      glFrontFace(GL_CCW);
      break;
    case M_SHADOW:
      /* Construct DL_SHADOW_VOLUME display list for the scene's current
         shadow volume. */
      generateShadowVolume();

      /* 1st scene pass. */
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_LIGHT0);
      scene();

      /* 1st shadow volume pass:  Enable stencil to increment the stencil
         value of pixels that pass the depth test when drawing the front
         facing polygons of the shadow volume.  Do not update the depth
         buffer while rendering the shadow volume. */
      glDisable(GL_LIGHTING);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glEnable(GL_STENCIL_TEST);
      glDepthMask(GL_FALSE);
      glStencilFunc(GL_ALWAYS, 0, 0);
      glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
      glCullFace(GL_FRONT);
      glCallList(DL_SHADOW_VOLUME);

      /* 2nd shadow volume pass:  Now, draw the back facing polygons of the
         shadow volume except decrement pixels that pass the depth test.
         Again, do not update the depth buffer. */
      glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
      glCullFace(GL_BACK);
      glCallList(DL_SHADOW_VOLUME);

      glDisable(GL_CULL_FACE);
      glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
      glCallList(DL_SHADOW_VOLUME_TOP);
      glEnable(GL_CULL_FACE);

      /* Now, pixels that lie within the shadow volume are tagged with a one
         stencil value.  Empty shadowed regions of the shadow volume get
         incremented, then decremented, to resolve to a net zero stencil
         value. */

      /* 2nd scene pass (render shadowed region):  Re-enable update of the
         depth and color buffer (use GL_LEQUAL for depth buffer so we can
         over-write depth values again with color.  Switch back to backface
         culling and disable the light source.  Only update pixels with a
         stencil value of one (shadowed). */
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#if 0
      glDepthFunc(GL_EQUAL);
#else
      glDepthMask(GL_TRUE);
      glDepthFunc(GL_LEQUAL);
#endif
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      glStencilFunc(GL_EQUAL, 1, 1);
      glDisable(GL_LIGHT0);
      glEnable(GL_LIGHTING);
      scene();

      /* Put state back to sane modes. */
      glDepthMask(GL_TRUE);
      glDepthFunc(GL_LESS);
      glDisable(GL_STENCIL_TEST);
      break;
    }
  }
  glutSwapBuffers();
}

void
idle(void)
{
  if (rotatingObject) {
    angle += 10.0;
  }
  if (swingingLight) {
    swingTime += 0.05;
    lightPos[X] += 2 * cos(swingTime);
  }
  glutPostRedisplay();
}

void
menu(int value)
{
  switch (value) {
  case M_TORUS:
  case M_CUBE:
  case M_SPHERE:
  case M_ICO:
  case M_DOUBLE_TORUS:
    shape = value;
    maxRadius = objectMaxRadius[value];
    glutPostRedisplay();
    break;
  case M_ANGLE:
    angle += 10.0;
    glutPostRedisplay();
    break;
  case M_BOUNDARY:
    renderBoundary = 1;
    glutPostRedisplay();
    break;
  case M_FRONT_VOLUME:
  case M_BACK_VOLUME:
    frontFace = (value == M_FRONT_VOLUME);
    /* FALLTHROUGH */
  case M_NO_SHADOW:
  case M_NO_LIGHT:
  case M_SHADOW:
    renderBoundary = 0;
    renderMode = value;
    glutPostRedisplay();
    break;
  case M_LIGHT_SOURCE_VIEW:
  case M_NORMAL_VIEW:
    view = value;
    glutPostRedisplay();
    break;
  case M_STOP:
    swingingLight = 0;
    rotatingObject = 0;
    glutIdleFunc(NULL);
    break;
  case M_SPIN:
    rotatingObject = 1;
    glutIdleFunc(idle);
    break;
  case M_SWING:
    swingingLight = 1;
    glutIdleFunc(idle);
    break;
  case 666:
    svsFreeShadowVolumeState(svs);
    exit(0);
    /* NOTREACHED */
    break;
  }
}

void
visible(int vis)
{
  if (vis == GLUT_VISIBLE && swingingLight && rotatingObject) {
    glutIdleFunc(idle);
  } else {
    glutIdleFunc(NULL);
  }
}

/* ARGSUSED1 */
void
key(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:             /* Escape. */
    svsFreeShadowVolumeState(svs);
    exit(0);
    break;
  case 13:             /* Return. */
    swingingLight = !swingingLight;
    swingTime = M_PI / 2.0;
    break;
  case ' ':            /* Space. */
    if (rotatingObject || swingingLight) {
      rotatingObject = 0;
      swingingLight = 0;
      glutIdleFunc(NULL);
    } else {
      rotatingObject = 1;
      swingingLight = 1;
      glutIdleFunc(idle);
    }
    break;
  }
}

/* ARGSUSED1 */
void
special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_HOME:
    frontFace = !frontFace;
    glutPostRedisplay();
    break;
  case GLUT_KEY_UP:
    lightPos[Y] += 10.0;
    glutPostRedisplay();
    break;
  case GLUT_KEY_DOWN:
    lightPos[Y] -= 10.0;
    glutPostRedisplay();
    break;
  case GLUT_KEY_PAGE_UP:
    lightPos[Z] += 10.0;
    glutPostRedisplay();
    break;
  case GLUT_KEY_PAGE_DOWN:
    lightPos[Z] -= 10.0;
    glutPostRedisplay();
    break;
  case GLUT_KEY_RIGHT:
    lightPos[X] += 10.0;
    glutPostRedisplay();
    break;
  case GLUT_KEY_LEFT:
    lightPos[X] -= 10.0;
    glutPostRedisplay();
    break;
  }
}

/* Create a single component checkboard texture map. */
GLfloat *
makeTexture(int maxs, int maxt)
{
  int s, t;
  static GLfloat *texture;

  texture = (GLfloat *) malloc(maxs * maxt * sizeof(GLfloat));
  for (t = 0; t < maxt; t++) {
    for (s = 0; s < maxs; s++) {
      texture[s + maxs * t] = ((s >> 4) & 0x1) ^ ((t >> 4) & 0x1);
    }
  }
  return texture;
}

void
initScene(void)
{
  GLfloat *tex;
  GLUquadricObj *qobj;
  static GLfloat sphere_mat[] =
  {1.0, 0.5, 0.0, 1.0};
  static GLfloat cone_mat[] =
  {0.0, 0.5, 1.0, 1.0};

  /* Turn on features. */
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_CULL_FACE);

  /* remove back faces to speed things up */
  glCullFace(GL_BACK);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  /* Make display lists for sphere and cone; for efficiency. */

  qobj = gluNewQuadric();

  glNewList(DL_BALL, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
  gluSphere(qobj, 20.0, 20, 20);
  glEndList();

  glNewList(DL_CONE, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
  glRotatef(-90.0, 1.0, 0.0, 0.0);
  gluDisk(qobj, 0.0, 20.0, 20, 1);
  gluCylinder(qobj, 20.0, 0.0, 60.0, 20, 20);
  glEndList();

  glNewList(DL_LIGHT, GL_COMPILE);
  glDisable(GL_LIGHTING);
  glColor3f(0.9, 0.9, 0.6);
  gluSphere(qobj, 5.0, 20, 20);
  glEnable(GL_LIGHTING);
  glEndList();

  gluDeleteQuadric(qobj);

  /* load pattern for current 2d texture */
  tex = makeTexture(TEXDIM, TEXDIM);
  glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
  free(tex);
}

int
main(int argc, char **argv)
{
  int shapeMenu, viewMenu, actionMenu, renderModeMenu;

  svs = svsCreateShadowVolumeState(1000.0, NULL);

  glutInitDisplayString("stencil>=1 rgb double depth samples");
  glutInit(&argc, argv);

  glutCreateWindow("shadowfun");

  stencilBits = glutGet(GLUT_WINDOW_STENCIL_SIZE);
  printf("bits of stencil = %d\n", stencilBits);

  glutDisplayFunc(display);
  glutVisibilityFunc(visible);
  glutSpecialFunc(special);
  glutKeyboardFunc(key);

  initScene();

  shapeMenu = glutCreateMenu(menu);
  glutAddMenuEntry("Torus", M_TORUS);
  glutAddMenuEntry("Cube", M_CUBE);
  glutAddMenuEntry("Sphere", M_SPHERE);
  glutAddMenuEntry("Icosahedron", M_ICO);
  glutAddMenuEntry("Double Torus", M_DOUBLE_TORUS);

  viewMenu = glutCreateMenu(menu);
  glutAddMenuEntry("Normal view", M_NORMAL_VIEW);
  glutAddMenuEntry("Light source view", M_LIGHT_SOURCE_VIEW);

  renderModeMenu = glutCreateMenu(menu);
  glutAddMenuEntry("With shadow", M_SHADOW);
  glutAddMenuEntry("With front shadow volume", M_FRONT_VOLUME);
  glutAddMenuEntry("With back shadow volume", M_BACK_VOLUME);
  glutAddMenuEntry("Without shadow", M_NO_SHADOW);
  glutAddMenuEntry("Without light", M_NO_LIGHT);
  glutAddMenuEntry("2D shadow boundary", M_BOUNDARY);

  actionMenu = glutCreateMenu(menu);
  glutAddMenuEntry("Spin object", M_SPIN);
  glutAddMenuEntry("Swing light", M_SWING);
  glutAddMenuEntry("Stop", M_STOP);

  glutCreateMenu(menu);
  glutAddSubMenu("Object shape", shapeMenu);
  glutAddSubMenu("Viewpoint", viewMenu);
  glutAddSubMenu("Render mode", renderModeMenu);
  glutAddSubMenu("Action", actionMenu);
  glutAddMenuEntry("Step rotate", M_ANGLE);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  menu(M_DOUBLE_TORUS);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}

#else
int main(int argc, char** argv)
{
  fprintf(stderr, "This program requires the new tesselator API in GLU 1.2.\n");
  fprintf(stderr, "Your GLU library does not support this new interface, sorry.\n");
  return 0;
}
#endif  /* GLU_VERSION_1_2 */
