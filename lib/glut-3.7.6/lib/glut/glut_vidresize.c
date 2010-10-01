
/* Copyright (c) Mark J. Kilgard, 1996. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>

#if !defined(_WIN32)
#include <GL/glx.h>
#endif

#ifdef __sgi
#include <dlfcn.h>
#endif

#include "glutint.h"

/* Grumble.  The IRIX 6.3 and early IRIX 6.4 OpenGL headers
   support the video resize extension, but failed to define
   GLX_SGIX_video_resize. */
#ifdef GLX_SYNC_FRAME_SGIX
#define GLX_SGIX_video_resize 1
#endif

#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_video_resize)
static int canVideoResize = -1;
static int videoResizeChannel;
#else
static int canVideoResize = 0;
#endif
static int videoResizeInUse = 0;
static int dx = -1, dy = -1, dw = -1, dh = -1;

/* XXX Note that IRIX 6.2, 6.3, and some 6.4 versions have a
   bug where programs seg-fault when they attempt video
   resizing from an indirect OpenGL context (either local or
   over a network). */

#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_video_resize)

static volatile int errorCaught;

/* ARGSUSED */
static
catchXSGIvcErrors(Display * dpy, XErrorEvent * event)
{
  errorCaught = 1;
  return 0;
}
#endif

/* CENTRY */
int APIENTRY 
glutVideoResizeGet(GLenum param)
{
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_video_resize)
  if (canVideoResize < 0) {
    canVideoResize = __glutIsSupportedByGLX("GLX_SGIX_video_resize");
    if (canVideoResize) {
#if __sgi
      /* This is a hack because IRIX 6.2, 6.3, and some 6.4
         versions were released with GLX_SGIX_video_resize
         being advertised by the X server though the video
         resize extension is not actually supported.  We try to
         determine if the libGL.so we are using actually has a
         video resize entrypoint before we try to use the
         feature. */
      void (*func) (void);
      void *glxDso = dlopen("libGL.so", RTLD_LAZY);

      func = (void (*)(void)) dlsym(glxDso, "glXQueryChannelDeltasSGIX");
      if (!func) {
        canVideoResize = 0;
      } else
#endif
      {
        char *channelString;
        int (*handler) (Display *, XErrorEvent *);

        channelString = getenv("GLUT_VIDEO_RESIZE_CHANNEL");
        videoResizeChannel = channelString ? atoi(channelString) : 0;

        /* Work around another annoying problem with SGI's
           GLX_SGIX_video_resize implementation.  Early IRIX
           6.4 OpenGL's advertise the extension and have the
           video resize API, but an XSGIvc X protocol errors
           result trying to use the API.  Set up an error
           handler to intercept what would otherwise be a fatal
           error.  If an error was recieved, do not report that
           video resize is possible. */
        handler = XSetErrorHandler(catchXSGIvcErrors);

        errorCaught = 0;

        glXQueryChannelDeltasSGIX(__glutDisplay, __glutScreen,
          videoResizeChannel, &dx, &dy, &dw, &dh);

        /* glXQueryChannelDeltasSGIX is an inherent X server
           round-trip so we know we will have gotten either the
           correct reply or and error by this time. */
        XSetErrorHandler(handler);

        /* Still yet another work around.  In IRIX 6.4 betas,
           glXQueryChannelDeltasSGIX will return as if it
           succeeded, but the values are filled with junk.
           Watch to make sure the delta variables really make
           sense. */
        if (errorCaught ||
          dx < 0 || dy < 0 || dw < 0 || dh < 0 ||
          dx > 2048 || dy > 2048 || dw > 2048 || dh > 2048) {
          canVideoResize = 0;
        }
      }
    }
  }
