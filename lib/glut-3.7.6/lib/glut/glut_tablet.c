
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>

#include "glutint.h"

void APIENTRY 
glutTabletMotionFunc(GLUTtabletMotionCB tabletMotionFunc)
{
  __glutCurrentWindow->tabletMotion = tabletMotionFunc;
  __glutUpdateInputDeviceMaskFunc = __glutUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
  /* If deinstalling callback, invalidate tablet position. */
  if (tabletMotionFunc == NULL) {
    __glutCurrentWindow->tabletPos[0] = -1;
    __glutCurrentWindow->tabletPos[1] = -1;
  }
}

void APIENTRY 
glutTabletButtonFunc(GLUTtabletButtonCB tabletButtonFunc)
{
  __glutCurrentWindow->tabletButton = tabletButtonFunc;
  __glutUpdateInputDeviceMaskFunc = __glutUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
}
