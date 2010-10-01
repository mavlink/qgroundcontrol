#ifndef __glutint_h__
#define __glutint_h__

/* Copyright (c) Mark J. Kilgard, 1994, 1997, 1998. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#if defined(__CYGWIN32__)
#include <sys/time.h>
#endif

#define SUPPORT_FORTRAN  /* With GLUT 3.7, everyone supports Fortran. */

#if defined(_WIN32)
#include "glutwin32.h"
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#endif

#define GLUT_BUILDING_LIB  /* Building the GLUT library itself. */

/* GLUT_BUILDING_LIB is used by <GL/glut.h> to 1) not #pragma link
   with the GLUT library, and 2) avoid the Win32 atexit hack. */

#include <GL/glut.h>

#ifndef CDECL
# if defined(_WIN32) && defined(_MSC_VER)
#  define CDECL __cdecl
# else
#  define CDECL
# endif
#endif

/* This must be done after <GL/gl.h> is included.  MESA is defined
   if the <GL/gl.h> is supplied by Brian Paul's Mesa library. */ 
#if defined(MESA) && defined(_WIN32)
/* Mesa implements "wgl" versions of GDI entry points needed for
   using OpenGL.  Map these "wgl" versions to the GDI names via
   macros. */
WINGDIAPI int WINAPI wglChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd);
WINGDIAPI int WINAPI wglDescribePixelFormat(HDC hdc,int iPixelFormat,UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd);
WINGDIAPI int WINAPI wglGetPixelFormat(HDC hdc);
WINGDIAPI BOOL WINAPI wglSetPixelFormat(HDC hdc, int iPixelFormat, PIXELFORMATDESCRIPTOR *ppfd);
WINGDIAPI BOOL WINAPI wglSwapBuffers(HDC hdc);
#define ChoosePixelFormat wglChoosePixelFormat
#define DescribePixelFormat wglDescribePixelFormat
#define GetPixelFormat wglGetPixelFormat
#define SetPixelFormat wglSetPixelFormat
#define SwapBuffers wglSwapBuffers
#endif

/* Non-Win32 platforms need APIENTRY defined to nothing
   because all the GLUT routines have the APIENTRY prefix
   to make Win32 happy. */
#ifndef APIENTRY
#define APIENTRY
#endif

#ifdef SUPPORT_FORTRAN
#include <GL/glutf90.h>
#endif

#ifdef __vms
#if ( __VMS_VER < 70000000 )
struct timeval {
  __int64 val;
};
extern int sys$gettim(struct timeval *);
#else
#include <time.h>
#endif
#else
#include <sys/types.h>
#if !defined(_WIN32)
#include <sys/time.h>
#else
#include <winsock.h>
#endif
#endif
#if defined(__vms) && ( __VMS_VER < 70000000 )

/* For VMS6.2 or lower :
   One TICK on VMS is 100 nanoseconds; 0.1 microseconds or
   0.0001 milliseconds. This means that there are 0.01
   ticks/ns, 10 ticks/us, 10,000 ticks/ms and 10,000,000
   ticks/second. */

#define TICKS_PER_MILLISECOND 10000
#define TICKS_PER_SECOND      10000000

#define GETTIMEOFDAY(_x) (void) sys$gettim (_x);

#define ADD_TIME(dest, src1, src2) { \
  (dest).val = (src1).val + (src2).val; \
}

#define TIMEDELTA(dest, src1, src2) { \
  (dest).val = (src1).val - (src2).val; \
}

#define IS_AFTER(t1, t2) ((t2).val > (t1).val)

#define IS_AT_OR_AFTER(t1, t2) ((t2).val >= (t1).val)

#else
#if defined(SVR4) && !defined(sun)  /* Sun claims SVR4, but
                                       wants 2 args. */
