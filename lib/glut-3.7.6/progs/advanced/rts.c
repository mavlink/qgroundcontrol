
/* Copyright (c) Mark J. Kilgard, 1997, 1998. */

/* This program is freely distributable without licensing fees and is
   provided without guarantee or warrantee expressed or implied.  This
   program is -not- in the public domain. */

/* Real-time Shadowing library, Version 0.97 */

/* XXX This is library is not fully implemented yet, but still quite
   functional. */

/* XXX This code _does_ assume that you that your realloc can realloc NULL
   (as specified by ANSI C).  You also should have the 1.2 version of the
   OpenGL Utility (GLU) library.  SGI users need IRIX 6.2 (or higher) or IRIX 
   5.3 with patch 1449 installed. */

/* This code will use multiple CPUs if available in its SGI version
   using IRIX's Shared Parallel Arena facility.  The generation of
   silhouettes with the GLU 1.2 tessellator is farmed out to a gang for
   tessellation threads. */

/* This code will use Win32's multithreading and multiple CPUs if
   available when compiled with the Visual C++ "/MT" option.  Just
   like with the IRIX multiprocessor support, the generation of
   silhouettes with the GLU 1.2 tessellator is farmed out to a gang
   of tessellation threads. -mjk July 28, 1998. */

/* Please do not naively assume that enabling the multiprocessor
   code will make rts-based programs run any faster if you do not
   have multiple CPUs.  Indeed, the extra thread overhead will in
   fact likely make the program slightly slower. */

#ifdef __sgi
# define MP
#endif

#ifdef _WIN32
# include <windows.h>  /* for wglGetProcAddress */
# ifdef _MT  /* If Visual C++ "/MT" compiler switch specified. */
#  define MP
# endif
#endif

#ifndef NDEBUG
#define NDEBUG  /* No assertions for best performance. */
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#ifdef MP
# ifdef __sgi
#  include <unistd.h>
#  include <sys/prctl.h>
#  include <ulocks.h>
#  include <signal.h>
#  include <sys/sysmp.h>
# endif
# ifdef _WIN32
#  include <process.h>
# endif
#endif

#include "rtshadow.h"

/* The "real time shadows" (RTS) library code requires the GLU 1.2
   polygon tessellator's boundary return capability to work. */
#ifdef GLU_VERSION_1_2

/* Win32 calling conventions. */
#ifndef CALLBACK
#define CALLBACK
#endif

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* For testing... */
#if 0
#undef GL_VERSION_1_1
#undef GL_EXT_vertex_array
#endif

#if defined(GL_VERSION_1_1) || defined(GL_EXT_vertex_array)
static int hasVertexArray = 0;
#else
static const int hasVertexArray = 0;
#endif

#ifdef GL_EXT_blend_subtract
static int hasBlendSubtract = 0;
#if defined(_WIN32) && !defined(MESA)
PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT = NULL;
#endif
#else
static const int hasBlendSubtract = 0;
#endif

/* Coordinates. */
enum {
  X, Y, Z
};

struct VertexHolder2D {
  struct VertexHolder2D *next;
  GLfloat v[2];
};

struct VertexHolder3D {
  struct VertexHolder3D *next;
  GLfloat v[3];
};

#ifndef MP

# define NUM_CONTEXTS 1
# define MP_ASSERT(assertion)

# define ARENA_VARIABLE(arena)
# define SEMA_VARIABLE(sema)
# define LOCK_VARIABLE(lock)
# define THREAD_VARIABLE(thread)
# define INITSEMA(arena)
# define WAIT(sema)
# define SIGNAL(sema)
# define INITLOCK(arena)
# define LOCK(lock)
# define UNLOCK(lock)
# define PRIVATE_MALLOC(size)        malloc(size)
# define PRIVATE_FREE(ptr)           free(ptr)
# define PRIVATE_REALLOC(ptr, size)  realloc(ptr, size)
# define SHARED_MALLOC(size)         malloc(size)
# define SHARED_FREE(ptr)            free(ptr)
# define SHARED_REALLOC(ptr, size)   realloc(ptr, size)

#else

# define NUM_CONTEXTS 4
# define MP_ASSERT(assertion)        assert(assertion)

# ifdef __sgi

#  define ARENA_VARIABLE(arena)      usptr_t *arena;
#  define SEMA_VARIABLE(sema)        usema_t *sema;
#  define LOCK_VARIABLE(lock)        ulock_t lock;
#  define THREAD_VARIABLE(thread)    pid_t thread;

#  define INITSEMA(arena, value)     usnewsema(arena, value)
#  define WAIT(sema)                 uspsema(sema)
#  define SIGNAL(sema)               usvsema(sema)
#  define SAMPLE(sema)               ustestsema(sema)

#  define INITLOCK(arena)            usnewlock(arena)
#  define LOCK(lock)                 ussetlock(lock)
#  define UNLOCK(lock)               usunsetlock(lock)

#  define PRIVATE_MALLOC(size)       malloc(size)
#  define PRIVATE_FREE(ptr)          free(ptr)
#  define PRIVATE_REALLOC(ptr, size) realloc(ptr, size)

#  define SHARED_MALLOC(size)        usmalloc(size, arena)
#  define SHARED_FREE(ptr)           usfree(ptr, arena)
#  define SHARED_REALLOC(ptr, size)  usrealloc(ptr, size, arena)

# endif  /* __sgi */

# ifdef _WIN32

#  define ARENA_VARIABLE(arena)      HANDLE arena;
#  define SEMA_VARIABLE(sema)        HANDLE sema;
#  define LOCK_VARIABLE(lock)        HANDLE lock;
#  define THREAD_VARIABLE(thread)    HANDLE thread;

#  define INITSEMA(arena, value)     CreateSemaphore(NULL, value, NUM_CONTEXTS, NULL)
#  define WAIT(sema)                 WaitForSingleObject(sema, INFINITE)
#  define SIGNAL(sema)               ReleaseSemaphore(sema, 1, NULL)

   /* Does Win32 have something cheaper than a mutex for locking? */
#  define INITLOCK(arena)            CreateMutex(NULL, FALSE, NULL)
#  define LOCK(lock)                 WaitForSingleObject(lock, INFINITE)
#  define UNLOCK(lock)               ReleaseMutex(lock)

#  define PRIVATE_MALLOC(size)       malloc(size)
#  define PRIVATE_FREE(ptr)          free(ptr)
#  define PRIVATE_REALLOC(ptr, size) realloc(ptr, size)

#  define SHARED_MALLOC(size)        malloc(size)
#  define SHARED_FREE(ptr)           free(ptr)
#  define SHARED_REALLOC(ptr, size)  realloc(ptr, size)

# endif  /* _WIN32 */

typedef enum {
  CS_UNUSED, CS_CAPTURING, CS_QUEUED, CS_GENERATING
} ContextState;

#endif

typedef struct ShadowVolumeState ShadowVolumeState;

typedef struct TessellationContext {
#ifdef MP
  ContextState state;
#endif
  RTSscene *scene;
  RTSlight *light;
  RTSobject *object;
  ShadowVolumeState *svs;

  GLUtesselator *tess;

  /* For managing memory allocated by the GLU tessellator's
     combine callback. */
  GLfloat *combineList;
  int combineListSize;
  int combineNext;
  struct VertexHolder2D *excessList2D;

  int saveFirst;
  GLfloat *firstVertex;

  GLfloat *feedbackBuffer;
  int feedbackBufferSize;
  int feedbackBufferReturned;

  GLfloat shadowProjectionDistance;
  GLfloat extentScale;

  int nextVertex;
  int *header;
  struct VertexHolder3D *excessList3D;
} TessellationContext;

const float uniquePassThroughValue = 34567.0;

#define SmallerOf(a,b) ((a) < (b) ? (a) : (b))

ARENA_VARIABLE(arena)
SEMA_VARIABLE(contextAvailable)
LOCK_VARIABLE(accessQueue)
SEMA_VARIABLE(silhouetteNeedsGeneration)

static TessellationContext *context[NUM_CONTEXTS];

