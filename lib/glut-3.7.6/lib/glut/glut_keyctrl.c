
/* Copyright (c) Mark J. Kilgard, 1996, 1997. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glutint.h"

/* CENTRY */
void APIENTRY
glutIgnoreKeyRepeat(int ignore)
{
  __glutCurrentWindow->ignoreKeyRepeat = ignore;
}

void APIENTRY
glutSetKeyRepeat(int repeatMode)
{
#if !defined(_WIN32)
  XKeyboardControl values;

  /* GLUT's repeatMode #define's match the Xlib API values. */
  values.auto_repeat_mode = repeatMode;
  XChangeKeyboardControl(__glutDisplay, KBAutoRepeatMode, &values);
#endif
}

/* ENDCENTRY */