#define GETTIMEOFDAY(_x) gettimeofday(_x)
#else
#define GETTIMEOFDAY(_x) gettimeofday(_x, NULL)
#endif
#define ADD_TIME(dest, src1, src2) { \
  if(((dest).tv_usec = \
    (src1).tv_usec + (src2).tv_usec) >= 1000000) { \
    (dest).tv_usec -= 1000000; \
    (dest).tv_sec = (src1).tv_sec + (src2).tv_sec + 1; \
  } else { \
    (dest).tv_sec = (src1).tv_sec + (src2).tv_sec; \
    if(((dest).tv_sec >= 1) && (((dest).tv_usec <0))) { \
      (dest).tv_sec --;(dest).tv_usec += 1000000; \
    } \
  } \
}
#define TIMEDELTA(dest, src1, src2) { \
  if(((dest).tv_usec = (src1).tv_usec - (src2).tv_usec) < 0) { \
    (dest).tv_usec += 1000000; \
    (dest).tv_sec = (src1).tv_sec - (src2).tv_sec - 1; \
  } else { \
     (dest).tv_sec = (src1).tv_sec - (src2).tv_sec; \
  } \
}
#define IS_AFTER(t1, t2) \
  (((t2).tv_sec > (t1).tv_sec) || \
  (((t2).tv_sec == (t1).tv_sec) && \
  ((t2).tv_usec > (t1).tv_usec)))
#define IS_AT_OR_AFTER(t1, t2) \
  (((t2).tv_sec > (t1).tv_sec) || \
  (((t2).tv_sec == (t1).tv_sec) && \
  ((t2).tv_usec >= (t1).tv_usec)))
#endif

#define IGNORE_IN_GAME_MODE() \
  { if (__glutGameModeWindow) return; }

#define GLUT_WIND_IS_RGB(x)         (((x) & GLUT_INDEX) == 0)
#define GLUT_WIND_IS_INDEX(x)       (((x) & GLUT_INDEX) != 0)
#define GLUT_WIND_IS_SINGLE(x)      (((x) & GLUT_DOUBLE) == 0)
#define GLUT_WIND_IS_DOUBLE(x)      (((x) & GLUT_DOUBLE) != 0)
#define GLUT_WIND_HAS_ACCUM(x)      (((x) & GLUT_ACCUM) != 0)
#define GLUT_WIND_HAS_ALPHA(x)      (((x) & GLUT_ALPHA) != 0)
#define GLUT_WIND_HAS_DEPTH(x)      (((x) & GLUT_DEPTH) != 0)
#define GLUT_WIND_HAS_STENCIL(x)    (((x) & GLUT_STENCIL) != 0)
#define GLUT_WIND_IS_MULTISAMPLE(x) (((x) & GLUT_MULTISAMPLE) != 0)
#define GLUT_WIND_IS_STEREO(x)      (((x) & GLUT_STEREO) != 0)
#define GLUT_WIND_IS_LUMINANCE(x)   (((x) & GLUT_LUMINANCE) != 0)
#define GLUT_MAP_WORK               (1 << 0)
#define GLUT_EVENT_MASK_WORK        (1 << 1)
#define GLUT_REDISPLAY_WORK         (1 << 2)
#define GLUT_CONFIGURE_WORK         (1 << 3)
#define GLUT_COLORMAP_WORK          (1 << 4)
#define GLUT_DEVICE_MASK_WORK	    (1 << 5)
#define GLUT_FINISH_WORK	    (1 << 6)
#define GLUT_DEBUG_WORK		    (1 << 7)
#define GLUT_DUMMY_WORK		    (1 << 8)
#define GLUT_FULL_SCREEN_WORK       (1 << 9)
#define GLUT_OVERLAY_REDISPLAY_WORK (1 << 10)
#define GLUT_REPAIR_WORK            (1 << 11)
#define GLUT_OVERLAY_REPAIR_WORK    (1 << 12)

/* Frame buffer capability macros and types. */
#define RGBA                    0
#define BUFFER_SIZE             1
#define DOUBLEBUFFER            2
#define STEREO                  3
#define AUX_BUFFERS             4
#define RED_SIZE                5  /* Used as mask bit for
                                      "color selected". */
#define GREEN_SIZE              6
#define BLUE_SIZE               7
#define ALPHA_SIZE              8
#define DEPTH_SIZE              9
#define STENCIL_SIZE            10
#define ACCUM_RED_SIZE          11  /* Used as mask bit for
                                       "acc selected". */
#define ACCUM_GREEN_SIZE        12
#define ACCUM_BLUE_SIZE         13
#define ACCUM_ALPHA_SIZE        14
#define LEVEL                   15

#define NUM_GLXCAPS             (LEVEL + 1)