struct RTSscene {
  GLfloat eyePos[3];
  GLbitfield usableStencilBits;
  int numStencilBits;
  char bitList[32];
  void (*renderSceneFunc) (GLenum castingLight, void *sceneData, RTSscene * scene);
  void *sceneData;

  SEMA_VARIABLE(silhouetteGenerationDone)
  THREAD_VARIABLE(*workerPids)
#ifdef MP
  ShadowVolumeState *waitingForSVS;
#endif

  GLfloat viewScale;
  GLint stencilBits;
  int stencilValidateNeeded;

  GLfloat sceneAmbient[4];

  int lightListSize;
  RTSlight **lightList;

  GLboolean stencilRenderingInvariantHack;
};

struct ShadowVolumeState {
  int lightSernum;
  int objectSernum;
#ifdef MP
  int generationDone;
#endif

  int silhouetteSize;
  GLfloat *silhouette;

  GLfloat angle;
  GLfloat axis[3];

  GLfloat topScale;
};

struct RTSlight {
  int refcnt;
  int sernum;

  GLenum glLight;
  GLfloat lightPos[3];
  GLfloat radius;

  int state;

  int sceneListSize;
  RTSscene **sceneList;

  int objectListSize;
  RTSobject **objectList;
  ShadowVolumeState *shadowVolumeList;
};

struct RTSobject {
  int refcnt;
  int sernum;

  GLfloat objectPos[3];
  GLfloat maxRadius;
  void (*renderObject) (void *objectData);
  void *objectData;

  int feedbackBufferSizeGuess;

  int state;

  int lightListSize;
  RTSlight **lightList;
};

#if defined(GL_VERSION_1_1)
static int
supportsOneDotOne(void)
{
  const char *version;
  int major, minor;

  version = (char *) glGetString(GL_VERSION);
  if (sscanf(version, "%d.%d", &major, &minor) == 2)
    return major >= 1 && minor >= 1;
  return 0;             /* OpenGL version string malformed! */
}
#endif

static int
extensionSupported(const char *extension)
{
  static const GLubyte *extensions = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  /* Extension names should not have spaces. */
  where = (GLubyte *) strchr(extension, ' ');
  if (where || *extension == '\0')
    return 0;

  if (!extensions)
    extensions = glGetString(GL_EXTENSIONS);
  /* It takes a bit of care to be fool-proof about parsing the OpenGL
     extensions string.  Don't be fooled by sub-strings,  etc. */
  start = extensions;
  for (;;) {
    where = (GLubyte *) strstr((const char *) start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ') {
      if (*terminator == ' ' || *terminator == '\0') {
        return 1;
      }
    }
    start = terminator;
  }
  return 0;
}

static GLfloat *
nextVertexHolder3D(TessellationContext * context)
{
  struct VertexHolder3D *holder;
  ShadowVolumeState *svs;
  GLfloat *newHolder;

  svs = context->svs;
  if (context->nextVertex >= svs->silhouetteSize) {
    holder = (struct VertexHolder3D *) PRIVATE_MALLOC(sizeof(struct VertexHolder3D));
    if (holder == NULL) {
      printf("holder alloc problem\n");
    }
    holder->next = context->excessList3D;
    context->excessList3D = holder;
    newHolder = holder->v;
  } else {
    newHolder = &svs->silhouette[context->nextVertex * 3];
  }
  context->nextVertex++;
  return newHolder;
}

/* ARGSUSED */
static void CALLBACK
begin(GLenum type, void *polyData)
{
  TessellationContext *context = polyData;
  GLfloat *newHolder;

  assert(type == GL_LINE_LOOP);
  context->saveFirst = 1;

  context->header = (int *) nextVertexHolder3D(context);
  context->header[0] = context->nextVertex;
  context->header[1] = 0xdeadbabe;  /* Aid assertion testing. */
  context->header[2] = 0xdeadbeef;  /* Non-termintor token. */

  newHolder = nextVertexHolder3D(context);
  newHolder[X] = 0.0;
  newHolder[Y] = 0.0;
  newHolder[Z] = 0.0;
}

static void CALLBACK
vertex(void *data, void *polyData)
{
  TessellationContext *context = polyData;
  GLfloat *v = data;
  GLfloat *newHolder;

  newHolder = nextVertexHolder3D(context);
  newHolder[X] = context->extentScale * v[X];
  newHolder[Y] = context->extentScale * v[Y];
  newHolder[Z] = context->shadowProjectionDistance;
  if (context->saveFirst) {
    context->firstVertex = newHolder;
    context->saveFirst = 0;
  }
}

static void CALLBACK
end(void *polyData)
{
  TessellationContext *context = polyData;
  GLfloat *newHolder;

  newHolder = nextVertexHolder3D(context);
  newHolder[X] = context->firstVertex[X];
  newHolder[Y] = context->firstVertex[Y];
  newHolder[Z] = context->firstVertex[Z];
  assert(context->firstVertex[Z] == context->shadowProjectionDistance);

  assert(context->header[1] == 0xdeadbabe);
  assert(context->header[2] == 0xdeadbeef);
  context->header[1] = context->nextVertex - context->header[0];
}

static void
freeExcessList(TessellationContext * context)
{
  struct VertexHolder2D *holder, *next;

  holder = context->excessList2D;
  while (holder) {
    next = holder->next;
    PRIVATE_FREE(holder);
    holder = next;
  }
  context->excessList2D = NULL;
}

/* The GLU tessellator's combine callback is called to create a
   new vertex when tessellation detects an intersection or wishes
   to merge features.

   The memory for the new vertex must be allocated by the GLU
   tessellator caller.  The caller is also responsible for this
   memory's deletion.  The combineList is an array for the memory
   for these combined vertices.  The array is of size combineListSize.
   combineNext decides how many vertices are in use on the combineList.

   The combineList is of finite size.  When this list is exhausted,
   individual vertex memory is allocated via malloc in a linked list.
   This is the excessList2D linked list.  After tessellation, the
   combineList will be expanded by how many vertices had to be
   added to the excessList2D list.  The idea is that next time the
   shadow volume tessellation is done, the combineList should
   hopefully be large enough.

/* ARGSUSED1 */
static void CALLBACK
combine(GLdouble coords[3], void *d[4], GLfloat w[4],
  void **dataOut, void *polyData)
{
  TessellationContext *context = polyData;
  struct VertexHolder2D *holder;
  GLfloat *newHolder;

  if (context->combineNext >= context->combineListSize) {
    holder = (struct VertexHolder2D *) PRIVATE_MALLOC(sizeof(struct VertexHolder2D));
    if (holder == NULL) {
      printf("got no holder alloc\n");
    }
    holder->next = context->excessList2D;
    context->excessList2D = holder;
    newHolder = holder->v;
  } else {
    newHolder = &context->combineList[context->combineNext * 2];
  }

  newHolder[0] = (GLfloat) coords[0];
  newHolder[1] = (GLfloat) coords[1];
  *dataOut = newHolder;

  context->combineNext++;
}

static void CALLBACK
error(GLenum errno)
{
  fprintf(stderr, "ERROR: %s\n", gluErrorString(errno));
}

#ifdef DEBUG

/* These verify routines are useful for asserting the sanity of
   the silhouette data structures. */

static void
verifySilhouette(ShadowVolumeState * svs)
{
  int *infoPtr = (int *) svs->silhouette;
  int *info = infoPtr;
  int fan;

  if (info[0] == 0) {
    printf("2 ZERO\n");
  }
  if (info ==0) {
    printf("ZERO\n");
  }
  fan = 0;
  for (;;) {
    if(info[2] == 0xdeadbeef || info[2] == 0xcafecafe) {
      if (info[2] == 0xcafecafe) {
        return;
      }
      info += ((1 + info[1]) * 3);
      fan++;
    } else {
      printf("Corrupted silhouette! (svs=0x%x, silhouette=0x%x, fan=%d)\n",
        svs, svs->silhouette, fan);
      abort();
    }
  }
}

