
/* Copyright (c) Mark J. Kilgard, 1994, 1996, 1997.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>  /* SunOS multithreaded assert() needs <stdio.h>.  Lame. */
#include <assert.h>
#if !defined(_WIN32)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>  /* for XA_RGB_DEFAULT_MAP atom */
#if defined(__vms)
#include <Xmu/StdCmap.h>  /* for XmuLookupStandardColormap */
#else
#include <X11/Xmu/StdCmap.h>  /* for XmuLookupStandardColormap */
#endif
#endif

/* SGI optimization introduced in IRIX 6.3 to avoid X server
   round trips for interning common X atoms. */
#if defined(_SGI_EXTRA_PREDEFINES) && !defined(NO_FAST_ATOMS)
#include <X11/SGIFastAtom.h>
#else
#define XSGIFastInternAtom(dpy,string,fast_name,how) XInternAtom(dpy,string,how)
#endif

#include "glutint.h"
#include "layerutil.h"

GLUTcolormap *__glutColormapList = NULL;

GLUTcolormap *
__glutAssociateNewColormap(XVisualInfo * vis)
{
  GLUTcolormap *cmap;
  int transparentPixel, i;
  unsigned long pixels[255];

  cmap = (GLUTcolormap *) malloc(sizeof(GLUTcolormap));
  if (!cmap)
    __glutFatalError("out of memory.");
#if defined(_WIN32)
  pixels[0] = 0;        /* avoid compilation warnings on win32 */
  cmap->visual = 0;
  cmap->size = 256;     /* always assume 256 on Win32 */
#else
  cmap->visual = vis->visual;
  cmap->size = vis->visual->map_entries;
#endif
  cmap->refcnt = 1;
  cmap->cells = (GLUTcolorcell *)
    malloc(sizeof(GLUTcolorcell) * cmap->size);
  if (!cmap->cells)
    __glutFatalError("out of memory.");
  /* make all color cell entries be invalid */
  for (i = cmap->size - 1; i >= 0; i--) {
    cmap->cells[i].component[GLUT_RED] = -1.0;
    cmap->cells[i].component[GLUT_GREEN] = -1.0;
    cmap->cells[i].component[GLUT_BLUE] = -1.0;
  }
  transparentPixel = __glutGetTransparentPixel(__glutDisplay, vis);
  if (transparentPixel == -1 || transparentPixel >= cmap->size) {

    /* If there is no transparent pixel or if the transparent
       pixel is outside the range of valid colormap cells (HP
       can implement their overlays this smart way since their
       transparent pixel is 255), we can AllocAll the colormap.
       See note below.  */

    cmap->cmap = XCreateColormap(__glutDisplay,
      __glutRoot, cmap->visual, AllocAll);
  } else {

    /* On machines where zero (or some other value in the range
       of 0 through map_entries-1), BadAlloc may be generated
       when an AllocAll overlay colormap is allocated since the
       transparent pixel precludes all the cells in the colormap
       being allocated (the transparent pixel is pre-allocated).
       So in this case, use XAllocColorCells to allocate
       map_entries-1 pixels (that is, all but the transparent
       pixel.  */

#if defined(_WIN32)
    cmap->cmap = XCreateColormap(__glutDisplay,
      __glutRoot, 0, AllocNone);
#else
    cmap->cmap = XCreateColormap(__glutDisplay,
      __glutRoot, vis->visual, AllocNone);
    XAllocColorCells(__glutDisplay, cmap->cmap, False, 0, 0,
      pixels, cmap->size - 1);
#endif
  }
  cmap->next = __glutColormapList;
  __glutColormapList = cmap;
  return cmap;
}

static GLUTcolormap *
associateColormap(XVisualInfo * vis)
{
#if !defined(_WIN32)
  GLUTcolormap *cmap = __glutColormapList;

  while (cmap != NULL) {
    /* Play safe: compare visual IDs, not Visual*'s. */
    if (cmap->visual->visualid == vis->visual->visualid) {
      /* Already have created colormap for the visual. */
      cmap->refcnt++;
      return cmap;
    }
    cmap = cmap->next;
  }
#endif
  return __glutAssociateNewColormap(vis);
}