#define XVISUAL                 (NUM_GLXCAPS + 0)
#define TRANSPARENT             (NUM_GLXCAPS + 1)
#define SAMPLES                 (NUM_GLXCAPS + 2)
#define XSTATICGRAY             (NUM_GLXCAPS + 3)  /* Used as
                                                      mask bit
                                                      for "any
                                                      visual type 
                                                      selected". */
#define XGRAYSCALE              (NUM_GLXCAPS + 4)
#define XSTATICCOLOR            (NUM_GLXCAPS + 5)
#define XPSEUDOCOLOR            (NUM_GLXCAPS + 6)
#define XTRUECOLOR              (NUM_GLXCAPS + 7)
#define XDIRECTCOLOR            (NUM_GLXCAPS + 8)
#define SLOW                    (NUM_GLXCAPS + 9)
#define CONFORMANT              (NUM_GLXCAPS + 10)

#define NUM_CAPS                (NUM_GLXCAPS + 11)

/* Frame buffer capablities that don't have a corresponding
   FrameBufferMode entry.  These get used as mask bits. */
#define NUM                     (NUM_CAPS + 0)
#define RGBA_MODE               (NUM_CAPS + 1)
#define CI_MODE                 (NUM_CAPS + 2)
#define LUMINANCE_MODE		(NUM_CAPS + 3)

#define NONE			0
#define EQ			1
#define NEQ			2
#define LTE			3
#define GTE			4
#define GT			5
#define LT			6
#define MIN			7

typedef struct _Criterion {
  int capability;
  int comparison;
  int value;
} Criterion;

typedef struct _FrameBufferMode {
  XVisualInfo *vi;
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)

  /* fbc is non-NULL when the XVisualInfo* is not OpenGL-capable
     (ie, GLX_USE_GL is false), but the SGIX_fbconfig extension shows
     the visual's fbconfig is OpenGL-capable.  The reason for this is typically
     an RGBA luminance fbconfig such as 16-bit StaticGray that could
     not be advertised as a GLX visual since StaticGray visuals are
     required (by the GLX specification) to be color index.  The
     SGIX_fbconfig allows StaticGray visuals to instead advertised as
     fbconfigs that can provide RGBA luminance support. */

  GLXFBConfigSGIX fbc;
#endif
  int valid;
  int cap[NUM_CAPS];
} FrameBufferMode;

/* DisplayMode capability macros for game mode. */
#define DM_WIDTH        0  /* "width" */
#define DM_HEIGHT       1  /* "height" */
#define DM_PIXEL_DEPTH  2  /* "bpp" (bits per pixel) */
#define DM_HERTZ        3  /* "hertz" */
#define DM_NUM          4  /* "num" */

#define NUM_DM_CAPS     (DM_NUM+1)

typedef struct _DisplayMode {
#ifdef _WIN32
  DEVMODE devmode;
#else
  /* XXX The X Window System does not have a standard
     mechanism for display setting changes.  On SGI
     systems, GLUT could use the XSGIvc (SGI X video
     control extension).  Perhaps this can be done in
     a future release of GLUT. */
#endif
  int valid;
  int cap[NUM_DM_CAPS];
} DisplayMode;

/* GLUT  function types */
typedef void (GLUTCALLBACK *GLUTdisplayCB) (void);
typedef void (GLUTCALLBACK *GLUTreshapeCB) (int, int);
typedef void (GLUTCALLBACK *GLUTkeyboardCB) (unsigned char, int, int);
typedef void (GLUTCALLBACK *GLUTmouseCB) (int, int, int, int);
typedef void (GLUTCALLBACK *GLUTmotionCB) (int, int);
typedef void (GLUTCALLBACK *GLUTpassiveCB) (int, int);
typedef void (GLUTCALLBACK *GLUTentryCB) (int);
typedef void (GLUTCALLBACK *GLUTvisibilityCB) (int);
typedef void (GLUTCALLBACK *GLUTwindowStatusCB) (int);
typedef void (GLUTCALLBACK *GLUTidleCB) (void);
typedef void (GLUTCALLBACK *GLUTtimerCB) (int);
typedef void (GLUTCALLBACK *GLUTmenuStateCB) (int);  /* DEPRICATED. */
typedef void (GLUTCALLBACK *GLUTmenuStatusCB) (int, int, int);
typedef void (GLUTCALLBACK *GLUTselectCB) (int);
typedef void (GLUTCALLBACK *GLUTspecialCB) (int, int, int);
typedef void (GLUTCALLBACK *GLUTspaceMotionCB) (int, int, int);
typedef void (GLUTCALLBACK *GLUTspaceRotateCB) (int, int, int);
typedef void (GLUTCALLBACK *GLUTspaceButtonCB) (int, int);
typedef void (GLUTCALLBACK *GLUTdialsCB) (int, int);
typedef void (GLUTCALLBACK *GLUTbuttonBoxCB) (int, int);
typedef void (GLUTCALLBACK *GLUTtabletMotionCB) (int, int);
typedef void (GLUTCALLBACK *GLUTtabletButtonCB) (int, int, int, int);
typedef void (GLUTCALLBACK *GLUTjoystickCB) (unsigned int buttonMask, int x, int y, int z);