static void
verifySilhouettesOfScene(RTSscene *scene)
{
  int i, obj;
  RTSlight *light;

  for (i = 0; i < scene->lightListSize; i++) {
    light = scene->lightList[i];
    if (light) {
      for (obj = 0; obj < light->objectListSize; obj++) {
        ShadowVolumeState *svs;

        svs = &light->shadowVolumeList[obj];
	if (svs && svs->generationDone) {
	  verifySilhouette(svs);
	}
      }
    }
  }
}

#endif

static void
generateSilhouette(TessellationContext * context)
{
  ShadowVolumeState * svs;
  GLfloat *start, *end, *loc;
  GLfloat *eyeLoc;
  GLdouble v[3];
  int token, nvertices, i;
  GLfloat passThroughToken;
  int watchingForEyePos;
  struct VertexHolder3D *holder, *next;

  assert(context->excessList2D == NULL);
  assert(context->excessList3D == NULL);

  svs = context->svs;

  context->nextVertex = 0;

  watchingForEyePos = 0;
  eyeLoc = NULL;

  gluTessBeginPolygon(context->tess, context);
  start = context->feedbackBuffer;
  end = start + context->feedbackBufferReturned;
  for (loc = start; loc < end;) {
    token = *loc;
    loc++;
    switch (token) {
    case GL_POLYGON_TOKEN:
      nvertices = *loc;
      loc++;
      assert(nvertices >= 3);
      gluTessBeginContour(context->tess);
      for (i = 0; i < nvertices; i++) {
        v[0] = loc[0];
        v[1] = loc[1];
        v[2] = 0.0;
        gluTessVertex(context->tess, v, loc);
        loc += 2;
      }
      gluTessEndContour(context->tess);
      break;
    case GL_PASS_THROUGH_TOKEN:
      passThroughToken = *loc;
      if (passThroughToken == uniquePassThroughValue) {
        watchingForEyePos = !watchingForEyePos;
      } else {
        /* Ignore everything else. */
        fprintf(stderr, "WARNING: Unexpected feedback token 0x%x (%d).\n",
          token, token);
      }
      loc++;
      break;
    case GL_POINT_TOKEN:
      if (watchingForEyePos) {
        fprintf(stderr,
          "WARNING: Eye point possibly within the shadow volume.\n");
        fprintf(stderr,
          "         Program should be improved to handle this.\n");
        /* XXX Write code to handle this case.  You would need to determine
           if the point was instead any of the returned boundary polyons.
           Once you found that you were really in the clipping volume, then I
           haven't quite thought about what you do. */
        eyeLoc = loc;
        watchingForEyePos = 0;
      } else {
        /* Ignore everything else. */
        fprintf(stderr, "WARNING: Unexpected feedback token 0x%x (%d).\n",
          token, token);
      }
      loc += 2;
      break;
    default:
      /* Ignore everything else. */
      fprintf(stderr, "WARING: Unexpected feedback token 0x%x (%d).\n",
        token, token);
    }
  }
  gluTessEndPolygon(context->tess);

  /* Free any memory that got allocated due to the combine callback during
     tessellation and then enlarge the combineList so we hopefully don't need
     the combine list next time. */
  if (context->combineNext > context->combineListSize) {
    freeExcessList(context);
    context->combineListSize = context->combineNext;
    SHARED_FREE(context->combineList);
    context->combineList = SHARED_MALLOC(sizeof(GLfloat) * 2 * context->combineListSize);
    if (context->combineList == NULL) {
      printf("problem alloc context->combineList\n");
    }
  }
  context->combineNext = 0;

  context->header[2] = 0xcafecafe;  /* Terminating token. */

  if (context->excessList3D) {
#ifndef NDEBUG
    int oldSize;

    oldSize = svs->silhouetteSize;
#endif
    assert(context->nextVertex > svs->silhouetteSize);
    svs->silhouetteSize = context->nextVertex;
    svs->silhouette = SHARED_REALLOC(svs->silhouette,
      svs->silhouetteSize * sizeof(GLfloat) * 3);
    if (svs->silhouette == NULL) {
      fprintf(stderr, "libRTS: generateSilhouette: out of memory\n");
      abort();
    }
    holder = context->excessList3D;
    while (holder) {
      context->nextVertex--;
      svs->silhouette[context->nextVertex * 3] = holder->v[0];
      svs->silhouette[context->nextVertex * 3 + 1] = holder->v[1];
      svs->silhouette[context->nextVertex * 3 + 2] = holder->v[2];
      next = holder->next;
      PRIVATE_FREE(holder);
      holder = next;
    }
    assert(context->nextVertex == oldSize);
    context->excessList3D = NULL;
  }

  /* Validate shadow volume state's serial numbers. */
  svs->lightSernum = context->light->sernum;
  svs->objectSernum = context->object->sernum;
}