void
__glutSetupColormap(XVisualInfo * vi, GLUTcolormap ** colormap, Colormap * cmap)
{
#if defined(_WIN32)
  if (vi->dwFlags & PFD_NEED_PALETTE || vi->iPixelType == PFD_TYPE_COLORINDEX) {
    *colormap = associateColormap(vi);
    *cmap = (*colormap)->cmap;
  } else {
    *colormap = NULL;
    *cmap = 0;
  }
#else
  Status status;
  XStandardColormap *standardCmaps;
  int i, numCmaps;
  static Atom hpColorRecoveryAtom = -1;
  int isRGB, visualClass, rc;

#if defined(__cplusplus) || defined(c_plusplus)
  visualClass = vi->c_class;
#else
  visualClass = vi->class;
#endif
  switch (visualClass) {
  case PseudoColor:
    /* Mesa might return a PseudoColor visual for RGB mode. */
    rc = glXGetConfig(__glutDisplay, vi, GLX_RGBA, &isRGB);
    if (rc == 0 && isRGB) {
      /* Must be Mesa. */
      *colormap = NULL;
      if (MaxCmapsOfScreen(DefaultScreenOfDisplay(__glutDisplay)) == 1
        && vi->visual == DefaultVisual(__glutDisplay, __glutScreen)) {
        char *privateCmap = getenv("MESA_PRIVATE_CMAP");

        if (privateCmap) {
          /* User doesn't want to share colormaps. */
          *cmap = XCreateColormap(__glutDisplay, __glutRoot,
            vi->visual, AllocNone);
        } else {
          /* Share the root colormap. */
          *cmap = DefaultColormap(__glutDisplay, __glutScreen);
        }
      } else {
        /* Get our own PseudoColor colormap. */
        *cmap = XCreateColormap(__glutDisplay, __glutRoot,
          vi->visual, AllocNone);
      }
    } else {
      /* CI mode, real GLX never returns a PseudoColor visual
         for RGB mode. */
      *colormap = associateColormap(vi);
      *cmap = (*colormap)->cmap;
    }
    break;
  case TrueColor:
  case DirectColor:
    *colormap = NULL;   /* NULL if RGBA */

    /* Hewlett-Packard supports a feature called "HP Color
       Recovery". Mesa has code to use HP Color Recovery.  For
       Mesa to use this feature, the atom
       _HP_RGB_SMOOTH_MAP_LIST must be defined on the root
       window AND the colormap obtainable by XGetRGBColormaps
       for that atom must be set on the window.  If that
       colormap is not set, the output will look stripy. */

    if (hpColorRecoveryAtom == -1) {
      char *xvendor;

#define VENDOR_HP "Hewlett-Packard"

      /* Only makes sense to make XInternAtom round-trip if we
         know that we are connected to an HP X server. */
      xvendor = ServerVendor(__glutDisplay);
      if (!strncmp(xvendor, VENDOR_HP, sizeof(VENDOR_HP) - 1)) {
        hpColorRecoveryAtom = XInternAtom(__glutDisplay, "_HP_RGB_SMOOTH_MAP_LIST", True);
      } else {
        hpColorRecoveryAtom = None;
      }
    }
    if (hpColorRecoveryAtom != None) {
      status = XGetRGBColormaps(__glutDisplay, __glutRoot,
        &standardCmaps, &numCmaps, hpColorRecoveryAtom);
      if (status == 1) {
        for (i = 0; i < numCmaps; i++) {
          if (standardCmaps[i].visualid == vi->visualid) {
            *cmap = standardCmaps[i].colormap;
            XFree(standardCmaps);
            return;
          }
        }
        XFree(standardCmaps);
      }
    }
#ifndef SOLARIS_2_4_BUG
    /* Solaris 2.4 and 2.5 have a bug in their
       XmuLookupStandardColormap implementations.  Please
       compile your Solaris 2.4 or 2.5 version of GLUT with
       -DSOLARIS_2_4_BUG to work around this bug. The symptom
       of the bug is that programs will get a BadMatch error
       from X_CreateWindow when creating a GLUT window because
       Solaris 2.4 and 2.5 create a  corrupted RGB_DEFAULT_MAP
       property.  Note that this workaround prevents Colormap
       sharing between applications, perhaps leading
       unnecessary colormap installations or colormap flashing.
       Sun fixed this bug in Solaris 2.6. */
    status = XmuLookupStandardColormap(__glutDisplay,
      vi->screen, vi->visualid, vi->depth, XA_RGB_DEFAULT_MAP,
      /* replace */ False, /* retain */ True);
    if (status == 1) {
      status = XGetRGBColormaps(__glutDisplay, __glutRoot,
        &standardCmaps, &numCmaps, XA_RGB_DEFAULT_MAP);
      if (status == 1) {
        for (i = 0; i < numCmaps; i++) {
          if (standardCmaps[i].visualid == vi->visualid) {
            *cmap = standardCmaps[i].colormap;
            XFree(standardCmaps);
            return;
          }
        }
        XFree(standardCmaps);
      }
    }
#endif
    /* If no standard colormap but TrueColor, just make a
       private one. */
    /* XXX Should do a better job of internal sharing for
       privately allocated TrueColor colormaps. */
    /* XXX DirectColor probably needs ramps hand initialized! */
    *cmap = XCreateColormap(__glutDisplay, __glutRoot,
      vi->visual, AllocNone);
    break;
  case StaticColor:
  case StaticGray:
  case GrayScale:
    /* Mesa supports these visuals */
    *colormap = NULL;
    *cmap = XCreateColormap(__glutDisplay, __glutRoot,
      vi->visual, AllocNone);
    break;
  default:
    __glutFatalError(
      "could not allocate colormap for visual type: %d.",
      visualClass);
  }
  return;
#endif
}

