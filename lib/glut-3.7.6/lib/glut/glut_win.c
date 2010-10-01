
/* Copyright (c) Mark J. Kilgard, 1994, 1997.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#if !defined(_WIN32)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#include "glutint.h"

GLUTwindow *__glutCurrentWindow = NULL;
GLUTwindow **__glutWindowList = NULL;
int __glutWindowListSize = 0;
#if !defined(_WIN32)
GLUTstale *__glutStaleWindowList = NULL;
#endif
GLUTwindow *__glutMenuWindow = NULL;

void (*__glutFreeOverlayFunc) (GLUToverlay *);
XVisualInfo *(*__glutDetermineVisualFromString) (char *string, Bool * treatAsSingle,
  Criterion * requiredCriteria, int nRequired, int requiredMask, void** fbc) = NULL;

static Criterion requiredWindowCriteria[] =
{
  {LEVEL, EQ, 0},
  {TRANSPARENT, EQ, 0}
};
static int numRequiredWindowCriteria = sizeof(requiredWindowCriteria) / sizeof(Criterion);
static int requiredWindowCriteriaMask = (1 << LEVEL) | (1 << TRANSPARENT);

static void
cleanWindowWorkList(GLUTwindow * window)
{
  GLUTwindow **pEntry = &__glutWindowWorkList;
  GLUTwindow *entry = __glutWindowWorkList;

  /* Tranverse singly-linked window work list look for the
     window. */
  while (entry) {
    if (entry == window) {
      /* Found it; delete it. */
      *pEntry = entry->prevWorkWin;
      return;
    } else {
      pEntry = &entry->prevWorkWin;
      entry = *pEntry;
    }
  }
}

#if !defined(_WIN32)

static void
cleanStaleWindowList(GLUTwindow * window)
{
  GLUTstale **pEntry = &__glutStaleWindowList;
  GLUTstale *entry = __glutStaleWindowList;

  /* Tranverse singly-linked stale window list look for the
     window ID. */
  while (entry) {
    if (entry->window == window) {
      /* Found it; delete it. */
      *pEntry = entry->next;
      free(entry);
      return;
    } else {
      pEntry = &entry->next;
      entry = *pEntry;
    }
  }
}

#endif

static GLUTwindow *__glutWindowCache = NULL;

GLUTwindow *
__glutGetWindow(Window win)
{
  int i;

  /* Does win belong to the last window ID looked up? */
  if (__glutWindowCache && (win == __glutWindowCache->win ||
      (__glutWindowCache->overlay && win ==
        __glutWindowCache->overlay->win))) {
    return
      __glutWindowCache;
  }
  /* Otherwise scan the window list looking for the window ID. */
  for (i = 0; i < __glutWindowListSize; i++) {
    if (__glutWindowList[i]) {
      if (win == __glutWindowList[i]->win) {
        __glutWindowCache = __glutWindowList[i];
        return __glutWindowCache;
      }
      if (__glutWindowList[i]->overlay) {
        if (win == __glutWindowList[i]->overlay->win) {
          __glutWindowCache = __glutWindowList[i];
          return __glutWindowCache;
        }
      }
    }
  }
#if !defined(_WIN32)
  {
    GLUTstale *entry;

    /* Scan through destroyed overlay window IDs for which no
       DestroyNotify has yet been received. */
    for (entry = __glutStaleWindowList; entry; entry = entry->next) {
      if (entry->win == win)
        return entry->window;
    }
  }
#endif
  return NULL;
}

/* CENTRY */
int APIENTRY
glutGetWindow(void)
{
  if (__glutCurrentWindow) {
    return __glutCurrentWindow->num + 1;
  } else {
    return 0;
  }
}
/* ENDCENTRY */