static int
listBits(GLbitfield usableStencilBits, char bitList[32])
{
  int num = 0, bit = 0;

  while (usableStencilBits) {
    if (usableStencilBits & 0x1) {
      bitList[num] = bit;
      num++;
    }
    bit++;
    usableStencilBits >>= 1;
  }
  return num;
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

static GLfloat
getViewScale(RTSscene * scene)
{
  if (scene->viewScale == 0.0) {
    GLfloat maxViewSize[2];

    glGetFloatv(GL_MAX_VIEWPORT_DIMS, maxViewSize);
    scene->viewScale = SmallerOf(maxViewSize[0], maxViewSize[1]) / 2.0;

    /* Other stuff piggy backs on viewScale to ensure initialization. */

    glGetIntegerv(GL_STENCIL_BITS, &scene->stencilBits);

#if defined(GL_VERSION_1_1)
    hasVertexArray = supportsOneDotOne();
#elif defined(GL_EXT_vertex_array)
    hasVertexArray = extensionSupported("GL_EXT_vertex_array");
#endif

#ifdef GL_EXT_blend_subtract
    hasBlendSubtract = extensionSupported("GL_EXT_blend_subtract");
#if defined(_WIN32) && !defined(MESA)
    if (hasBlendSubtract) {
      glBlendEquationEXT = (PFNGLBLENDEQUATIONEXTPROC) wglGetProcAddress("glBlendEquationEXT");
      if (glBlendEquationEXT == NULL) {
        hasBlendSubtract = 0;
      }
    }
#endif

    /* XXX RealityEngine workaround. */
    if (!strcmp((char *) glGetString(GL_VENDOR), "SGI")) {
      if (!strncmp((char *) glGetString(GL_RENDERER), "RE", 2)) {
        fprintf(stderr, "WARNING: RealityEngine workaround forcing additive blending.\n");
        hasBlendSubtract = 0;
      }
    }
#endif
  }
  return scene->viewScale;
}

static void
captureLightView(RTSscene * scene, RTSlight * light, RTSobject * object,
  ShadowVolumeState * svs, TessellationContext * context)
{
  static GLfloat unit[3] =
  {0.0, 0.0, 1.0};
  int feedbackBufferSizeGuess;
  GLfloat lightDelta[3], eyeDelta[3];
  GLfloat lightDistance, eyeDistance, fieldOfViewRatio, viewScale;
  GLdouble fieldOfViewAngle;
  GLdouble nnear, ffar;  /* Avoid x86 C keywords.  Grumble. */
  GLint returned;

  MP_ASSERT(context->state == CS_CAPTURING);
  viewScale = getViewScale(scene);

#ifdef MP
  svs->generationDone = 0;
#endif

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  /* Calculate the light's distance from the object being shadowed. */
  lightDelta[X] = object->objectPos[X] - light->lightPos[X];
  lightDelta[Y] = object->objectPos[Y] - light->lightPos[Y];
  lightDelta[Z] = object->objectPos[Z] - light->lightPos[Z];
  lightDistance = (GLfloat) sqrt(lightDelta[X] * lightDelta[X] +
    lightDelta[Y] * lightDelta[Y] + lightDelta[Z] * lightDelta[Z]);

  /* Determine the appropriate field of view.  We want to use as narrow a
     field of view as possible to not waste resolution, but not narrower than
     the object.  Add 50% extra slop. */
  fieldOfViewRatio = object->maxRadius / lightDistance;
  if (fieldOfViewRatio > 0.99) {
    fprintf(stderr,
      "WARNING: Clamping FOV to 164 degrees for determining shadow.\n");
    fprintf(stderr,
      "         Light distance = %g, object maxmium radius = %g\n",
      lightDistance, object->maxRadius);

    /* 2*asin(0.99) ~= 164 degrees. */
    fieldOfViewRatio = 0.99;
  }
  /* Pre-compute scaling factors for the near and far extent of the shadow
     volume. */
  context->extentScale = light->radius * fieldOfViewRatio / viewScale;
  context->shadowProjectionDistance = light->radius;

  nnear = 0.5 * (lightDistance - object->maxRadius);
  if (nnear < 0.0001) {
    fprintf(stderr,
      "WARNING: Clamping near clip plane to 0.0001 because light source too near.\n");
    fprintf(stderr,
      "         Light distance = %g, object maxmium radius = %g\n",
      lightDistance, object->maxRadius);
    nnear = 0.0001;
  }
  ffar = 2.0 * (lightDistance + object->maxRadius);

  eyeDelta[X] = scene->eyePos[X] - light->lightPos[X];
  eyeDelta[Y] = scene->eyePos[Y] - light->lightPos[Y];
  eyeDelta[Z] = scene->eyePos[Z] - light->lightPos[Z];
  eyeDistance = 1.05 *
    sqrt(eyeDelta[X] * eyeDelta[X] + eyeDelta[Y] * eyeDelta[Y]
    + eyeDelta[Z] * eyeDelta[Z]);
  if (eyeDistance > ffar) {
    ffar = eyeDistance;
  }
  fieldOfViewAngle = 2.0 * asin(fieldOfViewRatio) * 180 / M_PI;
  gluPerspective(fieldOfViewAngle, 1.0, nnear, ffar);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  /* XXX Look up vector needs adjusting. */
  gluLookAt(light->lightPos[X], light->lightPos[Y], light->lightPos[Z],
    object->objectPos[X], object->objectPos[Y], object->objectPos[Z],
    0.0, 1.0, 0.0);     /* up is in positive Y direction */

  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(-viewScale, -viewScale, 2 * viewScale, 2 * viewScale);

  feedbackBufferSizeGuess = object->feedbackBufferSizeGuess;

doFeedback:

  if (feedbackBufferSizeGuess > context->feedbackBufferSize) {
    context->feedbackBufferSize = feedbackBufferSizeGuess;
    object->feedbackBufferSizeGuess = feedbackBufferSizeGuess;

    /* A "free & malloc" is better than a "realloc" below because we
       do not care for the previous buffer contents to be preserved. */
    /* XXX Add 32 words of slop (an extra cache line) to end for buggy
       hardware that uses DMA to return feedback results but that sometimes
       overrun the buffer.  Yuck. */
    SHARED_FREE(context->feedbackBuffer);
    context->feedbackBuffer = (GLfloat *)
      SHARED_MALLOC(context->feedbackBufferSize * sizeof(GLfloat) + 32 * 4);
    if (context->feedbackBuffer == NULL) {
      fprintf(stderr, "libRTS: captureLightView: out of memory\n");
      abort();
    }
  }
  glFeedbackBuffer(context->feedbackBufferSize,
    GL_2D, context->feedbackBuffer);

  (void) glRenderMode(GL_FEEDBACK);

  /* Render the eye position.  The eye position is "bracketed" by unique pass
     through tokens.  These bracketing pass through tokens let us determine if
     the eye position was clipped or not.  This helps us determine whether  the
     eye position is possibly within the shadow volume or not.  If the point is
     clipped, the eye position is not in the shadow volume.  If the point is
     not clipped, a more complicated test is necessary to determine if the eye
     position is really in the shadow volume or not.  See generateSilhouette. */
  glPassThrough(uniquePassThroughValue);
  glBegin(GL_POINTS);
  glVertex3fv(scene->eyePos);
  glEnd();
  glPassThrough(uniquePassThroughValue);

  (object->renderObject) (object->objectData);

  returned = glRenderMode(GL_RENDER);
  assert(returned <= context->feedbackBufferSize);
#if 0
  if (returned == -1) {
#else
  /* XXX RealityEngine workaround. */
  if (returned == -1 || returned == context->feedbackBufferSize) {
#endif
    feedbackBufferSizeGuess = context->feedbackBufferSize
      + (context->feedbackBufferSize >> 1);
    goto doFeedback;    /* Try again with larger feedback buffer. */
  }
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();        /* Restore viewport. */

  vcross(unit, lightDelta, svs->axis);
  svs->angle = (GLfloat) acos(vdot(unit, lightDelta) / lightDistance) * 180.0 / M_PI;
  svs->topScale = (lightDistance + object->maxRadius) / light->radius;

  context->feedbackBufferReturned = returned;

  context->scene = scene;
  context->light = light;
  context->object = object;
  context->svs = svs;
}

static TessellationContext *
createTessellationContext(void)
{
  TessellationContext *context;
  GLUtesselator *tess;

  context = (TessellationContext *) SHARED_MALLOC(sizeof(TessellationContext));
  if (context == NULL) {
    printf("TessellationContext alloc failed\n");
    return NULL;
  }
  tess = gluNewTess();
  if (tess == NULL) {
    SHARED_FREE(context);
    return NULL;
  }
  gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_TRUE);
  gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
  gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (void (CALLBACK*)()) &begin);
  gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (void (CALLBACK*)()) &vertex);
  gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (void (CALLBACK*)()) &combine);
  gluTessCallback(tess, GLU_TESS_END_DATA, (void (CALLBACK*)()) &end);
  gluTessCallback(tess, GLU_TESS_ERROR, error);
  context->tess = tess;

#ifdef MP
  context->state = CS_UNUSED;
#endif
  context->combineListSize = 0;
  context->combineList = NULL;
  context->combineNext = 0;
  context->excessList2D = NULL;

  context->feedbackBufferSize = 0;
  context->feedbackBuffer = NULL;

  context->excessList3D = NULL;

  return context;
}

#ifdef MP

static void
work(void)
{
  TessellationContext *workContext;
  RTSscene *scene;
  int i;

    WAIT(silhouetteNeedsGeneration);

    LOCK(accessQueue);
    workContext = NULL;
    for (i=0; i<NUM_CONTEXTS; i++) {
      if (context[i]->state == CS_QUEUED) {
        workContext = context[i];
	break;
      }
    }
    assert(workContext);
    workContext->state = CS_GENERATING;
    UNLOCK(accessQueue);

    generateSilhouette(workContext);

    LOCK(accessQueue);
    assert(workContext->state == CS_GENERATING);
    workContext->state = CS_UNUSED;
    workContext->svs->generationDone = 1;
    scene = workContext->scene;

    /* If the main thread is sleeping on this particular
       shadow volume, wake up the main thread. */
    if (workContext->svs == scene->waitingForSVS) {
      scene->waitingForSVS = NULL;
      SIGNAL(scene->silhouetteGenerationDone);
    }
    UNLOCK(accessQueue);
    SIGNAL(contextAvailable);
}

static void
waitForSilhouetteGenerationDone(RTSscene * scene, ShadowVolumeState *svs)
{
  LOCK(accessQueue);

  if (svs->generationDone == 0) {
    scene->waitingForSVS = svs;
    UNLOCK(accessQueue);
    WAIT(scene->silhouetteGenerationDone);
    assert(svs->generationDone);
  } else {
    UNLOCK(accessQueue);
  }
}

#ifdef __sgi

static void
worker(void)
{
  signal(SIGHUP, SIG_DFL);
  prctl(PR_TERMCHILD);
  usadd(arena);
  for (;;) {
    work();
  }
}

