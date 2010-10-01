
/* Copyright (c) Mark J. Kilgard, 1994, 1997. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if !defined(_WIN32)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

/* SGI optimization introduced in IRIX 6.3 to avoid X server
   round trips for interning common X atoms. */
#if defined(_SGI_EXTRA_PREDEFINES) && !defined(NO_FAST_ATOMS)
#include <X11/SGIFastAtom.h>
#else
#define XSGIFastInternAtom(dpy,string,fast_name,how) XInternAtom(dpy,string,how)
#endif

#include "glutint.h"

/* GLUT inter-file variables */
/* *INDENT-OFF* */
char *__glutProgramName = NULL;
int __glutArgc = 0;
char **__glutArgv = NULL;
char *__glutGeometry = NULL;
Display *__glutDisplay = NULL;
int __glutScreen;
Window __glutRoot;
int __glutScreenHeight;
int __glutScreenWidth;
GLboolean __glutIconic = GL_FALSE;
GLboolean __glutDebug = GL_FALSE;
unsigned int __glutDisplayMode =
  GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH;
char *__glutDisplayString = NULL;
int __glutConnectionFD;
XSizeHints __glutSizeHints = {0};
int __glutInitWidth = 300, __glutInitHeight = 300;
int __glutInitX = -1, __glutInitY = -1;
GLboolean __glutForceDirect = GL_FALSE,
  __glutTryDirect = GL_TRUE;
Atom __glutWMDeleteWindow;
/* *INDENT-ON* */

#ifdef _WIN32
void (__cdecl *__glutExitFunc)(int retval) = NULL;
#endif

static Bool synchronize = False;

#if defined(_WIN32)

#ifdef __BORLANDC__
#include <float.h>  /* For masking floating point exceptions. */
#endif