#endif /* GLX_SGIX_video_resize */

  switch (param) {
  case GLUT_VIDEO_RESIZE_POSSIBLE:
    return canVideoResize;
  case GLUT_VIDEO_RESIZE_IN_USE:
    return videoResizeInUse;
  case GLUT_VIDEO_RESIZE_X_DELTA:
    return dx;
  case GLUT_VIDEO_RESIZE_Y_DELTA:
    return dy;
  case GLUT_VIDEO_RESIZE_WIDTH_DELTA:
    return dw;
  case GLUT_VIDEO_RESIZE_HEIGHT_DELTA:
    return dh;
  case GLUT_VIDEO_RESIZE_X:
  case GLUT_VIDEO_RESIZE_Y:
  case GLUT_VIDEO_RESIZE_WIDTH:
  case GLUT_VIDEO_RESIZE_HEIGHT:
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_video_resize)
    if (videoResizeInUse) {
      int x, y, width, height;

      glXQueryChannelRectSGIX(__glutDisplay, __glutScreen,
        videoResizeChannel, &x, &y, &width, &height);
      switch (param) {
      case GLUT_VIDEO_RESIZE_X:
        return x;
      case GLUT_VIDEO_RESIZE_Y:
        return y;
      case GLUT_VIDEO_RESIZE_WIDTH:
        return width;
      case GLUT_VIDEO_RESIZE_HEIGHT:
        return height;
      }
    }
#endif
    return -1;
  default:
    __glutWarning("invalid glutVideoResizeGet parameter: %d", param);
    return -1;
  }
}

void APIENTRY 
glutSetupVideoResizing(void)
{
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_video_resize)
  if (glutVideoResizeGet(GLUT_VIDEO_RESIZE_POSSIBLE)) {
    glXBindChannelToWindowSGIX(__glutDisplay, __glutScreen,
      videoResizeChannel, __glutCurrentWindow->win);
    videoResizeInUse = 1;
  } else
#endif
    __glutFatalError("glutEstablishVideoResizing: video resizing not possible.\n");
}

void APIENTRY 
glutStopVideoResizing(void)
{
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_video_resize)
  if (glutVideoResizeGet(GLUT_VIDEO_RESIZE_POSSIBLE)) {
    if (videoResizeInUse) {
      glXBindChannelToWindowSGIX(__glutDisplay, __glutScreen,
        videoResizeChannel, None);
      videoResizeInUse = 0;
    }
  }
#endif
}

/* ARGSUSED */
void APIENTRY 
glutVideoResize(int x, int y, int width, int height)
{
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_video_resize)
  if (videoResizeInUse) {
#ifdef GLX_SYNC_SWAP_SGIX
    /* glXChannelRectSyncSGIX introduced in a patch to IRIX
       6.2; the original unpatched IRIX 6.2 behavior is always
       GLX_SYNC_SWAP_SGIX. */
    glXChannelRectSyncSGIX(__glutDisplay, __glutScreen,
      videoResizeChannel, GLX_SYNC_SWAP_SGIX);
#endif
    glXChannelRectSGIX(__glutDisplay, __glutScreen,
      videoResizeChannel, x, y, width, height);
  }
#endif
}

/* ARGSUSED */
void APIENTRY 
glutVideoPan(int x, int y, int width, int height)
{
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_video_resize)
  if (videoResizeInUse) {
#ifdef GLX_SYNC_FRAME_SGIX
    /* glXChannelRectSyncSGIX introduced in a patch to IRIX
       6.2; the original unpatched IRIX 6.2 behavior is always
       GLX_SYNC_SWAP_SGIX.  We just ignore that we cannot
       accomplish GLX_SYNC_FRAME_SGIX on IRIX unpatched 6.2;
       this means you'd need a glutSwapBuffers to actually
       realize the video resize. */
    glXChannelRectSyncSGIX(__glutDisplay, __glutScreen,
      videoResizeChannel, GLX_SYNC_FRAME_SGIX);
#endif
    glXChannelRectSGIX(__glutDisplay, __glutScreen,
      videoResizeChannel, x, y, width, height);
  }
#endif
}

/* ENDCENTRY */