void
__glutSetWindow(GLUTwindow * window)
{
  /* It is tempting to try to short-circuit the call to
     glXMakeCurrent if we "know" we are going to make current
     to a window we are already current to.  In fact, this
     assumption breaks when GLUT is expected to integrated with
     other OpenGL windowing APIs that also make current to
     OpenGL contexts.  Since glXMakeCurrent short-circuits the
     "already bound" case, GLUT avoids the temptation to do so
     too. */
  __glutCurrentWindow = window;

  MAKE_CURRENT_LAYER(__glutCurrentWindow);

#if !defined(_WIN32)
  /* We should be careful to force a finish between each
     iteration through the GLUT main loop if indirect OpenGL 
     contexts are in use; indirect contexts tend to have  much
     longer latency because lots of OpenGL extension requests
     can queue up in the X protocol stream.  We accomplish this
     by posting GLUT_FINISH_WORK to be done. */
  if (!__glutCurrentWindow->isDirect)
    __glutPutOnWorkList(__glutCurrentWindow, GLUT_FINISH_WORK);
#endif

  /* If debugging is enabled, we'll want to check this window
     for any OpenGL errors every iteration through the GLUT
     main loop.  To accomplish this, we post the
     GLUT_DEBUG_WORK to be done on this window. */
  if (__glutDebug) {
    __glutPutOnWorkList(__glutCurrentWindow, GLUT_DEBUG_WORK);
  }
}

/* CENTRY */
void APIENTRY
glutSetWindow(int win)
{
  GLUTwindow *window;

  if (win < 1 || win > __glutWindowListSize) {
    __glutWarning("glutSetWindow attempted on bogus window.");
    return;
  }
  window = __glutWindowList[win - 1];
  if (!window) {
    __glutWarning("glutSetWindow attempted on bogus window.");
    return;
  }
  __glutSetWindow(window);
}
/* ENDCENTRY */

static int
getUnusedWindowSlot(void)
{
  int i;

  /* Look for allocated, unused slot. */
  for (i = 0; i < __glutWindowListSize; i++) {
    if (!__glutWindowList[i]) {
      return i;
    }
  }
  /* Allocate a new slot. */
  __glutWindowListSize++;
  if (__glutWindowList) {
    __glutWindowList = (GLUTwindow **)
      realloc(__glutWindowList,
      __glutWindowListSize * sizeof(GLUTwindow *));
  } else {
    /* XXX Some realloc's do not correctly perform a malloc
       when asked to perform a realloc on a NULL pointer,
       though the ANSI C library spec requires this. */
    __glutWindowList = (GLUTwindow **)
      malloc(sizeof(GLUTwindow *));
  }
  if (!__glutWindowList)
    __glutFatalError("out of memory.");
  __glutWindowList[__glutWindowListSize - 1] = NULL;
  return __glutWindowListSize - 1;
}

static XVisualInfo *
getVisualInfoCI(unsigned int mode)
{
  static int bufSizeList[] =
  {16, 12, 8, 4, 2, 1, 0};
  XVisualInfo *vi;
  int list[32];
  int i, n = 0;

  /* Should not be looking at display mode mask if
     __glutDisplayString is non-NULL. */
  assert(!__glutDisplayString);

  list[n++] = GLX_BUFFER_SIZE;
  list[n++] = 1;
  if (GLUT_WIND_IS_DOUBLE(mode)) {
    list[n++] = GLX_DOUBLEBUFFER;
  }
  if (GLUT_WIND_IS_STEREO(mode)) {
    list[n++] = GLX_STEREO;
  }
  if (GLUT_WIND_HAS_DEPTH(mode)) {
    list[n++] = GLX_DEPTH_SIZE;
    list[n++] = 1;
  }
  if (GLUT_WIND_HAS_STENCIL(mode)) {
    list[n++] = GLX_STENCIL_SIZE;
    list[n++] = 1;
  }
  list[n] = (int) None; /* terminate list */

  /* glXChooseVisual specify GLX_BUFFER_SIZE prefers the
     "smallest index buffer of at least the specified size".
     This would be reasonable if GLUT allowed the user to
     specify the required buffe size, but GLUT's display mode
     is too simplistic (easy to use?). GLUT should try to find
     the "largest".  So start with a large buffer size and
     shrink until we find a matching one that exists. */

  for (i = 0; bufSizeList[i]; i++) {
    /* XXX Assumes list[1] is where GLX_BUFFER_SIZE parameter
       is. */
    list[1] = bufSizeList[i];
    vi = glXChooseVisual(__glutDisplay,
      __glutScreen, list);
    if (vi)
      return vi;
  }
  return NULL;
}