static void
setupArena(int numWorkers)
{
  static int beenhere = 0;
  int i;
  THREAD_VARIABLE(pid)

  if (beenhere) {
    return;
  }
  beenhere = 1;

  usconfig(CONF_INITUSERS, 1 + numWorkers);
  usconfig(CONF_ARENATYPE, US_SHAREDONLY);
  usconfig(CONF_INITSIZE, 1024 * 1024);
  arena = usinit("/dev/zero");
  if (arena == NULL) {
    fprintf(stderr, "libRTS: could not create arena.\n");
    exit(1);
  }

  for (i=0; i<NUM_CONTEXTS; i++) {
    context[i] = createTessellationContext();
  }

  contextAvailable = INITSEMA(arena, NUM_CONTEXTS);
  accessQueue = INITLOCK(arena);
  silhouetteNeedsGeneration = INITSEMA(arena, 0);

  for (i=0; i<numWorkers; i++) {
    pid = fork();
    if (pid == -1) {
      perror("fork");
      exit(1);
    }
    if (pid == 0) {
      worker();
    }
  }
}

#endif  /* __sgi */

#ifdef _WIN32

static void
worker(void *unused)
{
  for (;;) {
    work();
  }
}

static void
setupArena(int numWorkers)
{
  static int beenhere = 0;
  int i;
  unsigned long retval;

  if (beenhere) {
    return;
  }
  beenhere = 1;

  for (i=0; i<NUM_CONTEXTS; i++) {
    context[i] = createTessellationContext();
  }

  contextAvailable = INITSEMA(arena, NUM_CONTEXTS);
  accessQueue = INITLOCK(arena);
  silhouetteNeedsGeneration = INITSEMA(arena, 0);

  for (i=0; i<numWorkers; i++) {
    retval = _beginthread(worker, 0, NULL);
    if (retval == -1) {
      perror("_beginthread");
      exit(1);
    }
  }
}

#endif  /* _WIN32 */

#endif  /* MP */

RTSscene *
rtsCreateScene(
  GLfloat eyePos[3],
  GLbitfield usableStencilBits,
  void (*renderSceneFunc) (GLenum castingLight, void *sceneData, RTSscene * scene),
  void *sceneData)
{
  RTSscene *scene;

#ifdef MP
# ifdef __sgi
  int numProcessors = (int) sysmp(MP_NAPROCS);

  printf("numProcesasors = %d\n", numProcessors);
  setupArena(SmallerOf(4, numProcessors));
# endif
# ifdef _WIN32
  int numProcessors = 2;

  printf("numProcessors = %d\n", numProcessors);
  setupArena(numProcessors);
# endif
#else
  context[0] = createTessellationContext();
#endif

  scene = (RTSscene *) SHARED_MALLOC(sizeof(RTSscene));
  if (scene == NULL) {
    printf("rtsCreateScene alloc failed\n");
    return NULL;
  }

#ifdef MP
  scene->silhouetteGenerationDone = INITSEMA(arena, 0);
  scene->waitingForSVS = NULL;
#endif

  scene->eyePos[X] = eyePos[X];
  scene->eyePos[Y] = eyePos[Y];
  scene->eyePos[Z] = eyePos[Z];
  scene->usableStencilBits = usableStencilBits;
  scene->renderSceneFunc = renderSceneFunc;
  scene->sceneData = sceneData;

  scene->viewScale = 0.0;  /* 0.0 means "to be determined". */
  scene->stencilValidateNeeded = 1;

  scene->sceneAmbient[0] = 0.2;
  scene->sceneAmbient[1] = 0.2;
  scene->sceneAmbient[2] = 0.2;
  scene->sceneAmbient[3] = 1.0;

  scene->lightListSize = 0;
  scene->lightList = NULL;

  scene->stencilRenderingInvariantHack = GL_FALSE;

  return scene;
}

RTSlight *
rtsCreateLight(
  GLenum glLight,
  GLfloat lightPos[3],
  GLfloat radius)
{
  RTSlight *light;

  light = (RTSlight *) SHARED_MALLOC(sizeof(RTSlight));
  if (light == NULL) {
    printf("rtsCreateLight failed\n");
    return NULL;
  }
  light->refcnt = 1;
  light->sernum = 1;

  light->glLight = glLight;
  light->lightPos[X] = lightPos[X];
  light->lightPos[Y] = lightPos[Y];
  light->lightPos[Z] = lightPos[Z];
  light->radius = radius;

  light->state = RTS_SHINING_AND_CASTING;

  light->sceneListSize = 0;
  light->sceneList = NULL;

  light->objectListSize = 0;
  light->objectList = NULL;
  light->shadowVolumeList = NULL;

  return light;
}

RTSobject *
rtsCreateObject(
  GLfloat objectPos[3],
  GLfloat maxRadius,
  void (*renderObject) (void *objectData),
  void *objectData,
  int feedbackBufferSizeGuess)
{
  RTSobject *object;

  object = (RTSobject *) SHARED_MALLOC(sizeof(RTSobject));
  if (object == NULL) {
    printf("rtsCreateObject failed\n");
    return NULL;
  }
  object->refcnt = 1;
  object->sernum = 1;

  object->objectPos[X] = objectPos[X];
  object->objectPos[Y] = objectPos[Y];
  object->objectPos[Z] = objectPos[Z];
  object->maxRadius = maxRadius;
  object->renderObject = renderObject;
  object->objectData = objectData;
  object->feedbackBufferSizeGuess = feedbackBufferSizeGuess;
#ifdef __sgi
  /* XXX Impact/Octane feedback bug work around.  If the feedback
     buffer on Impact/Octane is less than 2048 entries, a buggy
     hardware accelerated path is used.  Make sure at least 2049
     entries for the feedback buffer; this forces Impact/Octane to use
     the (bug free) software feedback path.  This bug is fixed in IRIX
     6.5. */
  if (object->feedbackBufferSizeGuess < 2048) {
    object->feedbackBufferSizeGuess = 2048;
  }
#endif

  object->state = RTS_SHADOWING;

  object->lightListSize = 0;
  object->lightList = NULL;

  return object;
}

void
rtsAddLightToScene(
  RTSscene * scene,
  RTSlight * light)
{
  int i;

  for (i = 0; i < light->sceneListSize; i++) {
    if (light->sceneList[i] == scene) {
      return;
    }
    if (light->sceneList[i] == NULL) {
      goto addToSceneList;
    }
  }
  light->sceneListSize++;
  light->sceneList = (RTSscene **) SHARED_REALLOC(light->sceneList,
    light->sceneListSize * sizeof(RTSscene *));
  if (light->sceneList == NULL) {
    fprintf(stderr, "rtsAddLightToScene: out of memory\n");
    abort();
  }
addToSceneList:

  light->sceneList[i] = scene;

  for (i = 0; i < scene->lightListSize; i++) {
    if (scene->lightList[i] == light) {
      fprintf(stderr, "rtsAddLightToScene: inconsistent lists\n");
      abort();
    }
    if (scene->lightList[i] == NULL) {
      goto addToLightList;
    }
  }
  scene->lightListSize++;
  scene->lightList = (RTSlight **) SHARED_REALLOC(scene->lightList,
    scene->lightListSize * sizeof(RTSlight *));
  if (scene->lightList == NULL) {
    fprintf(stderr, "rtsAddLightToScene: out of memory\n");
    abort();
  }
addToLightList:

  scene->lightList[i] = light;

  light->refcnt++;
}

static void
initShadowVolumeState(ShadowVolumeState * svs)
{
  svs->lightSernum = 0;
  svs->objectSernum = 0;
  svs->silhouette = NULL;
  svs->silhouetteSize = 0;
#ifdef MP
  svs->generationDone = 0;
#endif
}

