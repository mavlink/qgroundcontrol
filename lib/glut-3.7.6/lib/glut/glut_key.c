
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>

#include "glutint.h"

/* CENTRY */
void APIENTRY
glutKeyboardFunc(GLUTkeyboardCB keyboardFunc)
{
  __glutChangeWindowEventMask(KeyPressMask,
    keyboardFunc != NULL || __glutCurrentWindow->special != NULL);
  __glutCurrentWindow->keyboard = keyboardFunc;
}

void APIENTRY
glutSpecialFunc(GLUTspecialCB specialFunc)
{
  __glutChangeWindowEventMask(KeyPressMask,
    specialFunc != NULL || __glutCurrentWindow->keyboard != NULL);
  __glutCurrentWindow->special = specialFunc;
}

/* ENDCENTRY */