static XVisualInfo *
getVisualInfoRGB(unsigned int mode)
{
  int list[32];
  int n = 0;

  /* Should not be looking at display mode mask if
     __glutDisplayString is non-NULL. */
  assert(!__glutDisplayString);

  /* XXX Would a caching mechanism to minize the calls to
     glXChooseVisual? You'd have to reference count
     XVisualInfo* pointers.  Would also have to properly
     interact with glutInitDisplayString. */

  list[n++] = GLX_RGBA;
  list[n++] = GLX_RED_SIZE;
  list[n++] = 1;
  list[n++] = GLX_GREEN_SIZE;
  list[n++] = 1;
  list[n++] = GLX_BLUE_SIZE;
  list[n++] = 1;
  if (GLUT_WIND_HAS_ALPHA(mode)) {
    list[n++] = GLX_ALPHA_SIZE;
    list[n++] = 1;
  }
  if (GLUT_WIND_IS_DOUBLE(mode)) {
    list[n++] = GLX_DOUBLEBUFFER;
  }
  if (GLUT_WIND_IS_STEREO(mode)) {
    list[n++] = GLX_STEREO;
  }
  if (GLUT_WIND_HAS_DEPTH(mode)) {
    list[n++] = GLX_DEPTH_SIZE;
    list[n++] = 1;
  }
  if (GLUT_WIND_HAS_STENCIL(mode)) {
    list[n++] = GLX_STENCIL_SIZE;
    list[n++] = 1;
  }
  if (GLUT_WIND_HAS_ACCUM(mode)) {
    list[n++] = GLX_ACCUM_RED_SIZE;
    list[n++] = 1;
    list[n++] = GLX_ACCUM_GREEN_SIZE;
    list[n++] = 1;
    list[n++] = GLX_ACCUM_BLUE_SIZE;
    list[n++] = 1;
    if (GLUT_WIND_HAS_ALPHA(mode)) {
      list[n++] = GLX_ACCUM_ALPHA_SIZE;
      list[n++] = 1;
    }
  }
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIS_multisample)
  if (GLUT_WIND_IS_MULTISAMPLE(mode)) {
    if (!__glutIsSupportedByGLX("GLX_SGIS_multisample"))
      return NULL;
    list[n++] = GLX_SAMPLES_SGIS;
    /* XXX Is 4 a reasonable minimum acceptable number of
       samples? */
    list[n++] = 4;
  }
#endif
  list[n] = (int) None; /* terminate list */

  return glXChooseVisual(__glutDisplay,
    __glutScreen, list);
}

XVisualInfo *
__glutGetVisualInfo(unsigned int mode)
{
  /* XXX GLUT_LUMINANCE not implemented for GLUT 3.0. */
  if (GLUT_WIND_IS_LUMINANCE(mode))
    return NULL;

  if (GLUT_WIND_IS_RGB(mode))
    return getVisualInfoRGB(mode);
  else
    return getVisualInfoCI(mode);
}

XVisualInfo *
__glutDetermineVisual(
  unsigned int displayMode,
  Bool * treatAsSingle,
  XVisualInfo * (getVisualInfo) (unsigned int))
{
  XVisualInfo *vis;

  /* Should not be looking at display mode mask if
     __glutDisplayString is non-NULL. */
  assert(!__glutDisplayString);

  *treatAsSingle = GLUT_WIND_IS_SINGLE(displayMode);
  vis = getVisualInfo(displayMode);
  if (!vis) {
    /* Fallback cases when can't get exactly what was asked
       for... */
    if (GLUT_WIND_IS_SINGLE(displayMode)) {
      /* If we can't find a single buffered visual, try looking
         for a double buffered visual.  We can treat a double
         buffered visual as a single buffer visual by changing
         the draw buffer to GL_FRONT and treating any swap
         buffers as no-ops. */
      displayMode |= GLUT_DOUBLE;
      vis = getVisualInfo(displayMode);
      *treatAsSingle = True;
    }
    if (!vis && GLUT_WIND_IS_MULTISAMPLE(displayMode)) {
      /* If we can't seem to get multisampling (ie, not Reality
         Engine class graphics!), go without multisampling.  It
         is up to the application to query how many multisamples
         were allocated (0 equals no multisampling) if the
         application is going to use multisampling for more than
         just antialiasing. */
      displayMode &= ~GLUT_MULTISAMPLE;
      vis = getVisualInfo(displayMode);
    }
  }
  return vis;
}