void
rtsAddObjectToLight(
  RTSlight * light,
  RTSobject * object)
{
  int i;

  for (i = 0; i < object->lightListSize; i++) {
    if (object->lightList[i] == light) {
      return;
    }
    if (object->lightList[i] == NULL) {
      goto addToLightList;
    }
  }
  object->lightListSize++;
  object->lightList = (RTSlight **) SHARED_REALLOC(object->lightList,
    object->lightListSize * sizeof(RTSlight *));
  if (object->lightList == NULL) {
    fprintf(stderr, "rtsAddObjectToLight: out of memory\n");
    abort();
  }
addToLightList:
  object->lightList[i] = light;

  for (i = 0; i < light->objectListSize; i++) {
    if (light->objectList[i] == object) {
      fprintf(stderr, "rtsAddObjectToLight: inconsistent lists\n");
      abort();
    }
    if (light->objectList[i] == NULL) {
      goto addToObjectList;
    }
  }

  /* Extend object list. */
  light->objectListSize++;
  light->objectList = (RTSobject **) SHARED_REALLOC(light->objectList,
    light->objectListSize * sizeof(RTSscene *));
  if (light->objectList == NULL) {
    fprintf(stderr, "rtsAddObjectToLight: out of memory\n");
    abort();
  }
  /* Extend shadow volume list. */
  light->shadowVolumeList = (ShadowVolumeState *)
    SHARED_REALLOC(light->shadowVolumeList,
    light->objectListSize * sizeof(ShadowVolumeState));
  if (light->shadowVolumeList == NULL) {
    fprintf(stderr, "rtsAddObjectToLight: out of memory\n");
    abort();
  }
addToObjectList:

  initShadowVolumeState(&light->shadowVolumeList[i]);

  light->objectList[i] = object;

  light->refcnt++;
  object->refcnt++;
}

void
rtsSetLightState(
  RTSlight * light,
  RTSlightState state)
{
  light->state = state;
}

void
rtsSetObjectState(
  RTSobject * object,
  RTSobjectState state)
{
  object->state = state;
}

void
rtsUpdateEyePos(
  RTSscene * scene,
  GLfloat eyePos[3])
{
  scene->eyePos[X] = eyePos[X];
  scene->eyePos[Y] = eyePos[Y];
  scene->eyePos[Z] = eyePos[Z];
}

void
rtsUpdateUsableStencilBits(
  RTSscene * scene,
  GLbitfield usableStencilBits)
{
  scene->usableStencilBits = usableStencilBits;
  scene->stencilValidateNeeded = 1;
}

void
rtsUpdateLightPos(
  RTSlight * light,
  GLfloat lightPos[3])
{
  light->lightPos[X] = lightPos[X];
  light->lightPos[Y] = lightPos[Y];
  light->lightPos[Z] = lightPos[Z];
  light->sernum++;
}

void
rtsUpdateLightRadius(
  RTSlight * light,
  GLfloat radius)
{
  light->radius = radius;
  light->sernum++;
}

void
rtsUpdateObjectPos(
  RTSobject * object,
  GLfloat objectPos[3])
{
  object->objectPos[X] = objectPos[X];
  object->objectPos[Y] = objectPos[Y];
  object->objectPos[Z] = objectPos[Z];
  object->sernum++;
}

void
rtsUpdateObjectShape(
  RTSobject * object)
{
  object->sernum++;
}

void
rtsUpdateObjectMaxRadius(
  RTSobject * object,
  GLfloat maxRadius)
{
  object->maxRadius = maxRadius;
  object->sernum++;
}

#if defined(GL_EXT_vertex_array) && !defined(GL_VERSION_1_1)
/* Only needed if has vertex array extension, but no OpenGL 1.1. */
static void
setupVertexArray(ShadowVolumeState * svs, int numCoordinates)
{
  glDisable(GL_EDGE_FLAG_ARRAY_EXT);
  glEnable(GL_VERTEX_ARRAY_EXT);
  glDisable(GL_NORMAL_ARRAY_EXT);
  glDisable(GL_COLOR_ARRAY_EXT);
  glDisable(GL_TEXTURE_COORD_ARRAY_EXT);
  glDisable(GL_EDGE_FLAG_ARRAY_EXT);
  glVertexPointerEXT(numCoordinates, GL_FLOAT, 3 * sizeof(GLfloat), svs->silhouetteSize, svs->silhouette);
}
#endif

static void
renderSilhouette(ShadowVolumeState * svs)
{
  int *infoPtr = (int *) svs->silhouette;
  int *info = infoPtr;
  int end, i;

  /* CONSTANTCONDITION */
  assert(sizeof(GLfloat) == sizeof(GLint));

  if (hasVertexArray) {
#if defined(GL_VERSION_1_1)
    glInterleavedArrays(GL_V2F, 3 * sizeof(GLfloat), svs->silhouette);
#elif defined(GL_EXT_vertex_array)
    setupVertexArray(svs, 2);
#endif
  }
  for (;;) {
    assert(info[2] == 0xdeadbeef || info[2] == 0xcafecafe);
    /* Two fewer vertices get rendered in the renderSilhouette case (compared
       to renderShadowVolumeBase) because because a line loop does not need
       the initial fan center or the final repeated first vertex. */
    if (hasVertexArray) {
#if defined(GL_VERSION_1_1)
      glDrawArrays(GL_LINE_LOOP, info[0] + 1, info[1] - 2);
#elif defined(GL_EXT_vertex_array)
      glDrawArraysEXT(GL_LINE_LOOP, info[0] + 1, info[1] - 2);
#endif
    } else {
      glBegin(GL_LINE_LOOP);
      end = info[0] + info[1] - 2;
      for (i = info[0] + 1; i < end; i++) {
        glVertex2fv(&svs->silhouette[i * 3]);
      }
      glEnd();
    }
    if (info[2] == 0xcafecafe) {
      return;
    }
    info += ((1 + info[1]) * 3);
  }
}

static void
renderShadowVolumeBase(ShadowVolumeState * svs)
{
  int *infoPtr = (int *) svs->silhouette;
  int *info = infoPtr;
  int end, i;
  int fan;

  fan = 0;
  /* CONSTANTCONDITION */
  assert(sizeof(GLfloat) == sizeof(GLint));
  glRotatef(svs->angle, svs->axis[X], svs->axis[Y], svs->axis[Z]);
  for (;;) {
    assert((info[2] == 0xdeadbeef || info[2] == 0xcafecafe) && info[1] > 0);
    if (hasVertexArray) {
      /* Note: assumes that glInterleavedArrays has already been called. */
#if defined(GL_VERSION_1_1)
      glDrawArrays(GL_TRIANGLE_FAN, info[0], info[1]);
#elif defined(GL_EXT_vertex_array)
      glDrawArraysEXT(GL_TRIANGLE_FAN, info[0], info[1]);
#endif
    } else {
      glBegin(GL_TRIANGLE_FAN);
      end = info[0] + info[1];
      for (i = info[0]; i < end; i++) {
        glVertex3fv(&svs->silhouette[i * 3]);
      }
      glEnd();
    }
    if (info[2] == 0xcafecafe) {
      return;
    }
    assert(info[1] > 0);
    info += ((1 + info[1]) * 3);
    fan++;
  }
}

static void
renderShadowVolume(ShadowVolumeState * svs, GLfloat lightPos[3])
{
  glPushMatrix();
  glTranslatef(lightPos[X], lightPos[Y], lightPos[Z]);
  renderShadowVolumeBase(svs);
  glPopMatrix();
}

static void
renderShadowVolumeTop(ShadowVolumeState * svs, GLfloat lightPos[3])
{
  glPushMatrix();
  glTranslatef(lightPos[X], lightPos[Y], lightPos[Z]);
  glScalef(svs->topScale, svs->topScale, svs->topScale);
  renderShadowVolumeBase(svs);
  glPopMatrix();
}

static void
validateShadowVolume(RTSscene * scene, RTSlight * light,
  RTSobject * object, ShadowVolumeState * svs)
{
  /* Serial number mismatch indicates light or object has changed since last
     shadow volume generation. If mismatch, regenerate the shadow volume. */
  if (light->sernum != svs->lightSernum
    || object->sernum != svs->objectSernum) {
    TessellationContext *workContext;
#ifdef MP
    int i;

    WAIT(contextAvailable);

    LOCK(accessQueue);
    workContext = NULL;
    for (i=0; i<NUM_CONTEXTS; i++) {
      if (context[i]->state == CS_UNUSED) {
        workContext = context[i];
	break;
      }
    }
    assert(workContext);
    workContext->state = CS_CAPTURING;
    UNLOCK(accessQueue);

    captureLightView(scene, light,
      object, svs, workContext);
    
    workContext->state = CS_QUEUED;
    SIGNAL(silhouetteNeedsGeneration);

#else
    workContext = context[0];
    captureLightView(scene, light,
      object, svs, workContext);

    generateSilhouette(workContext);
#endif
  }
}

