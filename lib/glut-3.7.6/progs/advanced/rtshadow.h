#ifndef __rtshadow_h__
#define __rtshadow_h__

/* Copyright (c) Mark J. Kilgard, 1997, 1998. */

/* This program is freely distributable without licensing fees and is
   provided without guarantee or warrantee expressed or implied.  This
   program is -not- in the public domain. */

/* Real-time Shadowing library, Version 0.96 */

#if defined(_WIN32)

/* Try to avoid including <windows.h> to avoid name space
   pollution, but Win32's <GL/gl.h> needs APIENTRY and
   WINGDIAPI defined properly. */
# if 0
#  define  WIN32_LEAN_AND_MEAN
#  include <windows.h>
# else
   /* This is from Win32's <windef.h> */
#  ifndef APIENTRY
#   if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#    define APIENTRY    __stdcall
#   else
#    define APIENTRY
#   endif
#  endif
#  ifndef CALLBACK
    /* This is from Win32's <winnt.h> */
#   if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
#    define CALLBACK __stdcall
#   else
#    define CALLBACK
#   endif
#  endif
   /* This is from Win32's <wingdi.h> and <winnt.h> */
#  ifndef WINGDIAPI
#   define WINGDIAPI __declspec(dllimport)
#  endif
   /* XXX This is from Win32's <ctype.h> */
#  ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#   define _WCHAR_T_DEFINED
#  endif
# endif

#pragma warning (disable:4244)	/* Disable bogus conversion warnings. */
#pragma warning (disable:4305)  /* VC++ 5.0 version of above warning. */

#endif

#include <GL/gl.h>
#include <GL/glu.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  RTS_ERROR_OUT_OF_MEMORY,
  RTS_WARNING_EYE_IN_SHADOW,
  RTS_WARNING_LIGHT_TOO_CLOSE
};

typedef enum {
  RTS_OFF,
  RTS_SHINING,
  RTS_SHINING_AND_CASTING
} RTSlightState;

typedef enum {
  RTS_NOT_SHADOWING,
  RTS_SHADOWING
} RTSobjectState;

typedef enum {
  RTS_NO_SHADOWS,
  RTS_USE_SHADOWS,
  RTS_USE_SHADOWS_NO_OVERLAP
} RTSmode;

typedef struct RTSscene RTSscene;
typedef struct RTSlight RTSlight;
typedef struct RTSobject RTSobject;

typedef void (*RTSerrorHandler)(int error, char *message);
typedef void (*RTSrenderSceneFunc)(GLenum castingLight, void *sceneData, RTSscene *scene);

extern RTSscene *rtsCreateScene(
  GLfloat eyePos[3],
  GLbitfield usableStencilBits,
  RTSrenderSceneFunc func,
  void *sceneData);
extern RTSlight *rtsCreateLight(
  GLenum glLight,
  GLfloat lightPos[3],
  GLfloat radius);
extern RTSobject *rtsCreateObject(
  GLfloat objectPos[3],
  GLfloat maxRadius,
  void (*renderObject) (void *objectData),
  void *objectData,
  int feedbackBufferSizeGuess);

extern void rtsAddLightToScene(
  RTSscene * scene,
  RTSlight * light);
extern void rtsAddObjectToLight(
  RTSlight * light,
  RTSobject * object);

extern void rtsRemoveLightFromScene(
  RTSscene * scene,
  RTSlight * light);
extern void rtsRemoveObjectFromLight(
  RTSlight * light,
  RTSobject * object);

extern void rtsSetLightState(
  RTSlight * light,
  RTSlightState state);
extern void rtsSetObjectState(
  RTSobject * object,
  RTSobjectState state);

extern void rtsUpdateEyePos(
  RTSscene * scene,
  GLfloat eyePos[3]);
extern void rtsUpdateUsableStencilBits(
  RTSscene * scene,
  GLbitfield usableStencilBits);

extern void rtsUpdateLightPos(
  RTSlight * light,
  GLfloat lightPos[3]);
extern void rtsUpdateLightRadius(
  RTSlight * light,
  GLfloat lightRadius);

extern void rtsUpdateObjectPos(
  RTSobject * object,
  GLfloat objectPos[3]);
extern void rtsUpdateObjectShape(
  RTSobject * object);
extern void rtsUpdateObjectMaxRadius(
  RTSobject * object,
  GLfloat maxRadius);

extern void rtsFreeScene(
  RTSscene * scene);
extern void rtsFreeLight(
  RTSlight * light);
extern void rtsFreeObject(
  RTSobject * object);

extern int rtsTriviallyOutsideShadowVolume(
  RTSscene * scene,
  GLfloat objectPos[3],
  GLfloat maxRadius);

extern void rtsRenderScene(
  RTSscene * scene,
  RTSmode mode);

extern void rtsRenderSilhouette(
  RTSscene * scene,
  RTSlight * light,
  RTSobject * object);

extern RTSerrorHandler rtsSetErrorHandler(
  RTSerrorHandler handler);

extern void rtsStencilRenderingInvariantHack(
  RTSscene * scene,
  GLboolean enableHack);

#ifdef __cplusplus
}

#endif
#endif /* __rtshadow_h__ */