typedef struct _GLUTcolorcell GLUTcolorcell;
struct _GLUTcolorcell {
  /* GLUT_RED, GLUT_GREEN, GLUT_BLUE */
  GLfloat component[3];
};

typedef struct _GLUTcolormap GLUTcolormap;
struct _GLUTcolormap {
  Visual *visual;       /* visual of the colormap */
  Colormap cmap;        /* X colormap ID */
  int refcnt;           /* number of windows using colormap */
  int size;             /* number of cells in colormap */
  int transparent;      /* transparent pixel, or -1 if opaque */
  GLUTcolorcell *cells; /* array of cells */
  GLUTcolormap *next;   /* next colormap in list */
};

typedef struct _GLUTwindow GLUTwindow;
typedef struct _GLUToverlay GLUToverlay;
struct _GLUTwindow {
  int num;              /* Small integer window id (0-based). */

  /* Window system related state. */
#if defined(_WIN32)
  int pf;               /* Pixel format. */
  HDC hdc;              /* Window's Win32 device context. */
#endif
  Window win;           /* X window for GLUT window */
  GLXContext ctx;       /* OpenGL context GLUT glut window */
  XVisualInfo *vis;     /* visual for window */
  Bool visAlloced;      /* if vis needs deallocate on destroy */
  Colormap cmap;        /* RGB colormap for window; None if CI */
  GLUTcolormap *colormap;  /* colormap; NULL if RGBA */
  GLUToverlay *overlay; /* overlay; NULL if no overlay */
#if defined(_WIN32)
  HDC renderDc;         /* Win32's device context for rendering. */
#endif
  Window renderWin;     /* X window for rendering (might be
                           overlay) */
  GLXContext renderCtx; /* OpenGL context for rendering (might
                           be overlay) */
  /* GLUT settable or visible window state. */
  int width;            /* window width in pixels */
  int height;           /* window height in pixels */
  int cursor;           /* cursor name */
  int visState;         /* visibility state (-1 is unknown) */
  int shownState;       /* if window mapped */
  int entryState;       /* entry state (-1 is unknown) */
#define GLUT_MAX_MENUS              3