void
rtsRenderScene(
  RTSscene * scene,
  RTSmode mode)
{
  static GLfloat totalDarkness[4] =
  {0.0, 0.0, 0.0, 0.0};
  int i, obj, bit;
  int numStencilBits, numCastingLights, numShadowingObjects;
  RTSlight *firstLight, *prevLight, *light;
  RTSobject *object;
  GLbitfield fullStencilMask;

  /* Expect application (caller) to do the glClear (including stencil). */
  /* Expect application (caller) to enable depth testing. */

  if (mode != RTS_NO_SHADOWS) {
    /* Validate shadow volumes, count casting lights, and stash the first
       light. */
    numCastingLights = 0;
    firstLight = NULL;
    for (i = 0; i < scene->lightListSize; i++) {
      light = scene->lightList[i];
      if (light) {
        if (light->state != RTS_OFF) {
          if (light->state == RTS_SHINING_AND_CASTING) {

            if (numCastingLights == 0) {
              /* Count number of shadowing objects. */
              numShadowingObjects = 0;
              for (obj = 0; obj < light->objectListSize; obj++) {
                if (light->objectList[obj]->state == RTS_SHADOWING) {
                  numShadowingObjects++;
                }
              }
              if (numShadowingObjects == 0) {
                /* Not casting on any object; skip it. */
                continue;
              }
              assert(firstLight == NULL);
              firstLight = light;
            }
            numCastingLights++;
            if (numCastingLights == 1 || hasBlendSubtract) {
              glEnable(light->glLight);
            } else {
              glDisable(light->glLight);
            }

            for (obj = 0; obj < light->objectListSize; obj++) {
              object = light->objectList[obj];
              if (object->state == RTS_SHADOWING) {
                ShadowVolumeState *svs;

                svs = &light->shadowVolumeList[obj];
                validateShadowVolume(scene, light, object, svs);
              }
            }
          } else if (light->state == RTS_SHINING_AND_CASTING) {
            glEnable(light->glLight);
          }
        } else {
          glDisable(light->glLight);
        }
      }
    }
  }
  glEnable(GL_LIGHTING);
  glEnable(GL_CULL_FACE);
  if (scene->stencilRenderingInvariantHack) {
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  } else {
    /* XXX Note that the non-hack case does not enable or
       disable stencil.  The hack case enables stencil
       testing but sets up the stencil modes so that stencil
       testing is effectively disabled.  If you wanted
       stencil testing on during the renderSceneFunc, you won't
       need to have the hack enabled though! */
  }
  scene->renderSceneFunc(GL_NONE, scene->sceneData, scene);

  if (mode == RTS_NO_SHADOWS) {
    return;
  }
  if (numCastingLights == 0) {
    /* No lights, no shadows. */
    return;
  }
  assert(firstLight);
  assert(numShadowingObjects > 0);

  /* Determine exactly which stencil bits usable for shadowing. */
  if (scene->stencilValidateNeeded) {
    GLbitfield shadowStencilBits;

    shadowStencilBits = scene->usableStencilBits & ((1 << scene->stencilBits) - 1);
    scene->numStencilBits = listBits(shadowStencilBits, scene->bitList);
    if (scene->numStencilBits == 0) {
      fprintf(stderr,
        "WARNING: No stencil bits available for shadowing, expect bad results.\n");
      fprintf(stderr,
        "         Frame buffer stencil bits = %d, usable stencil bits = 0x%x.\n",
        scene->stencilBits, scene->usableStencilBits);
    }
    scene->stencilValidateNeeded = 0;
  }
  numStencilBits = scene->numStencilBits;

  /* The first light is easier than the rest since we need subtractive
     blending for two or more lights. Do the first light the fast way. */

  bit = 0;
  assert(scene->stencilValidateNeeded == 0);

  glDisable(firstLight->glLight);
  glEnable(GL_STENCIL_TEST);
  glDepthMask(GL_FALSE);

  obj = 0;
  while (firstLight->objectList[obj]->state == RTS_NOT_SHADOWING) {
    obj++;
  }

  do {
    assert(bit < numStencilBits);
    assert(firstLight->objectList[obj]->state == RTS_SHADOWING);
    assert(obj < firstLight->objectListSize);

    fullStencilMask = 0;

    glDisable(GL_LIGHTING);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_CULL_FACE);
    glStencilFunc(GL_ALWAYS, 0, 0);
    do {

#ifdef MP
      waitForSilhouetteGenerationDone(scene, &firstLight->shadowVolumeList[obj]);
#endif

      if (hasVertexArray) {
#if defined(GL_VERSION_1_1)
        glInterleavedArrays(GL_V3F, 0,
          firstLight->shadowVolumeList[obj].silhouette);
#elif defined(GL_EXT_vertex_array)
        setupVertexArray(&firstLight->shadowVolumeList[obj], 3);
#endif
      }
      fullStencilMask |= 1 << scene->bitList[bit];
      glStencilMask(1 << scene->bitList[bit]);
      glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
      renderShadowVolume(&firstLight->shadowVolumeList[obj],
        firstLight->lightPos);

      glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
      renderShadowVolumeTop(&firstLight->shadowVolumeList[obj],
        firstLight->lightPos);

      bit++;
      do {
        obj++;
      } while (obj < firstLight->objectListSize
        && firstLight->objectList[obj]->state == RTS_NOT_SHADOWING);

    } while (bit < numStencilBits && obj < firstLight->objectListSize);

    glEnable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(GL_EQUAL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_NOTEQUAL, 0, fullStencilMask);
    glEnable(GL_LIGHTING);
    scene->renderSceneFunc(firstLight->glLight, scene->sceneData, scene);
    if (obj < firstLight->objectListSize) {
      glStencilMask(~0);
      glClear(GL_STENCIL_BUFFER_BIT);
      glDepthFunc(GL_LESS);  /* XXX needed? */
      bit = 0;
    }
  } while (obj < firstLight->objectListSize);

  if (numCastingLights == 1) {
    glStencilMask(~0);
    glCullFace(GL_BACK);  /* XXX Needed? */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    if (scene->stencilRenderingInvariantHack) {
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    } else {
      glDisable(GL_STENCIL_TEST);
    }
    if (hasVertexArray) {
#if defined(GL_VERSION_1_1)
      glDisableClientState(GL_VERTEX_ARRAY);
#elif defined(GL_EXT_vertex_array)
      glDisable(GL_VERTEX_ARRAY_EXT);
#endif
    }
    return;
  }
  /* Get ready to subtract out the particular contribution for each light
     source in regions shadowed by the light source's shadowing objects. */
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, totalDarkness);
  glDepthFunc(GL_LESS);
#ifdef GL_EXT_blend_subtract
  if (hasBlendSubtract) {
    glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
  }