void GLUTCALLBACK
__glutDefaultDisplay(void)
{
  /* XXX Remove the warning after GLUT 3.0. */
  __glutWarning("The following is a new check for GLUT 3.0; update your code.");
  __glutFatalError(
    "redisplay needed for window %d, but no display callback.",
    __glutCurrentWindow->num + 1);
}

void GLUTCALLBACK
__glutDefaultReshape(int width, int height)
{
  GLUToverlay *overlay;

  /* Adjust the viewport of the window (and overlay if one
     exists). */
  MAKE_CURRENT_WINDOW(__glutCurrentWindow);
  glViewport(0, 0, (GLsizei) width, (GLsizei) height);
  overlay = __glutCurrentWindow->overlay;
  if (overlay) {
    MAKE_CURRENT_OVERLAY(overlay);
    glViewport(0, 0, (GLsizei) width, (GLsizei) height);
  }
  /* Make sure we are current to the current layer (application
     should be able to count on the current layer not changing
     unless the application explicitly calls glutUseLayer). */
  MAKE_CURRENT_LAYER(__glutCurrentWindow);
}

XVisualInfo *
__glutDetermineWindowVisual(Bool * treatAsSingle, Bool * visAlloced, void **fbc)
{
  if (__glutDisplayString) {

    /* __glutDisplayString should be NULL except if
       glutInitDisplayString has been called to register a
       different display string.  Calling glutInitDisplayString
       means using a string instead of an integer mask determine
       the visual to use. Using the function pointer variable
       __glutDetermineVisualFromString below avoids linking in
       the code for implementing glutInitDisplayString (ie,
       glut_dstr.o) unless glutInitDisplayString gets called by
       the application. */

    assert(__glutDetermineVisualFromString);
    *visAlloced = False;
    *fbc = NULL;
    return __glutDetermineVisualFromString(__glutDisplayString, treatAsSingle,
      requiredWindowCriteria, numRequiredWindowCriteria, requiredWindowCriteriaMask, fbc);
  } else {
    *visAlloced = True;
    *fbc = NULL;
    return __glutDetermineVisual(__glutDisplayMode,
      treatAsSingle, __glutGetVisualInfo);
  }
}

/* ARGSUSED5 */  /* Only Win32 uses gameMode parameter. */
GLUTwindow *
__glutCreateWindow(GLUTwindow * parent,
  int x, int y, int width, int height, int gameMode)
{
  GLUTwindow *window;
  XSetWindowAttributes wa;
  unsigned long attribMask;
  int winnum;
  int i;
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)
  GLXFBConfigSGIX fbc;
#else
  void *fbc;
#endif

#if defined(_WIN32)
  WNDCLASS wc;
  int style;

  if (!GetClassInfo(GetModuleHandle(NULL), "GLUT", &wc)) {
    __glutOpenWin32Connection(NULL);
  }
#else
  if (!__glutDisplay) {
    __glutOpenXConnection(NULL);
  }
#endif
  if (__glutGameModeWindow) {
    __glutFatalError("cannot create windows in game mode.");
  }
  winnum = getUnusedWindowSlot();
  window = (GLUTwindow *) malloc(sizeof(GLUTwindow));
  if (!window) {
    __glutFatalError("out of memory.");
  }
  window->num = winnum;

#if !defined(_WIN32)
  window->vis = __glutDetermineWindowVisual(&window->treatAsSingle,
    &window->visAlloced, (void**) &fbc);
  if (!window->vis) {
    __glutFatalError(
      "visual with necessary capabilities not found.");
  }
  __glutSetupColormap(window->vis, &window->colormap, &window->cmap);
#else
  window->cmap = 0;