  int menu[GLUT_MAX_MENUS];  /* attatched menu nums */
  /* Window relationship state. */
  GLUTwindow *parent;   /* parent window */
  GLUTwindow *children; /* list of children */
  GLUTwindow *siblings; /* list of siblings */
  /* Misc. non-API visible (hidden) state. */
  Bool treatAsSingle;   /* treat this window as single-buffered
                           (it might be "fake" though) */
  Bool forceReshape;    /* force reshape before display */
#if !defined(_WIN32)
  Bool isDirect;        /* if direct context (X11 only) */
#endif
  Bool usedSwapBuffers; /* if swap buffers used last display */
  long eventMask;       /* mask of X events selected for */
  int buttonUses;       /* number of button uses, ref cnt */
  int tabletPos[2];     /* tablet position (-1 is invalid) */
  /* Work list related state. */
  unsigned int workMask;  /* mask of window work to be done */
  GLUTwindow *prevWorkWin;  /* link list of windows to work on */
  Bool desiredMapState; /* how to mapped window if on map work
                           list */
  Bool ignoreKeyRepeat;  /* if window ignores autorepeat */
  int desiredConfMask;  /* mask of desired window configuration
                         */
  int desiredX;         /* desired X location */
  int desiredY;         /* desired Y location */
  int desiredWidth;     /* desired window width */
  int desiredHeight;    /* desired window height */
  int desiredStack;     /* desired window stack */
  /* Per-window callbacks. */
  GLUTdisplayCB display;  /* redraw */
  GLUTreshapeCB reshape;  /* resize (width,height) */
  GLUTmouseCB mouse;    /* mouse (button,state,x,y) */
  GLUTmotionCB motion;  /* motion (x,y) */
  GLUTpassiveCB passive;  /* passive motion (x,y) */
  GLUTentryCB entry;    /* window entry/exit (state) */
  GLUTkeyboardCB keyboard;  /* keyboard (ASCII,x,y) */
  GLUTkeyboardCB keyboardUp;  /* keyboard up (ASCII,x,y) */
  GLUTwindowStatusCB windowStatus;  /* window status */
  GLUTvisibilityCB visibility;  /* visibility */
  GLUTspecialCB special;  /* special key */
  GLUTspecialCB specialUp;  /* special up key */
  GLUTbuttonBoxCB buttonBox;  /* button box */
  GLUTdialsCB dials;    /* dials */
  GLUTspaceMotionCB spaceMotion;  /* Spaceball motion */
  GLUTspaceRotateCB spaceRotate;  /* Spaceball rotate */
  GLUTspaceButtonCB spaceButton;  /* Spaceball button */
  GLUTtabletMotionCB tabletMotion;  /* tablet motion */
  GLUTtabletButtonCB tabletButton;  /* tablet button */
#ifdef _WIN32
  GLUTjoystickCB joystick;  /* joystick */
  int joyPollInterval; /* joystick polling interval */
#endif
#ifdef SUPPORT_FORTRAN
  GLUTdisplayFCB fdisplay;  /* Fortran display  */
  GLUTreshapeFCB freshape;  /* Fortran reshape  */
  GLUTmouseFCB fmouse;  /* Fortran mouse  */
  GLUTmotionFCB fmotion;  /* Fortran motion  */
  GLUTpassiveFCB fpassive;  /* Fortran passive  */
  GLUTentryFCB fentry;  /* Fortran entry  */
  GLUTkeyboardFCB fkeyboard;  /* Fortran keyboard  */
  GLUTkeyboardFCB fkeyboardUp;  /* Fortran keyboard up */
  GLUTwindowStatusFCB fwindowStatus;  /* Fortran window status */
  GLUTvisibilityFCB fvisibility;  /* Fortran visibility */
  GLUTspecialFCB fspecial;  /* special key */
  GLUTspecialFCB fspecialUp;  /* special key up */
  GLUTbuttonBoxFCB fbuttonBox;  /* button box */
  GLUTdialsFCB fdials;  /* dials */
  GLUTspaceMotionFCB fspaceMotion;  /* Spaceball motion */
  GLUTspaceRotateFCB fspaceRotate;  /* Spaceball rotate */
  GLUTspaceButtonFCB fspaceButton;  /* Spaceball button */
  GLUTtabletMotionFCB ftabletMotion;  /* tablet motion */
  GLUTtabletButtonFCB ftabletButton;  /* tablet button */
#ifdef _WIN32
  GLUTjoystickFCB fjoystick;  /* joystick */
#endif
#endif
};

struct _GLUToverlay {
#if defined(_WIN32)
  int pf;
  HDC hdc;
#endif
  Window win;
  GLXContext ctx;
  XVisualInfo *vis;     /* visual for window */
  Bool visAlloced;      /* if vis needs deallocate on destroy */
  Colormap cmap;        /* RGB colormap for window; None if CI */
  GLUTcolormap *colormap;  /* colormap; NULL if RGBA */
  int shownState;       /* if overlay window mapped */
  Bool treatAsSingle;   /* treat as single-buffered */
#if !defined(_WIN32)
  Bool isDirect;        /* if direct context */
#endif
  int transparentPixel; /* transparent pixel value */
  GLUTdisplayCB display;  /* redraw  */
#ifdef SUPPORT_FORTRAN
  GLUTdisplayFCB fdisplay;  /* redraw  */
#endif
};

typedef struct _GLUTstale GLUTstale;
struct _GLUTstale {
  GLUTwindow *window;
  Window win;
  GLUTstale *next;
};

extern GLUTstale *__glutStaleWindowList;