#endif
  glBlendFunc(GL_ONE, GL_ONE);
  glEnable(GL_BLEND);

  prevLight = firstLight;

  for (i = 1; i < scene->lightListSize; i++) {
    light = scene->lightList[i];
    if (light) {
      if (light->state == RTS_SHINING_AND_CASTING) {

        /* Count number of shadowing objects. */
        numShadowingObjects = 0;
        for (obj = 0; obj < light->objectListSize; obj++) {
          if (light->objectList[obj]->state == RTS_SHADOWING) {
            numShadowingObjects++;
          }
        }

        if (numShadowingObjects > 0) {
          int reservedStencilBit;

          assert(scene->stencilValidateNeeded == 0);

          /* Switch off the last light; switch on the current light (all
             other lights should be disabled). */
          glDisable(prevLight->glLight);
          glEnable(light->glLight);

          /* Complicated logic to try to figure out the stencil clear
             strategy.  Tries hard to conserve stencil bit planes and scene
             re-renders. */
          if (numStencilBits < numShadowingObjects) {
            if (numStencilBits == 1) {
              fprintf(stderr, "WARNING: 1 bit of stencil not enough to reserve a bit.\n");
              fprintf(stderr, "         Skipping lights beyond the first.\n");
              continue;
            }
            /* Going to require one or more stencil clears; this requires
               reserving a bit of stencil to avoid double subtracts. */
            reservedStencilBit = 1 << scene->bitList[0];
            bit = 1;
            glStencilMask(~0);
            glClear(GL_STENCIL_BUFFER_BIT);
            glDepthFunc(GL_LESS);  /* XXX Needed? */
          } else {
            /* Faster cases.  All the objects can be rendered each to a
               distinct available stencil plane.  No need to reserve a
               stencil bit to avoid double blending since only one scene
               render required. */
            reservedStencilBit = 0;
            if (numShadowingObjects <= numStencilBits - bit) {
              /* Best case:  Enough stencil bits available to not even
                 require a stencil clear for this light.  Keep "bit" as is. */
            } else {
              /* Not enough left over bitplanes to subtract out this light
                 with what's currently available, so clear the stencil buffer
                 to get enough. */
              glStencilMask(~0);
              glClear(GL_STENCIL_BUFFER_BIT);
              bit = 0;
            }
          }

          obj = 0;
          while (light->objectList[obj]->state == RTS_NOT_SHADOWING) {
            obj++;
          }

          do {
            assert(bit < numStencilBits);
            assert(light->objectList[obj]->state == RTS_SHADOWING);
            assert(obj < light->objectListSize);

            fullStencilMask = reservedStencilBit;

            glDisable(GL_LIGHTING);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glStencilFunc(GL_ALWAYS, 0, 0);
            glDisable(GL_CULL_FACE);
            do {

#ifdef MP
              waitForSilhouetteGenerationDone(scene,
	        &light->shadowVolumeList[obj]);
#endif

              if (hasVertexArray) {
#if defined(GL_VERSION_1_1)
                glInterleavedArrays(GL_V3F, 0,
                  light->shadowVolumeList[obj].silhouette);
#elif defined(GL_EXT_vertex_array)
                setupVertexArray(&light->shadowVolumeList[obj], 3);
#endif
              }
              fullStencilMask |= 1 << scene->bitList[bit];
              glStencilMask(1 << scene->bitList[bit]);
              glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
              renderShadowVolume(&light->shadowVolumeList[obj],
                light->lightPos);

              glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
              renderShadowVolumeTop(&light->shadowVolumeList[obj],
                light->lightPos);

              bit++;
              do {
                obj++;
              } while (obj < light->objectListSize
                && light->objectList[obj]->state == RTS_NOT_SHADOWING);

            } while (bit < scene->numStencilBits && obj < light->objectListSize);

            glEnable(GL_CULL_FACE);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthFunc(GL_EQUAL);
            if (reservedStencilBit) {
              glStencilMask(reservedStencilBit);
              glStencilOp(GL_KEEP, GL_KEEP, GL_ONE);
              if (hasBlendSubtract) {
                /* Subtract lighting contribution inside of shadow; prevent
                   double drawing via stencil */
                glStencilFunc(GL_GREATER, reservedStencilBit, fullStencilMask);
              } else {
                /* Add lighting contribution outside of shadow; prevent
                   double drawing via stencil. */
                glStencilFunc(GL_EQUAL, 0, fullStencilMask);
              }
            } else {
              if (hasBlendSubtract) {
                glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
                glStencilFunc(GL_NOTEQUAL, 0, fullStencilMask);
              } else {
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                glStencilFunc(GL_EQUAL, 0, fullStencilMask);
              }
            }
            glEnable(GL_LIGHTING);
            scene->renderSceneFunc(light->glLight, scene->sceneData, scene);

            if (obj < light->objectListSize) {
              assert(reservedStencilBit);
              glStencilMask(~0);
              glClear(GL_STENCIL_BUFFER_BIT);
              glDepthFunc(GL_LESS);  /* XXX Needed? */
              bit = 1;
            }
          } while (obj < light->objectListSize);

          prevLight = light;
        }
      }
    }
  }

  glStencilMask(~0);
  glCullFace(GL_BACK);  /* XXX needed? */
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
  if (scene->stencilRenderingInvariantHack) {
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  } else {
    glDisable(GL_STENCIL_TEST);
  }
  glDisable(GL_BLEND);
  if (hasVertexArray) {
#if defined(GL_VERSION_1_1)
    glDisableClientState(GL_VERTEX_ARRAY);
#elif defined(GL_EXT_vertex_array)
    glDisable(GL_VERTEX_ARRAY_EXT);
#endif
  }
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, scene->sceneAmbient);
}

void
rtsRenderSilhouette(
  RTSscene * scene,
  RTSlight * light,
  RTSobject * object)
{
  GLfloat lightDelta[3];
  GLfloat lightDistance, viewScale, fieldOfViewRatio, extentScale;
  ShadowVolumeState svsRec, *svs;
  int obj;
  int anonymousShadowVolumeState;

  /* Calculate the light's distance from the object being shadowed. */
  lightDelta[X] = object->objectPos[X] - light->lightPos[X];
  lightDelta[Y] = object->objectPos[Y] - light->lightPos[Y];
  lightDelta[Z] = object->objectPos[Z] - light->lightPos[Z];
  lightDistance = sqrt(lightDelta[X] * lightDelta[X] +
    lightDelta[Y] * lightDelta[Y] + lightDelta[Z] * lightDelta[Z]);

  viewScale = getViewScale(scene);
  fieldOfViewRatio = object->maxRadius / lightDistance;
  extentScale = light->radius * fieldOfViewRatio / viewScale;

  for (obj = 0; obj < light->objectListSize; obj++) {
    if (light->objectList[obj] == object) {
      svs = &light->shadowVolumeList[obj];
      anonymousShadowVolumeState = 0;
      goto gotShadowVolumeState;
    }
  }

  /* It probably makes sense to have the object on the light's object list
     already since then we would have a ShadowVolumeState structure ready to
     use and likely to have a reasonably sized silhouette vertex array. Plus,
     we'd validate the light and object's shadow volume.

     Anyway, rtsRenderSilhouette will still handle the case where the object
     is not already added to the specified light for generality (but not
     economy).  Use an "anonymous" ShadowVolumeState data structure that only
     lives during this routine. */

  svs = &svsRec;
  anonymousShadowVolumeState = 1;
  initShadowVolumeState(svs);

gotShadowVolumeState:

  validateShadowVolume(scene, light, object, svs);

  glPushAttrib(GL_ENABLE_BIT);
  /* Disable a few things likely to screw up the rendering of  the
     silhouette. */
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
#if 0
  glDisable(GL_STENCIL_TEST);
#else
  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
#endif
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(-viewScale, viewScale, -viewScale, viewScale);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glScalef(1.0 / extentScale, 1.0 / extentScale, 1.0 / extentScale);

  renderSilhouette(svs);

#if 0
  glColor3f(0.0, 1.0, 0.0);
  glPointSize(7.0);
  glBegin(GL_POINTS);
  glVertex2fv(eyeLoc);
  glEnd();
#endif

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();

  if (anonymousShadowVolumeState) {
    /* Deallocate "anonymous" ShadowVolumeState's silhouette vertex array. */
    SHARED_FREE(svs->silhouette);
  }
}

void
rtsStencilRenderingInvariantHack(RTSscene * scene, GLboolean enableHack)
{
  scene->stencilRenderingInvariantHack = enableHack;
}

/* XXX These free routines are not complete. */

#if 0
static void
freeTessellationContext(TessellationContext *context)
{
  gluDeleteTess(context->tess);
  free(context->feedbackBuffer);
  free(context);
}
#endif

void
rtsFreeScene(
  RTSscene * scene)
{
  int i;

  for (i=0; i < scene->lightListSize; i++) {
    if (scene->lightList[i]) {
      rtsFreeLight(scene->lightList[i]);
    }
  }
  free(scene->lightList);
  free(scene);
}

void
rtsFreeLight(
  RTSlight * light)
{
  int i;

  for (i=0; i<light->sceneListSize; i++) {
    if (light->sceneList[i]) {
      rtsFreeScene(light->sceneList[i]);
    }
  }
  free(light);
}

void
rtsFreeObject(
  RTSobject * object)
{
  free(object);
}

#endif /* GLU_VERSION_1_2 */
