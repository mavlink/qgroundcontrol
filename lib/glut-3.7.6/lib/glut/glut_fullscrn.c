
/* Copyright (c) Mark J. Kilgard, 1995, 1998. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>  /* SunOS multithreaded assert() needs <stdio.h>.  Lame. */
#include <assert.h>

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

/* CENTRY */
void APIENTRY 
glutFullScreen(void)
{
  assert(!__glutCurrentWindow->parent);
  IGNORE_IN_GAME_MODE();
#if !defined(_WIN32)
  if (__glutMotifHints == None) {
    __glutMotifHints = XSGIFastInternAtom(__glutDisplay, "_MOTIF_WM_HINTS",
      SGI_XA__MOTIF_WM_HINTS, 0);
    if (__glutMotifHints == None) {
      __glutWarning("Could not intern X atom for _MOTIF_WM_HINTS.");
    }
  }
#endif

  __glutCurrentWindow->desiredX = 0;
  __glutCurrentWindow->desiredY = 0;
  __glutCurrentWindow->desiredWidth = __glutScreenWidth;
  __glutCurrentWindow->desiredHeight = __glutScreenHeight;
  __glutCurrentWindow->desiredConfMask |= CWX | CWY | CWWidth | CWHeight;

  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_CONFIGURE_WORK | GLUT_FULL_SCREEN_WORK);
}

/* ENDCENTRY */
