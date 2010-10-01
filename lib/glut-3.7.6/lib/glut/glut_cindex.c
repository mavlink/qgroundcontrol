
/* Copyright (c) Mark J. Kilgard, 1994, 1996, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include "glutint.h"

#define CLAMP(i) ((i) > 1.0 ? 1.0 : ((i) < 0.0 ? 0.0 : (i)))

/* CENTRY */
void APIENTRY
glutSetColor(int ndx, GLfloat red, GLfloat green, GLfloat blue)
{
  GLUTcolormap *cmap, *newcmap;
  XVisualInfo *vis;
  XColor color;
  int i;

  if (__glutCurrentWindow->renderWin == __glutCurrentWindow->win) {
    cmap = __glutCurrentWindow->colormap;
    vis = __glutCurrentWindow->vis;
  } else {
    cmap = __glutCurrentWindow->overlay->colormap;
    vis = __glutCurrentWindow->overlay->vis;
    if (ndx == __glutCurrentWindow->overlay->transparentPixel) {
      __glutWarning(
        "glutSetColor: cannot set color of overlay transparent index %d\n",
        ndx);
      return;
    }
  }

  if (!cmap) {
    __glutWarning("glutSetColor: current window is RGBA");
    return;
  }
#if defined(_WIN32)
  if (ndx >= 256 ||     /* always assume 256 colors on Win32 */
#else
  if (ndx >= vis->visual->map_entries ||
#endif
    ndx < 0) {
    __glutWarning("glutSetColor: index %d out of range", ndx);
    return;
  }
  if (cmap->refcnt > 1) {
    newcmap = __glutAssociateNewColormap(vis);
    cmap->refcnt--;
    /* Wouldn't it be nice if XCopyColormapAndFree could be
       told not to free the old colormap's entries! */
    for (i = cmap->size - 1; i >= 0; i--) {
      if (i == ndx) {
        /* We are going to set this cell shortly! */
        continue;
      }
      if (cmap->cells[i].component[GLUT_RED] >= 0.0) {
        color.pixel = i;
        newcmap->cells[i].component[GLUT_RED] =
          cmap->cells[i].component[GLUT_RED];
        color.red = (GLfloat) 0xffff *
          cmap->cells[i].component[GLUT_RED];
        newcmap->cells[i].component[GLUT_GREEN] =
          cmap->cells[i].component[GLUT_GREEN];
        color.green = (GLfloat) 0xffff *
          cmap->cells[i].component[GLUT_GREEN];
        newcmap->cells[i].component[GLUT_BLUE] =
          cmap->cells[i].component[GLUT_BLUE];
        color.blue = (GLfloat) 0xffff *
          cmap->cells[i].component[GLUT_BLUE];
        color.flags = DoRed | DoGreen | DoBlue;
#if defined(_WIN32)
        if (IsWindowVisible(__glutCurrentWindow->win)) {
          XHDC = __glutCurrentWindow->hdc;
        } else {
          XHDC = 0;
        }
#endif
        XStoreColor(__glutDisplay, newcmap->cmap, &color);
      } else {
        /* Leave unallocated entries unallocated. */
      }
    }
    cmap = newcmap;
    if (__glutCurrentWindow->renderWin == __glutCurrentWindow->win) {
      __glutCurrentWindow->colormap = cmap;
      __glutCurrentWindow->cmap = cmap->cmap;
    } else {
      __glutCurrentWindow->overlay->colormap = cmap;
      __glutCurrentWindow->overlay->cmap = cmap->cmap;
    }
    XSetWindowColormap(__glutDisplay,
      __glutCurrentWindow->renderWin, cmap->cmap);

#if !defined(_WIN32)
    {
      GLUTwindow *toplevel;

      toplevel = __glutToplevelOf(__glutCurrentWindow);
      if (toplevel->cmap != cmap->cmap) {
        __glutPutOnWorkList(toplevel, GLUT_COLORMAP_WORK);
      }
    }
#endif
  }
  color.pixel = ndx;
  red = CLAMP(red);
  cmap->cells[ndx].component[GLUT_RED] = red;
  color.red = (GLfloat) 0xffff *red;
  green = CLAMP(green);
  cmap->cells[ndx].component[GLUT_GREEN] = green;
  color.green = (GLfloat) 0xffff *green;
  blue = CLAMP(blue);
  cmap->cells[ndx].component[GLUT_BLUE] = blue;
  color.blue = (GLfloat) 0xffff *blue;
  color.flags = DoRed | DoGreen | DoBlue;
#if defined(_WIN32)
  if (IsWindowVisible(__glutCurrentWindow->win)) {
    XHDC = __glutCurrentWindow->hdc;
  } else {
    XHDC = 0;
  }
#endif
  XStoreColor(__glutDisplay, cmap->cmap, &color);
}

GLfloat APIENTRY
glutGetColor(int ndx, int comp)
{
  GLUTcolormap *colormap;
  XVisualInfo *vis;

  if (__glutCurrentWindow->renderWin == __glutCurrentWindow->win) {
    colormap = __glutCurrentWindow->colormap;
    vis = __glutCurrentWindow->vis;
  } else {
    colormap = __glutCurrentWindow->overlay->colormap;
    vis = __glutCurrentWindow->overlay->vis;
    if (ndx == __glutCurrentWindow->overlay->transparentPixel) {
      __glutWarning("glutGetColor: requesting overlay transparent index %d\n",
        ndx);
      return -1.0;
    }
  }

  if (!colormap) {
    __glutWarning("glutGetColor: current window is RGBA");
    return -1.0;
  }
#if defined(_WIN32)
#define OUT_OF_RANGE_NDX(ndx) (ndx >= 256 || ndx < 0)
#else
#define OUT_OF_RANGE_NDX(ndx) (ndx >= vis->visual->map_entries || ndx < 0)
#endif
  if (OUT_OF_RANGE_NDX(ndx)) {
    __glutWarning("glutGetColor: index %d out of range", ndx);
    return -1.0;
  }
  return colormap->cells[ndx].component[comp];
}

void APIENTRY
glutCopyColormap(int winnum)
{
  GLUTwindow *window = __glutWindowList[winnum - 1];
  GLUTcolormap *oldcmap, *newcmap;
  XVisualInfo *dstvis;

  if (__glutCurrentWindow->renderWin == __glutCurrentWindow->win) {
    oldcmap = __glutCurrentWindow->colormap;
    dstvis = __glutCurrentWindow->vis;
    newcmap = window->colormap;
  } else {
    oldcmap = __glutCurrentWindow->overlay->colormap;
    dstvis = __glutCurrentWindow->overlay->vis;
    if (!window->overlay) {
      __glutWarning("glutCopyColormap: window %d has no overlay", winnum);
      return;
    }
    newcmap = window->overlay->colormap;
  }

  if (!oldcmap) {
    __glutWarning("glutCopyColormap: destination colormap must be color index");
    return;
  }
  if (!newcmap) {
    __glutWarning(
      "glutCopyColormap: source colormap of window %d must be color index",
      winnum);
    return;
  }
  if (newcmap == oldcmap) {
    /* Source and destination are the same; now copy needed. */
    return;
  }
#if !defined(_WIN32)
  /* Play safe: compare visual IDs, not Visual*'s. */
  if (newcmap->visual->visualid == oldcmap->visual->visualid) {
#endif
    /* Visuals match!  "Copy" by reference...  */
    __glutFreeColormap(oldcmap);
    newcmap->refcnt++;
    if (__glutCurrentWindow->renderWin == __glutCurrentWindow->win) {
      __glutCurrentWindow->colormap = newcmap;
      __glutCurrentWindow->cmap = newcmap->cmap;
    } else {
      __glutCurrentWindow->overlay->colormap = newcmap;
      __glutCurrentWindow->overlay->cmap = newcmap->cmap;
    }
    XSetWindowColormap(__glutDisplay, __glutCurrentWindow->renderWin,
      newcmap->cmap);
#if !defined(_WIN32)
    __glutPutOnWorkList(__glutToplevelOf(window), GLUT_COLORMAP_WORK);
  } else {
    GLUTcolormap *copycmap;
    XColor color;
    int i, last;

    /* Visuals different - need a distinct X colormap! */
    copycmap = __glutAssociateNewColormap(dstvis);
    /* Wouldn't it be nice if XCopyColormapAndFree could be
       told not to free the old colormap's entries! */
    last = newcmap->size;
    if (last > copycmap->size) {
      last = copycmap->size;
    }
    for (i = last - 1; i >= 0; i--) {
      if (newcmap->cells[i].component[GLUT_RED] >= 0.0) {
        color.pixel = i;
        copycmap->cells[i].component[GLUT_RED] =
          newcmap->cells[i].component[GLUT_RED];
        color.red = (GLfloat) 0xffff *
          newcmap->cells[i].component[GLUT_RED];
        copycmap->cells[i].component[GLUT_GREEN] =
          newcmap->cells[i].component[GLUT_GREEN];
        color.green = (GLfloat) 0xffff *
          newcmap->cells[i].component[GLUT_GREEN];
        copycmap->cells[i].component[GLUT_BLUE] =
          newcmap->cells[i].component[GLUT_BLUE];
        color.blue = (GLfloat) 0xffff *
          newcmap->cells[i].component[GLUT_BLUE];
        color.flags = DoRed | DoGreen | DoBlue;
        XStoreColor(__glutDisplay, copycmap->cmap, &color);
      }
    }
  }
#endif
}
/* ENDCENTRY */