#endif
  window->eventMask = StructureNotifyMask | ExposureMask;

  attribMask = CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask;
  wa.background_pixmap = None;
  wa.border_pixel = 0;
  wa.colormap = window->cmap;
  wa.event_mask = window->eventMask;
  if (parent) {
    if (parent->eventMask & GLUT_HACK_STOP_PROPAGATE_MASK)
      wa.event_mask |= GLUT_HACK_STOP_PROPAGATE_MASK;
    attribMask |= CWDontPropagate;
    wa.do_not_propagate_mask = parent->eventMask & GLUT_DONT_PROPAGATE_FILTER_MASK;
  } else {
    wa.do_not_propagate_mask = 0;
  }

  /* Stash width and height before Win32's __glutAdjustCoords
     possibly overwrites the values. */
  window->width = width;
  window->height = height;
  window->forceReshape = True;
  window->ignoreKeyRepeat = False;

#if defined(_WIN32)
  __glutAdjustCoords(parent ? parent->win : NULL,
    &x, &y, &width, &height);
  if (parent) {
    style = WS_CHILD;
  } else {
    if (gameMode) {
      /* Game mode window should be a WS_POPUP window to
         ensure that the taskbar is hidden by it.  A standard
         WS_OVERLAPPEDWINDOW does not hide the task bar. */
      style = WS_POPUP | WS_MAXIMIZE;
    } else {
      /* A standard toplevel window with borders and such. */
      style = WS_OVERLAPPEDWINDOW;
    }
  }
  window->win = CreateWindow("GLUT", "GLUT",
    WS_CLIPSIBLINGS | WS_CLIPCHILDREN | style,
    x, y, width, height, parent ? parent->win : __glutRoot,
    NULL, GetModuleHandle(NULL), 0);
  window->hdc = GetDC(window->win);
  /* Must set the XHDC for fake glXChooseVisual & fake
     glXCreateContext & fake XAllocColorCells. */
  XHDC = window->hdc;
  window->vis = __glutDetermineWindowVisual(&window->treatAsSingle,
    &window->visAlloced, &fbc);
  if (!window->vis) {
    __glutFatalError(
      "pixel format with necessary capabilities not found.");
  }
  if (!SetPixelFormat(window->hdc,
      ChoosePixelFormat(window->hdc, window->vis),
      window->vis)) {
    __glutFatalError("SetPixelFormat failed during window create.");
  }
  __glutSetupColormap(window->vis, &window->colormap, &window->cmap);
  /* Make sure subwindows get a windowStatus callback. */
  if (parent) {
    PostMessage(parent->win, WM_ACTIVATE, 0, 0);
  }
  window->renderDc = window->hdc;
#else
  window->win = XCreateWindow(__glutDisplay,
    parent == NULL ? __glutRoot : parent->win,
    x, y, width, height, 0,
    window->vis->depth, InputOutput, window->vis->visual,
    attribMask, &wa);
#endif
  window->renderWin = window->win;
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)
  if (fbc) {
    window->ctx = glXCreateContextWithConfigSGIX(__glutDisplay, fbc,
      GLX_RGBA_TYPE_SGIX, None, __glutTryDirect);
  } else
#endif
  {
    window->ctx = glXCreateContext(__glutDisplay, window->vis,
      None, __glutTryDirect);
  }
  if (!window->ctx) {
    __glutFatalError(
      "failed to create OpenGL rendering context.");
  }
  window->renderCtx = window->ctx;
#if !defined(_WIN32)
  window->isDirect = glXIsDirect(__glutDisplay, window->ctx);
  if (__glutForceDirect) {
    if (!window->isDirect)
      __glutFatalError("direct rendering not possible.");
  }
#endif

  window->parent = parent;
  if (parent) {
    window->siblings = parent->children;
    parent->children = window;
  } else {
    window->siblings = NULL;
  }
  window->overlay = NULL;
  window->children = NULL;
  window->display = __glutDefaultDisplay;
  window->reshape = __glutDefaultReshape;
  window->mouse = NULL;
  window->motion = NULL;
  window->passive = NULL;
  window->entry = NULL;
  window->keyboard = NULL;
  window->keyboardUp = NULL;
  window->windowStatus = NULL;
  window->visibility = NULL;
  window->special = NULL;
  window->specialUp = NULL;
  window->buttonBox = NULL;
  window->dials = NULL;
  window->spaceMotion = NULL;
  window->spaceRotate = NULL;
  window->spaceButton = NULL;
  window->tabletMotion = NULL;
  window->tabletButton = NULL;
