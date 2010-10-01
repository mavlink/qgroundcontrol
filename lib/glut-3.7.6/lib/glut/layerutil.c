
/* Copyright (c) Mark J. Kilgard, 1993, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* Based on XLayerUtil.c: Revision: 1.5 */

#include <stdio.h>
#include <stdlib.h>
#include "layerutil.h"

/* SGI optimization introduced in IRIX 6.3 to avoid X server
   round trips for interning common X atoms. */
#include <X11/Xatom.h>
#if defined(_SGI_EXTRA_PREDEFINES) && !defined(NO_FAST_ATOMS)
#include <X11/SGIFastAtom.h>
#else
#define XSGIFastInternAtom(dpy,string,fast_name,how) XInternAtom(dpy,string,how)
#endif

static Bool layersRead = False;
static OverlayInfo **overlayInfoPerScreen;
static unsigned long *numOverlaysPerScreen;

static void
findServerOverlayVisualsInfo(Display * dpy)
{
  static Atom overlayVisualsAtom;
  Atom actualType;
  Status status;
  unsigned long sizeData, bytesLeft;
  Window root;
  int actualFormat, numScreens, i;

  if (layersRead == False) {
    overlayVisualsAtom = XSGIFastInternAtom(dpy,
      "SERVER_OVERLAY_VISUALS", SGI_XA_SERVER_OVERLAY_VISUALS, True);
    if (overlayVisualsAtom != None) {
      numScreens = ScreenCount(dpy);
      overlayInfoPerScreen = (OverlayInfo **)
        malloc(numScreens * sizeof(OverlayInfo *));
      numOverlaysPerScreen = (unsigned long *)
        malloc(numScreens * sizeof(unsigned long));
      if (overlayInfoPerScreen != NULL &&
        numOverlaysPerScreen != NULL) {
        for (i = 0; i < numScreens; i++) {
          root = RootWindow(dpy, i);
          status = XGetWindowProperty(dpy, root,
            overlayVisualsAtom, 0L, (long) 10000, False,
            overlayVisualsAtom, &actualType, &actualFormat,
            &sizeData, &bytesLeft,
            (unsigned char **) &overlayInfoPerScreen[i]);
          if (status != Success ||
            actualType != overlayVisualsAtom ||
            actualFormat != 32 || sizeData < 4)
            numOverlaysPerScreen[i] = 0;
          else
            /* Four 32-bit quantities per
               SERVER_OVERLAY_VISUALS entry. */
            numOverlaysPerScreen[i] = sizeData / 4;
        }
        layersRead = True;
      } else {
        if (overlayInfoPerScreen != NULL)
          free(overlayInfoPerScreen);
        if (numOverlaysPerScreen != NULL)
          free(numOverlaysPerScreen);
      }
    }
  }
}

int
__glutGetTransparentPixel(Display * dpy, XVisualInfo * vinfo)
{
  int i, screen = vinfo->screen;
  OverlayInfo *overlayInfo;

  findServerOverlayVisualsInfo(dpy);
  if (layersRead) {
    for (i = 0; i < numOverlaysPerScreen[screen]; i++) {
      overlayInfo = &overlayInfoPerScreen[screen][i];
      if (vinfo->visualid == overlayInfo->overlay_visual) {
        if (overlayInfo->transparent_type == TransparentPixel) {
          return (int) overlayInfo->value;
        } else {
          return -1;
        }
      }
    }
  }
  return -1;
}

XLayerVisualInfo *
__glutXGetLayerVisualInfo(Display * dpy, long lvinfo_mask,
  XLayerVisualInfo * lvinfo_template, int *nitems_return)
{
  XVisualInfo *vinfo;
  XLayerVisualInfo *layerInfo;
  int numVisuals, count, i, j;

  vinfo = XGetVisualInfo(dpy, lvinfo_mask & VisualAllMask,
    &lvinfo_template->vinfo, nitems_return);
  if (vinfo == NULL)
    return NULL;
  numVisuals = *nitems_return;
  findServerOverlayVisualsInfo(dpy);
  layerInfo = (XLayerVisualInfo *)
    malloc(numVisuals * sizeof(XLayerVisualInfo));
  if (layerInfo == NULL) {
    XFree(vinfo);
    return NULL;
  }
  count = 0;
  for (i = 0; i < numVisuals; i++) {
    XVisualInfo *pVinfo = &vinfo[i];
    int screen = pVinfo->screen;
    OverlayInfo *overlayInfo = NULL;

    overlayInfo = NULL;
    if (layersRead) {
      for (j = 0; j < numOverlaysPerScreen[screen]; j++)
        if (pVinfo->visualid ==
          overlayInfoPerScreen[screen][j].overlay_visual) {
          overlayInfo = &overlayInfoPerScreen[screen][j];
          break;
        }
    }
    if (lvinfo_mask & VisualLayerMask)
      if (overlayInfo == NULL) {
        if (lvinfo_template->layer != 0)
          continue;
      } else if (lvinfo_template->layer != overlayInfo->layer)
        continue;
    if (lvinfo_mask & VisualTransparentType)
      if (overlayInfo == NULL) {
        if (lvinfo_template->type != None)
          continue;
      } else if (lvinfo_template->type !=
        overlayInfo->transparent_type)
        continue;
    if (lvinfo_mask & VisualTransparentValue)
      if (overlayInfo == NULL)
        /* Non-overlay visuals have no sense of
           TransparentValue. */
        continue;
      else if (lvinfo_template->value != overlayInfo->value)
        continue;
    layerInfo[count].vinfo = *pVinfo;
    if (overlayInfo == NULL) {
      layerInfo[count].layer = 0;
      layerInfo[count].type = None;
      layerInfo[count].value = 0;  /* meaningless */
    } else {
      layerInfo[count].layer = overlayInfo->layer;
      layerInfo[count].type = overlayInfo->transparent_type;
      layerInfo[count].value = overlayInfo->value;
    }
    count++;
  }
  XFree(vinfo);
  *nitems_return = count;
  if (count == 0) {
    XFree(layerInfo);
    return NULL;
  } else
    return layerInfo;
}

#if 0                   /* Unused by GLUT. */
Status
__glutXMatchLayerVisualInfo(Display * dpy, int screen,
  int depth, int visualClass, int layer,
  XLayerVisualInfo * lvinfo_return)
{
  XLayerVisualInfo *lvinfo;
  XLayerVisualInfo lvinfoTemplate;
  int nitems;

  lvinfoTemplate.vinfo.screen = screen;
  lvinfoTemplate.vinfo.depth = depth;
#if defined(__cplusplus) || defined(c_plusplus)
  lvinfoTemplate.vinfo.c_class = visualClass;
#else
  lvinfoTemplate.vinfo.class = visualClass;
#endif
  lvinfoTemplate.layer = layer;
  lvinfo = __glutXGetLayerVisualInfo(dpy,
    VisualScreenMask | VisualDepthMask |
    VisualClassMask | VisualLayerMask,
    &lvinfoTemplate, &nitems);
  if (lvinfo != NULL && nitems > 0) {
    *lvinfo_return = *lvinfo;
    return 1;
  } else
    return 0;
}
#endif