#if !defined(_WIN32)
static int
findColormaps(GLUTwindow * window,
  Window * winlist, Colormap * cmaplist, int num, int max)
{
  GLUTwindow *child;
  int i;

  /* Do not allow more entries that maximum number of
     colormaps! */
  if (num >= max)
    return num;
  /* Is cmap for this window already on the list? */
  for (i = 0; i < num; i++) {
    if (cmaplist[i] == window->cmap)
      goto normalColormapAlreadyListed;
  }
  /* Not found on the list; add colormap and window. */
  winlist[num] = window->win;
  cmaplist[num] = window->cmap;
  num++;

normalColormapAlreadyListed:

  /* Repeat above but for the overlay colormap if there one. */
  if (window->overlay) {
    if (num >= max)
      return num;
    for (i = 0; i < num; i++) {
      if (cmaplist[i] == window->overlay->cmap)
        goto overlayColormapAlreadyListed;
    }
    winlist[num] = window->overlay->win;
    cmaplist[num] = window->overlay->cmap;
    num++;
  }
overlayColormapAlreadyListed:

  /* Recursively search children. */
  child = window->children;
  while (child) {
    num = findColormaps(child, winlist, cmaplist, num, max);
    child = child->siblings;
  }
  return num;
}

void
__glutEstablishColormapsProperty(GLUTwindow * window)
{
  /* this routine is strictly X.  Win32 doesn't need to do
     anything of this sort (but has to do other wacky stuff
     later). */
  static Atom wmColormapWindows = None;
  Window *winlist;
  Colormap *cmaplist;
  Status status;
  int maxcmaps, num;

  assert(!window->parent);
  maxcmaps = MaxCmapsOfScreen(ScreenOfDisplay(__glutDisplay,
      __glutScreen));
  /* For portability reasons we don't use alloca for winlist
     and cmaplist, but we could. */
  winlist = (Window *) malloc(maxcmaps * sizeof(Window));
  cmaplist = (Colormap *) malloc(maxcmaps * sizeof(Colormap));
  num = findColormaps(window, winlist, cmaplist, 0, maxcmaps);
  if (num < 2) {
    /* Property no longer needed; remove it. */
    wmColormapWindows = XSGIFastInternAtom(__glutDisplay,
      "WM_COLORMAP_WINDOWS", SGI_XA_WM_COLORMAP_WINDOWS, False);
    if (wmColormapWindows == None) {
      __glutWarning("Could not intern X atom for WM_COLORMAP_WINDOWS.");
      return;
    }
    XDeleteProperty(__glutDisplay, window->win, wmColormapWindows);
  } else {
    status = XSetWMColormapWindows(__glutDisplay, window->win,
      winlist, num);
    /* XSetWMColormapWindows should always work unless the
       WM_COLORMAP_WINDOWS property cannot be intern'ed.  We
       check to be safe. */
    if (status == False)
      __glutFatalError("XSetWMColormapWindows returned False.");
  }
  /* For portability reasons we don't use alloca for winlist
     and cmaplist, but we could. */
  free(winlist);
  free(cmaplist);
}

GLUTwindow *
__glutToplevelOf(GLUTwindow * window)
{
  while (window->parent) {
    window = window->parent;
  }
  return window;
}
#endif

void
__glutFreeColormap(GLUTcolormap * cmap)
{
  GLUTcolormap *cur, **prev;

  cmap->refcnt--;
  if (cmap->refcnt == 0) {
    /* remove from colormap list */
    cur = __glutColormapList;
    prev = &__glutColormapList;
    while (cur) {
      if (cur == cmap) {
        *prev = cmap->next;
        break;
      }
      prev = &(cur->next);
      cur = cur->next;
    }
    /* actually free colormap */
    XFreeColormap(__glutDisplay, cmap->cmap);
    free(cmap->cells);
    free(cmap);
  }
}