#define GLUT_OVERLAY_EVENT_FILTER_MASK \
  (ExposureMask | \
  StructureNotifyMask | \
  EnterWindowMask | \
  LeaveWindowMask)
#define GLUT_DONT_PROPAGATE_FILTER_MASK \
  (ButtonReleaseMask | \
  ButtonPressMask | \
  KeyPressMask | \
  KeyReleaseMask | \
  PointerMotionMask | \
  Button1MotionMask | \
  Button2MotionMask | \
  Button3MotionMask)
#define GLUT_HACK_STOP_PROPAGATE_MASK \
  (KeyPressMask | \
  KeyReleaseMask)

typedef struct _GLUTmenu GLUTmenu;
typedef struct _GLUTmenuItem GLUTmenuItem;
struct _GLUTmenu {
  int id;               /* small integer menu id (0-based) */
#if defined(_WIN32)
  HMENU win;            /* Win32 menu */
#else
  Window win;           /* X window for the menu */
#endif
  GLUTselectCB select;  /*  function of menu */
  GLUTmenuItem *list;   /* list of menu entries */
  int num;              /* number of entries */
#if !defined(_WIN32)
  Bool managed;         /* are the InputOnly windows size
                           validated? */
  Bool searched;	/* help detect menu loops */
  int pixheight;        /* height of menu in pixels */
  int pixwidth;         /* width of menu in pixels */
#endif
  int submenus;         /* number of submenu entries */
  GLUTmenuItem *highlighted;  /* pointer to highlighted menu
                                 entry, NULL not highlighted */
  GLUTmenu *cascade;    /* currently cascading this menu  */
  GLUTmenuItem *anchor; /* currently anchored to this entry */
  int x;                /* current x origin relative to the
                           root window */
  int y;                /* current y origin relative to the
                           root window */
#ifdef SUPPORT_FORTRAN
  GLUTselectFCB fselect;  /*  function of menu */
#endif
};

struct _GLUTmenuItem {
#if defined(_WIN32)
  HMENU win;            /* Win32 window for entry */
#else
  Window win;           /* InputOnly X window for entry */
#endif
  GLUTmenu *menu;       /* menu entry belongs to */
  Bool isTrigger;       /* is a submenu trigger? */
  int value;            /* value to return for selecting this
                           entry; doubles as submenu id
                           (0-base) if submenu trigger */
#if defined(_WIN32)
  UINT unique;          /* unique menu item id (Win32 only) */
#endif
  char *label;          /* __glutStrdup'ed label string */
  int len;              /* length of label string */
  int pixwidth;         /* width of X window in pixels */
  GLUTmenuItem *next;   /* next menu entry on list for menu */
};

typedef struct _GLUTtimer GLUTtimer;
struct _GLUTtimer {
  GLUTtimer *next;      /* list of timers */
  struct timeval timeout;  /* time to be called */
  GLUTtimerCB func;     /* timer  (value) */
  int value;            /*  return value */
#ifdef SUPPORT_FORTRAN
  GLUTtimerFCB ffunc;   /* Fortran timer  */
#endif
};

typedef struct _GLUTeventParser GLUTeventParser;
struct _GLUTeventParser {
  int (*func) (XEvent *);
  GLUTeventParser *next;
};

/* Declarations to implement glutFullScreen support with
   mwm/4Dwm. */

/* The following X property format is defined in Motif 1.1's
   Xm/MwmUtils.h, but GLUT should not depend on that header
   file. Note: Motif 1.2 expanded this structure with
   uninteresting fields (to GLUT) so just stick with the
   smaller Motif 1.1 structure. */
typedef struct {
#define MWM_HINTS_DECORATIONS   2
  long flags;
  long functions;
  long decorations;
  long input_mode;
} MotifWmHints;

/* Make current and buffer swap macros. */
#ifdef _WIN32
#define MAKE_CURRENT_LAYER(window)                                    \
  {                                                                   \
    HGLRC currentContext = wglGetCurrentContext();                    \
    HDC currentDc = wglGetCurrentDC();                                \
                                                                      \
    if (currentContext != window->renderCtx                           \
      || currentDc != window->renderDc) {                             \
      wglMakeCurrent(window->renderDc, window->renderCtx);            \
    }                                                                 \
  }
