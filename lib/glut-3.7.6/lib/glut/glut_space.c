
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glutint.h"

void APIENTRY 
glutSpaceballMotionFunc(GLUTspaceMotionCB spaceMotionFunc)
{
  __glutCurrentWindow->spaceMotion = spaceMotionFunc;
  __glutUpdateInputDeviceMaskFunc = __glutUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
}

void APIENTRY 
glutSpaceballRotateFunc(GLUTspaceRotateCB spaceRotateFunc)
{
  __glutCurrentWindow->spaceRotate = spaceRotateFunc;
  __glutUpdateInputDeviceMaskFunc = __glutUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
}

void APIENTRY 
glutSpaceballButtonFunc(GLUTspaceButtonCB spaceButtonFunc)
{
  __glutCurrentWindow->spaceButton = spaceButtonFunc;
  __glutUpdateInputDeviceMaskFunc = __glutUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
}