#ifdef _WIN32
  window->joystick = NULL;
  window->joyPollInterval = 0;
#endif
  window->tabletPos[0] = -1;
  window->tabletPos[1] = -1;
  window->shownState = 0;
  window->visState = -1;  /* not VisibilityUnobscured,
                             VisibilityPartiallyObscured, or
                             VisibilityFullyObscured */
  window->entryState = -1;  /* not EnterNotify or LeaveNotify */

  window->desiredConfMask = 0;
  window->buttonUses = 0;
  window->cursor = GLUT_CURSOR_INHERIT;

  /* Setup window to be mapped when glutMainLoop starts. */
  window->workMask = GLUT_MAP_WORK;
#ifdef _WIN32
  if (gameMode) {
    /* When mapping a game mode window, just show
       the window.  We have already created the game
       mode window with a maximize flag at creation
       time.  Doing a ShowWindow(window->win, SW_SHOWNORMAL)
       would be wrong for a game mode window since it
       would unmaximize the window. */
    window->desiredMapState = GameModeState;
  } else {
    window->desiredMapState = NormalState;
  }
#else
  window->desiredMapState = NormalState;
#endif
  window->prevWorkWin = __glutWindowWorkList;
  __glutWindowWorkList = window;

  /* Initially, no menus attached. */
  for (i = 0; i < GLUT_MAX_MENUS; i++) {
    window->menu[i] = 0;
  }

  /* Add this new window to the window list. */
  __glutWindowList[winnum] = window;

  /* Make the new window the current window. */
  __glutSetWindow(window);

  __glutDetermineMesaSwapHackSupport();

  if (window->treatAsSingle) {
    /* We do this because either the window really is single
       buffered (in which case this is redundant, but harmless,
       because this is the initial single-buffered context
       state); or we are treating a double buffered window as a
       single-buffered window because the system does not appear
       to export any suitable single- buffered visuals (in which
       the following are necessary). */
    glDrawBuffer(GL_FRONT);
    glReadBuffer(GL_FRONT);
  }
  #ifdef WIN32
  if (gameMode) {
	  glutFullScreen();
  }
  #endif
  return window;
}

/* CENTRY */
int APIENTRY
glutCreateWindow(const char *title)
{
  static int firstWindow = 1;
  GLUTwindow *window;
#if !defined(_WIN32)
  XWMHints *wmHints;
#endif
  Window win;
  XTextProperty textprop;

  if (__glutGameModeWindow) {
    __glutFatalError("cannot create windows in game mode.");
  }
  window = __glutCreateWindow(NULL,
    __glutSizeHints.x, __glutSizeHints.y,
    __glutInitWidth, __glutInitHeight,
    /* not game mode */ 0);
  win = window->win;
  /* Setup ICCCM properties. */
  textprop.value = (unsigned char *) title;
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = strlen(title);
#if defined(_WIN32)
  SetWindowText(win, title);
  if (__glutIconic) {
    window->desiredMapState = IconicState;
  }
#else
  wmHints = XAllocWMHints();
  wmHints->initial_state =
    __glutIconic ? IconicState : NormalState;
  wmHints->flags = StateHint;
  XSetWMProperties(__glutDisplay, win, &textprop, &textprop,
  /* Only put WM_COMMAND property on first window. */
    firstWindow ? __glutArgv : NULL,
    firstWindow ? __glutArgc : 0,
    &__glutSizeHints, wmHints, NULL);
  XFree(wmHints);
  XSetWMProtocols(__glutDisplay, win, &__glutWMDeleteWindow, 1);
#endif
  firstWindow = 0;
  return window->num + 1;
}

#ifdef _WIN32
int APIENTRY
__glutCreateWindowWithExit(const char *title, void (__cdecl *exitfunc)(int))
{
  __glutExitFunc = exitfunc;
  return glutCreateWindow(title);
}
#endif

int APIENTRY
glutCreateSubWindow(int win, int x, int y, int width, int height)
{
  GLUTwindow *window;

  window = __glutCreateWindow(__glutWindowList[win - 1],
    x, y, width, height, /* not game mode */ 0);
#if !defined(_WIN32)
  {
    GLUTwindow *toplevel;

    toplevel = __glutToplevelOf(window);
    if (toplevel->cmap != window->cmap) {
      __glutPutOnWorkList(toplevel, GLUT_COLORMAP_WORK);
    }
  }
#endif
  return window->num + 1;
}
/* ENDCENTRY */

