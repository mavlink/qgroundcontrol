
/* Copyright (c) Mark J. Kilgard, 1994.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#if !defined(_WIN32)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>  /* for XA_STRING atom */
#endif

#include "glutint.h"

/* CENTRY */
void APIENTRY 
glutSetWindowTitle(const char *title)
{
  XTextProperty textprop;

  assert(!__glutCurrentWindow->parent);
  IGNORE_IN_GAME_MODE();
  textprop.value = (unsigned char *) title;
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = strlen(title);
  XSetWMName(__glutDisplay,
    __glutCurrentWindow->win, &textprop);
  XFlush(__glutDisplay);
}

void APIENTRY 
glutSetIconTitle(const char *title)
{
#if !defined(_WIN32)
  XTextProperty textprop;

  assert(!__glutCurrentWindow->parent);
  IGNORE_IN_GAME_MODE();
  textprop.value = (unsigned char *) title;
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = strlen(title);
  XSetWMIconName(__glutDisplay,
    __glutCurrentWindow->win, &textprop);
  XFlush(__glutDisplay);
#endif
}

void APIENTRY 
glutPositionWindow(int x, int y)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredX = x;
  __glutCurrentWindow->desiredY = y;
  __glutCurrentWindow->desiredConfMask |= CWX | CWY;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void APIENTRY 
glutReshapeWindow(int w, int h)
{
  IGNORE_IN_GAME_MODE();
  if (w <= 0 || h <= 0)
    __glutWarning("glutReshapeWindow: non-positive width or height not allowed");

  __glutCurrentWindow->desiredWidth = w;
  __glutCurrentWindow->desiredHeight = h;
  __glutCurrentWindow->desiredConfMask |= CWWidth | CWHeight;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void APIENTRY 
glutPopWindow(void)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredStack = Above;
  __glutCurrentWindow->desiredConfMask |= CWStackMode;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void APIENTRY 
glutPushWindow(void)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredStack = Below;
  __glutCurrentWindow->desiredConfMask |= CWStackMode;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void APIENTRY 
glutIconifyWindow(void)
{
  IGNORE_IN_GAME_MODE();
  assert(!__glutCurrentWindow->parent);
  __glutCurrentWindow->desiredMapState = IconicState;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_MAP_WORK);
}

void APIENTRY 
glutShowWindow(void)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredMapState = NormalState;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_MAP_WORK);
}

void APIENTRY 
glutHideWindow(void)
{
  IGNORE_IN_GAME_MODE();
  __glutCurrentWindow->desiredMapState = WithdrawnState;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_MAP_WORK);
}

/* ENDCENTRY */