#define MAKE_CURRENT_WINDOW(window)                                   \
  {                                                                   \
    HGLRC currentContext = wglGetCurrentContext();                    \
    HDC currentDc = wglGetCurrentDC();                                \
                                                                      \
    if (currentContext != window->ctx || currentDc != window->hdc) {  \
      wglMakeCurrent(window->hdc, window->ctx);                       \
    }                                                                 \
  }
#define MAKE_CURRENT_OVERLAY(overlay) \
  wglMakeCurrent(overlay->hdc, overlay->ctx)
#define UNMAKE_CURRENT() \
  wglMakeCurrent(NULL, NULL)
#define SWAP_BUFFERS_WINDOW(window) \
  SwapBuffers(window->hdc)
#define SWAP_BUFFERS_LAYER(window) \
  SwapBuffers(window->renderDc)
#else
#define MAKE_CURRENT_LAYER(window) \
  glXMakeCurrent(__glutDisplay, window->renderWin, window->renderCtx)
#define MAKE_CURRENT_WINDOW(window) \
  glXMakeCurrent(__glutDisplay, window->win, window->ctx)
#define MAKE_CURRENT_OVERLAY(overlay) \
  glXMakeCurrent(__glutDisplay, overlay->win, overlay->ctx)
#define UNMAKE_CURRENT() \
  glXMakeCurrent(__glutDisplay, None, NULL)
#define SWAP_BUFFERS_WINDOW(window) \
  glXSwapBuffers(__glutDisplay, window->win)
#define SWAP_BUFFERS_LAYER(window) \
  glXSwapBuffers(__glutDisplay, window->renderWin)
#endif

/* private variables from glut_event.c */
extern GLUTwindow *__glutWindowWorkList;
extern int __glutWindowDamaged;
#ifdef SUPPORT_FORTRAN
extern GLUTtimer *__glutTimerList;
extern GLUTtimer *__glutNewTimer;
#endif
extern GLUTmenu *__glutMappedMenu;

extern void (*__glutUpdateInputDeviceMaskFunc) (GLUTwindow *);
#if !defined(_WIN32)
extern void (*__glutMenuItemEnterOrLeave)(GLUTmenuItem * item,
  int num, int type);
extern void (*__glutFinishMenu)(Window win, int x, int y);
extern void (*__glutPaintMenu)(GLUTmenu * menu);
extern void (*__glutStartMenu)(GLUTmenu * menu,
  GLUTwindow * window, int x, int y, int x_win, int y_win);
extern GLUTmenu * (*__glutGetMenuByNum)(int menunum);
extern GLUTmenuItem * (*__glutGetMenuItem)(GLUTmenu * menu,
  Window win, int *which);
extern GLUTmenu * (*__glutGetMenu)(Window win);
#endif

/* private variables from glut_init.c */
extern Atom __glutWMDeleteWindow;
extern Display *__glutDisplay;
extern unsigned int __glutDisplayMode;
extern char *__glutDisplayString;
extern XVisualInfo *(*__glutDetermineVisualFromString) (char *string, Bool * treatAsSingle,
  Criterion * requiredCriteria, int nRequired, int requiredMask, void **fbc);
extern GLboolean __glutDebug;
extern GLboolean __glutForceDirect;
extern GLboolean __glutIconic;
extern GLboolean __glutTryDirect;
extern Window __glutRoot;
extern XSizeHints __glutSizeHints;
extern char **__glutArgv;
extern char *__glutProgramName;
extern int __glutArgc;
extern int __glutConnectionFD;
extern int __glutInitHeight;
extern int __glutInitWidth;
extern int __glutInitX;
extern int __glutInitY;
extern int __glutScreen;
extern int __glutScreenHeight;
extern int __glutScreenWidth;
extern Atom __glutMotifHints;
extern unsigned int __glutModifierMask;
#ifdef _WIN32
extern void (__cdecl *__glutExitFunc)(int retval);
#endif

/* private variables from glut_menu.c */
extern GLUTmenuItem *__glutItemSelected;
extern GLUTmenu **__glutMenuList;
extern void (GLUTCALLBACK *__glutMenuStatusFunc) (int, int, int);
extern void __glutMenuModificationError(void);
extern void __glutSetMenuItem(GLUTmenuItem * item,
  const char *label, int value, Bool isTrigger);