void
__glutOpenWin32Connection(char* display)
{
  static char *classname;
  WNDCLASS  wc;
  HINSTANCE hInstance = GetModuleHandle(NULL);
  
  /* Make sure we register the window only once. */
  if(classname)
    return;

#ifdef __BORLANDC__
  /* Under certain conditions (e.g. while rendering solid surfaces with
     lighting enabled) Microsoft OpenGL libraries cause some illegal
     operations like floating point overflow or division by zero. The
     default behaviour of Microsoft compilers is to mask (ignore)
     floating point exceptions, while Borland compilers do not.  The
     following function of Borland RTL allows to mask exceptions.
     Advice from Pier Giorgio Esposito (mc2172@mclink.it). */
  _control87(MCW_EM,MCW_EM);
#endif

  classname = "GLUT";

  /* Clear (important!) and then fill in the window class structure. */
  memset(&wc, 0, sizeof(WNDCLASS));
  wc.style         = CS_OWNDC;
  wc.lpfnWndProc   = (WNDPROC)__glutWindowProc;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(hInstance, "GLUT_ICON");
  wc.hCursor       = LoadCursor(hInstance, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = classname;

  if(wc.hIcon == NULL) {
    HINSTANCE hDLLInstance = LoadLibrary("glut32.dll");
	if (hDLLInstance == NULL) {
	  /* Fill in a default icon if one isn't specified as a resource. */
	  wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	} else {
	  wc.hIcon = LoadIcon(hDLLInstance, "GLUT_ICON");
	}
  }
  
  if(!RegisterClass(&wc)) {
    __glutFatalError("RegisterClass() failed:"
		     "Cannot register GLUT window class.");
  }
 
  __glutScreenWidth     = GetSystemMetrics(SM_CXSCREEN);
  __glutScreenHeight    = GetSystemMetrics(SM_CYSCREEN);

  /* Set the root window to NULL because windows creates a top-level
     window when the parent is NULL.  X creates a top-level window
     when the parent is the root window. */
  __glutRoot            = NULL;

  /* Set the display to 1 -- we shouldn't be using this anywhere
     (except as an argument to X calls). */
  __glutDisplay         = (Display*)1;

  /* There isn't any concept of multiple screens in Win32, therefore,
     we don't need to keep track of the screen we're on... it's always
     the same one. */
  __glutScreen          = 0;
}
#else /* !_WIN32 */
void
__glutOpenXConnection(char *display)
{
  int errorBase, eventBase;

  __glutDisplay = XOpenDisplay(display);
  if (!__glutDisplay)
    __glutFatalError("could not open display: %s",
      XDisplayName(display));
  if (synchronize)
    XSynchronize(__glutDisplay, True);
  if (!glXQueryExtension(__glutDisplay, &errorBase, &eventBase))
    __glutFatalError(
      "OpenGL GLX extension not supported by display: %s",
      XDisplayName(display));
  __glutScreen = DefaultScreen(__glutDisplay);
  __glutRoot = RootWindow(__glutDisplay, __glutScreen);
  __glutScreenWidth = DisplayWidth(__glutDisplay, __glutScreen);
  __glutScreenHeight = DisplayHeight(__glutDisplay,
    __glutScreen);
  __glutConnectionFD = ConnectionNumber(__glutDisplay);
  __glutWMDeleteWindow = XSGIFastInternAtom(__glutDisplay,
    "WM_DELETE_WINDOW", SGI_XA_WM_DELETE_WINDOW, False);
}
#endif /* _WIN32 */

void
__glutInitTime(struct timeval *beginning)
{
  static int beenhere = 0;
  static struct timeval genesis;

  if (!beenhere) {
    GETTIMEOFDAY(&genesis);
    beenhere = 1;
  }
  *beginning = genesis;
}

static void
removeArgs(int *argcp, char **argv, int numToRemove)
{
  int i, j;

  for (i = 0, j = numToRemove; argv[j]; i++, j++) {
    argv[i] = argv[j];
  }
  argv[i] = NULL;
  *argcp -= numToRemove;
}

void APIENTRY 
glutInit(int *argcp, char **argv)
{
  char *display = NULL;
  char *str, *geometry = NULL;
  struct timeval unused;
  int i;

  if (__glutDisplay) {
    __glutWarning("glutInit being called a second time.");
    return;
  }
  /* Determine temporary program name. */
  str = strrchr(argv[0], '/');
  if (str == NULL) {
    __glutProgramName = argv[0];
  } else {
    __glutProgramName = str + 1;
  }

  /* Make private copy of command line arguments. */
  __glutArgc = *argcp;
  __glutArgv = (char **) malloc(__glutArgc * sizeof(char *));
  if (!__glutArgv)
    __glutFatalError("out of memory.");
  for (i = 0; i < __glutArgc; i++) {
    __glutArgv[i] = __glutStrdup(argv[i]);
    if (!__glutArgv[i])
      __glutFatalError("out of memory.");
  }

  /* determine permanent program name */
  str = strrchr(__glutArgv[0], '/');
  if (str == NULL) {
    __glutProgramName = __glutArgv[0];
  } else {
    __glutProgramName = str + 1;
  }

  /* parse arguments for standard options */
  for (i = 1; i < __glutArgc; i++) {
    if (!strcmp(__glutArgv[i], "-display")) {
#if defined(_WIN32)
      __glutWarning("-display option not supported by Win32 GLUT.");
#endif
      if (++i >= __glutArgc) {
        __glutFatalError(
          "follow -display option with X display name.");
      }
      display = __glutArgv[i];
      removeArgs(argcp, &argv[1], 2);
    } else if (!strcmp(__glutArgv[i], "-geometry")) {
      if (++i >= __glutArgc) {
        __glutFatalError(
          "follow -geometry option with geometry parameter.");
      }
      geometry = __glutArgv[i];
      removeArgs(argcp, &argv[1], 2);
    } else if (!strcmp(__glutArgv[i], "-direct")) {
#if defined(_WIN32)
      __glutWarning("-direct option not supported by Win32 GLUT.");
#endif
      if (!__glutTryDirect)
        __glutFatalError(
          "cannot force both direct and indirect rendering.");
      __glutForceDirect = GL_TRUE;
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-indirect")) {
#if defined(_WIN32)
      __glutWarning("-indirect option not supported by Win32 GLUT.");
#endif
      if (__glutForceDirect)
        __glutFatalError(
          "cannot force both direct and indirect rendering.");
      __glutTryDirect = GL_FALSE;
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-iconic")) {
      __glutIconic = GL_TRUE;
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-gldebug")) {
      __glutDebug = GL_TRUE;
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-sync")) {
#if defined(_WIN32)
      __glutWarning("-sync option not supported by Win32 GLUT.");
#endif
      synchronize = GL_TRUE;
      removeArgs(argcp, &argv[1], 1);
    } else {
      /* Once unknown option encountered, stop command line
         processing. */
      break;
    }
  }
#if defined(_WIN32)
  __glutOpenWin32Connection(display);
#else
  __glutOpenXConnection(display);
#endif
  if (geometry) {
    int flags, x, y, width, height;

    /* Fix bogus "{width|height} may be used before set"
       warning */
    width = 0;
    height = 0;

    flags = XParseGeometry(geometry, &x, &y,
      (unsigned int *) &width, (unsigned int *) &height);
    if (WidthValue & flags) {
      /* Careful because X does not allow zero or negative
         width windows */
      if (width > 0)
        __glutInitWidth = width;
    }
    if (HeightValue & flags) {
      /* Careful because X does not allow zero or negative
         height windows */
      if (height > 0)
        __glutInitHeight = height;
    }
    glutInitWindowSize(__glutInitWidth, __glutInitHeight);
    if (XValue & flags) {
      if (XNegative & flags)
        x = DisplayWidth(__glutDisplay, __glutScreen) +
          x - __glutSizeHints.width;
      /* Play safe: reject negative X locations */
      if (x >= 0)
        __glutInitX = x;
    }
    if (YValue & flags) {
      if (YNegative & flags)
        y = DisplayHeight(__glutDisplay, __glutScreen) +
          y - __glutSizeHints.height;
      /* Play safe: reject negative Y locations */
      if (y >= 0)
        __glutInitY = y;
    }
    glutInitWindowPosition(__glutInitX, __glutInitY);
  }
  __glutInitTime(&unused);
}

#ifdef _WIN32
void APIENTRY 
__glutInitWithExit(int *argcp, char **argv, void (__cdecl *exitfunc)(int))
{
  __glutExitFunc = exitfunc;
  glutInit(argcp, argv);
}
#endif

/* CENTRY */
void APIENTRY 
glutInitWindowPosition(int x, int y)
{
  __glutInitX = x;
  __glutInitY = y;
  if (x >= 0 && y >= 0) {
    __glutSizeHints.x = x;
    __glutSizeHints.y = y;
    __glutSizeHints.flags |= USPosition;
  } else {
    __glutSizeHints.flags &= ~USPosition;
  }
}

void APIENTRY 
glutInitWindowSize(int width, int height)
{
  __glutInitWidth = width;
  __glutInitHeight = height;
  if (width > 0 && height > 0) {
    __glutSizeHints.width = width;
    __glutSizeHints.height = height;
    __glutSizeHints.flags |= USSize;
  } else {
    __glutSizeHints.flags &= ~USSize;
  }
}

void APIENTRY 
glutInitDisplayMode(unsigned int mask)
{
  __glutDisplayMode = mask;
}

/* ENDCENTRY */
