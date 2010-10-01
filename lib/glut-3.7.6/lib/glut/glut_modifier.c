
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include "glutint.h"

/* CENTRY */
int APIENTRY
glutGetModifiers(void)
{
  int modifiers;

  if(__glutModifierMask == (unsigned int) ~0) {
    __glutWarning(
      "glutCurrentModifiers: do not call outside core input callback.");
    return 0;
  }
  modifiers = 0;
  if(__glutModifierMask & (ShiftMask|LockMask))
    modifiers |= GLUT_ACTIVE_SHIFT;
  if(__glutModifierMask & ControlMask)
    modifiers |= GLUT_ACTIVE_CTRL;
  if(__glutModifierMask & Mod1Mask)
    modifiers |= GLUT_ACTIVE_ALT;
  return modifiers;
}

/* ENDCENTRY */