void
__glutDestroyWindow(GLUTwindow * window,
  GLUTwindow * initialWindow)
{
  GLUTwindow **prev, *cur, *parent, *siblings;

  /* Recursively destroy any children. */
  cur = window->children;
  while (cur) {
    siblings = cur->siblings;
    __glutDestroyWindow(cur, initialWindow);
    cur = siblings;
  }
  /* Remove from parent's children list (only necessary for
     non-initial windows and subwindows!). */
  parent = window->parent;
  if (parent && parent == initialWindow->parent) {
    prev = &parent->children;
    cur = parent->children;
    while (cur) {
      if (cur == window) {
        *prev = cur->siblings;
        break;
      }
      prev = &(cur->siblings);
      cur = cur->siblings;
    }
  }
  /* Unbind if bound to this window. */
  if (window == __glutCurrentWindow) {
    UNMAKE_CURRENT();
    __glutCurrentWindow = NULL;
  }
  /* Begin tearing down window itself. */
  if (window->overlay) {
    __glutFreeOverlayFunc(window->overlay);
  }
  XDestroyWindow(__glutDisplay, window->win);
  glXDestroyContext(__glutDisplay, window->ctx);
  if (window->colormap) {
    /* Only color index windows have colormap data structure. */
    __glutFreeColormap(window->colormap);
  }
  /* NULLing the __glutWindowList helps detect is a window
     instance has been destroyed, given a window number. */
  __glutWindowList[window->num] = NULL;

  /* Cleanup data structures that might contain window. */
  cleanWindowWorkList(window);
#if !defined(_WIN32)
  cleanStaleWindowList(window);
#endif
  /* Remove window from the "get window cache" if it is there. */
  if (__glutWindowCache == window)
    __glutWindowCache = NULL;

  if (window->visAlloced) {
    /* Only free XVisualInfo* gotten from glXChooseVisual. */
    XFree(window->vis);
  }

  if (window == __glutGameModeWindow) {
    /* Destroying the game mode window should implicitly
       have GLUT leave game mode. */
    __glutCloseDownGameMode();
  }

  free(window);
}

/* CENTRY */
void APIENTRY
glutDestroyWindow(int win)
{
  GLUTwindow *window = __glutWindowList[win - 1];

  if (__glutMappedMenu && __glutMenuWindow == window) {
    __glutFatalUsage("destroying menu window not allowed while menus in use");
  }
#if !defined(_WIN32)
  /* If not a toplevel window... */
  if (window->parent) {
    /* Destroying subwindows may change colormap requirements;
       recalculate toplevel window's WM_COLORMAP_WINDOWS
       property. */
    __glutPutOnWorkList(__glutToplevelOf(window->parent),
      GLUT_COLORMAP_WORK);
  }
#endif
  __glutDestroyWindow(window, window);
  XFlush(__glutDisplay);
}
/* ENDCENTRY */

void
__glutChangeWindowEventMask(long eventMask, Bool add)
{
  if (add) {
    /* Add eventMask to window's event mask. */
    if ((__glutCurrentWindow->eventMask & eventMask) !=
      eventMask) {
      __glutCurrentWindow->eventMask |= eventMask;
      __glutPutOnWorkList(__glutCurrentWindow,
        GLUT_EVENT_MASK_WORK);
    }
  } else {
    /* Remove eventMask from window's event mask. */
    if (__glutCurrentWindow->eventMask & eventMask) {
      __glutCurrentWindow->eventMask &= ~eventMask;
      __glutPutOnWorkList(__glutCurrentWindow,
        GLUT_EVENT_MASK_WORK);
    }
  }
}

void APIENTRY
glutDisplayFunc(GLUTdisplayCB displayFunc)
{
  /* XXX Remove the warning after GLUT 3.0. */
  if (!displayFunc)
    __glutFatalError("NULL display callback not allowed in GLUT 3.0; update your code.");
  __glutCurrentWindow->display = displayFunc;
}