/* private variables from glut_win.c */
extern GLUTwindow **__glutWindowList;
extern GLUTwindow *__glutCurrentWindow;
extern GLUTwindow *__glutMenuWindow;
extern GLUTmenu *__glutCurrentMenu;
extern int __glutWindowListSize;
extern void (*__glutFreeOverlayFunc) (GLUToverlay *);
extern XVisualInfo *__glutDetermineWindowVisual(Bool * treatAsSingle,
  Bool * visAlloced, void **fbc);

/* private variables from glut_mesa.c */
extern int __glutMesaSwapHackSupport;

/* private variables from glut_gamemode.c */
extern GLUTwindow *__glutGameModeWindow;

/* private routines from glut_cindex.c */
extern GLUTcolormap * __glutAssociateNewColormap(XVisualInfo * vis);
extern void __glutFreeColormap(GLUTcolormap *);

/* private routines from glut_cmap.c */
extern void __glutSetupColormap(
  XVisualInfo * vi,
  GLUTcolormap ** colormap,
  Colormap * cmap);
#if !defined(_WIN32)
extern void __glutEstablishColormapsProperty(
  GLUTwindow * window);
extern GLUTwindow *__glutToplevelOf(GLUTwindow * window);
#endif

/* private routines from glut_cursor.c */
extern void __glutSetCursor(GLUTwindow *window);

/* private routines from glut_event.c */
extern void __glutPutOnWorkList(GLUTwindow * window,
  int work_mask);
extern void __glutRegisterEventParser(GLUTeventParser * parser);
extern void __glutPostRedisplay(GLUTwindow * window, int layerMask);

/* private routines from glut_init.c */
#if !defined(_WIN32)
extern void __glutOpenXConnection(char *display);
#else
extern void __glutOpenWin32Connection(char *display);
#endif
extern void __glutInitTime(struct timeval *beginning);

/* private routines for glut_menu.c (or win32_menu.c) */
#if defined(_WIN32)
extern GLUTmenu *__glutGetMenu(HMENU win);
extern GLUTmenu *__glutGetMenuByNum(int menunum);
extern GLUTmenuItem *__glutGetMenuItem(GLUTmenu * menu,
  HMENU win, int *which);
extern void __glutStartMenu(GLUTmenu * menu,
  GLUTwindow * window, int x, int y, int x_win, int y_win);
extern void __glutFinishMenu(Window win, int x, int y);
#endif
extern void __glutSetMenu(GLUTmenu * menu);

/* private routines from glut_util.c */
extern char * __glutStrdup(const char *string);
extern void __glutWarning(char *format,...);
extern void __glutFatalError(char *format,...);
extern void __glutFatalUsage(char *format,...);

/* private routines from glut_win.c */
extern GLUTwindow *__glutGetWindow(Window win);
extern void __glutChangeWindowEventMask(long mask, Bool add);
extern XVisualInfo *__glutDetermineVisual(
  unsigned int mode,
  Bool * fakeSingle,
  XVisualInfo * (getVisualInfo) (unsigned int));
extern XVisualInfo *__glutGetVisualInfo(unsigned int mode);
extern void __glutSetWindow(GLUTwindow * window);
extern void __glutReshapeFunc(GLUTreshapeCB reshapeFunc,
  int callingConvention);
extern void GLUTCALLBACK __glutDefaultReshape(int, int);
extern GLUTwindow *__glutCreateWindow(
  GLUTwindow * parent,
  int x, int y, int width, int height, int gamemode);
extern void __glutDestroyWindow(
  GLUTwindow * window,
  GLUTwindow * initialWindow);

#if !defined(_WIN32)
/* private routines from glut_glxext.c */
extern int __glutIsSupportedByGLX(char *);
#endif

/* private routines from glut_input.c */
extern void  __glutUpdateInputDeviceMask(GLUTwindow * window);

/* private routines from glut_mesa.c */
extern void __glutDetermineMesaSwapHackSupport(void);

/* private routines from glut_gameglut.c */
extern void CDECL __glutCloseDownGameMode(void);

#if defined(_WIN32)
/* private routines from win32_*.c */
extern LONG WINAPI __glutWindowProc(HWND win, UINT msg, WPARAM w, LPARAM l);
extern HDC XHDC;
#endif

#endif /* __glutint_h__ */