void APIENTRY
glutMouseFunc(GLUTmouseCB mouseFunc)
{
  if (__glutCurrentWindow->mouse) {
    if (!mouseFunc) {
      /* Previous mouseFunc being disabled. */
      __glutCurrentWindow->buttonUses--;
      __glutChangeWindowEventMask(
        ButtonPressMask | ButtonReleaseMask,
        __glutCurrentWindow->buttonUses > 0);
    }
  } else {
    if (mouseFunc) {
      /* Previously no mouseFunc, new one being installed. */
      __glutCurrentWindow->buttonUses++;
      __glutChangeWindowEventMask(
        ButtonPressMask | ButtonReleaseMask, True);
    }
  }
  __glutCurrentWindow->mouse = mouseFunc;
}

void APIENTRY
glutMotionFunc(GLUTmotionCB motionFunc)
{
  /* Hack.  Some window managers (4Dwm by default) will mask
     motion events if the client is not selecting for button
     press and release events. So we select for press and
     release events too (being careful to use reference
     counting).  */
  if (__glutCurrentWindow->motion) {
    if (!motionFunc) {
      /* previous mouseFunc being disabled */
      __glutCurrentWindow->buttonUses--;
      __glutChangeWindowEventMask(
        ButtonPressMask | ButtonReleaseMask,
        __glutCurrentWindow->buttonUses > 0);
    }
  } else {
    if (motionFunc) {
      /* Previously no mouseFunc, new one being installed. */
      __glutCurrentWindow->buttonUses++;
      __glutChangeWindowEventMask(
        ButtonPressMask | ButtonReleaseMask, True);
    }
  }
  /* Real work of selecting for passive mouse motion.  */
  __glutChangeWindowEventMask(
    Button1MotionMask | Button2MotionMask | Button3MotionMask,
    motionFunc != NULL);
  __glutCurrentWindow->motion = motionFunc;
}

void APIENTRY
glutPassiveMotionFunc(GLUTpassiveCB passiveMotionFunc)
{
  __glutChangeWindowEventMask(PointerMotionMask,
    passiveMotionFunc != NULL);

  /* Passive motion also requires watching enters and leaves so
     that a fake passive motion event can be generated on an
     enter. */
  __glutChangeWindowEventMask(EnterWindowMask | LeaveWindowMask,
    __glutCurrentWindow->entry != NULL || passiveMotionFunc != NULL);

  __glutCurrentWindow->passive = passiveMotionFunc;
}

void APIENTRY
glutEntryFunc(GLUTentryCB entryFunc)
{
  __glutChangeWindowEventMask(EnterWindowMask | LeaveWindowMask,
    entryFunc != NULL || __glutCurrentWindow->passive);
  __glutCurrentWindow->entry = entryFunc;
  if (!entryFunc) {
    __glutCurrentWindow->entryState = -1;
  }
}

void APIENTRY
glutWindowStatusFunc(GLUTwindowStatusCB windowStatusFunc)
{
  __glutChangeWindowEventMask(VisibilityChangeMask,
    windowStatusFunc != NULL);
  __glutCurrentWindow->windowStatus = windowStatusFunc;
  if (!windowStatusFunc) {
    /* Make state invalid. */
    __glutCurrentWindow->visState = -1;
  }
}

static void GLUTCALLBACK
visibilityHelper(int status)
{
  if (status == GLUT_HIDDEN || status == GLUT_FULLY_COVERED)
    __glutCurrentWindow->visibility(GLUT_NOT_VISIBLE);
  else
    __glutCurrentWindow->visibility(GLUT_VISIBLE);
}

void APIENTRY
glutVisibilityFunc(GLUTvisibilityCB visibilityFunc)
{
  __glutCurrentWindow->visibility = visibilityFunc;
  if (visibilityFunc)
    glutWindowStatusFunc(visibilityHelper);
  else
    glutWindowStatusFunc(NULL);
}

void APIENTRY
glutReshapeFunc(GLUTreshapeCB reshapeFunc)
{
  if (reshapeFunc) {
    __glutCurrentWindow->reshape = reshapeFunc;
  } else {
    __glutCurrentWindow->reshape = __glutDefaultReshape;
  }
}
