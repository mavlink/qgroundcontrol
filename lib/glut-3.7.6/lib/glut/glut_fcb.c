
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

/* I appreciate the guidance from William Mitchell
   (mitchell@cam.nist.gov) in developing this friend interface
   for use by the f90gl package.  See ../../README.fortran */

#include "glutint.h"

/* FCB stands for Fortran CallBack. */

/* There is only one idleFunc, menuStateFunc, and menuStatusFunc, so they
   can be saved in the wrappers for Fortran rather than the C structures. */

/* Set a Fortran callback function. */

void APIENTRY
__glutSetFCB(int which, void *func)
{
#ifdef SUPPORT_FORTRAN
  switch (which) {
  case GLUT_FCB_DISPLAY:
    __glutCurrentWindow->fdisplay = (GLUTdisplayFCB) func;
    break;
  case GLUT_FCB_RESHAPE:
    __glutCurrentWindow->freshape = (GLUTreshapeFCB) func;
    break;
  case GLUT_FCB_MOUSE:
    __glutCurrentWindow->fmouse = (GLUTmouseFCB) func;
    break;
  case GLUT_FCB_MOTION:
    __glutCurrentWindow->fmotion = (GLUTmotionFCB) func;
    break;
  case GLUT_FCB_PASSIVE:
    __glutCurrentWindow->fpassive = (GLUTpassiveFCB) func;
    break;
  case GLUT_FCB_ENTRY:
    __glutCurrentWindow->fentry = (GLUTentryFCB) func;
    break;
  case GLUT_FCB_KEYBOARD:
    __glutCurrentWindow->fkeyboard = (GLUTkeyboardFCB) func;
    break;
  case GLUT_FCB_KEYBOARD_UP:
    __glutCurrentWindow->fkeyboardUp = (GLUTkeyboardFCB) func;
    break;
  case GLUT_FCB_WINDOW_STATUS:
    __glutCurrentWindow->fwindowStatus = (GLUTwindowStatusFCB) func;
    break;
  case GLUT_FCB_VISIBILITY:
    __glutCurrentWindow->fvisibility = (GLUTvisibilityFCB) func;
    break;
  case GLUT_FCB_SPECIAL:
    __glutCurrentWindow->fspecial = (GLUTspecialFCB) func;
    break;
  case GLUT_FCB_SPECIAL_UP:
    __glutCurrentWindow->fspecialUp = (GLUTspecialFCB) func;
    break;
  case GLUT_FCB_BUTTON_BOX:
    __glutCurrentWindow->fbuttonBox = (GLUTbuttonBoxFCB) func;
    break;
  case GLUT_FCB_DIALS:
    __glutCurrentWindow->fdials = (GLUTdialsFCB) func;
    break;
  case GLUT_FCB_SPACE_MOTION:
    __glutCurrentWindow->fspaceMotion = (GLUTspaceMotionFCB) func;
    break;
  case GLUT_FCB_SPACE_ROTATE:
    __glutCurrentWindow->fspaceRotate = (GLUTspaceRotateFCB) func;
    break;
  case GLUT_FCB_SPACE_BUTTON:
    __glutCurrentWindow->fspaceButton = (GLUTspaceButtonFCB) func;
    break;
  case GLUT_FCB_TABLET_MOTION:
    __glutCurrentWindow->ftabletMotion = (GLUTtabletMotionFCB) func;
    break;
  case GLUT_FCB_TABLET_BUTTON:
    __glutCurrentWindow->ftabletButton = (GLUTtabletButtonFCB) func;
    break;
#ifdef _WIN32
  case GLUT_FCB_JOYSTICK:
    __glutCurrentWindow->fjoystick = (GLUTjoystickFCB) func;
    break;
#endif
  case GLUT_FCB_OVERLAY_DISPLAY:
    __glutCurrentWindow->overlay->fdisplay = (GLUTdisplayFCB) func;
    break;
  case GLUT_FCB_SELECT:
    __glutCurrentMenu->fselect = (GLUTselectFCB) func;
    break;
  case GLUT_FCB_TIMER:
    __glutNewTimer->ffunc = (GLUTtimerFCB) func;
    break;
  }
#endif
}

/* Get a Fortran callback function. */

void* APIENTRY
__glutGetFCB(int which)
{
#ifdef SUPPORT_FORTRAN
  switch (which) {
  case GLUT_FCB_DISPLAY:
    return (void *) __glutCurrentWindow->fdisplay;
  case GLUT_FCB_RESHAPE:
    return (void *) __glutCurrentWindow->freshape;
  case GLUT_FCB_MOUSE:
    return (void *) __glutCurrentWindow->fmouse;
  case GLUT_FCB_MOTION:
    return (void *) __glutCurrentWindow->fmotion;
  case GLUT_FCB_PASSIVE:
    return (void *) __glutCurrentWindow->fpassive;
  case GLUT_FCB_ENTRY:
    return (void *) __glutCurrentWindow->fentry;
  case GLUT_FCB_KEYBOARD:
    return (void *) __glutCurrentWindow->fkeyboard;
  case GLUT_FCB_KEYBOARD_UP:
    return (void *) __glutCurrentWindow->fkeyboardUp;
  case GLUT_FCB_WINDOW_STATUS:
    return (void *) __glutCurrentWindow->fwindowStatus;
  case GLUT_FCB_VISIBILITY:
    return (void *) __glutCurrentWindow->fvisibility;
  case GLUT_FCB_SPECIAL:
    return (void *) __glutCurrentWindow->fspecial;
  case GLUT_FCB_SPECIAL_UP:
    return (void *) __glutCurrentWindow->fspecialUp;
  case GLUT_FCB_BUTTON_BOX:
    return (void *) __glutCurrentWindow->fbuttonBox;
  case GLUT_FCB_DIALS:
    return (void *) __glutCurrentWindow->fdials;
  case GLUT_FCB_SPACE_MOTION:
    return (void *) __glutCurrentWindow->fspaceMotion;
  case GLUT_FCB_SPACE_ROTATE:
    return (void *) __glutCurrentWindow->fspaceRotate;
  case GLUT_FCB_SPACE_BUTTON:
    return (void *) __glutCurrentWindow->fspaceButton;
  case GLUT_FCB_TABLET_MOTION:
    return (void *) __glutCurrentWindow->ftabletMotion;
  case GLUT_FCB_TABLET_BUTTON:
    return (void *) __glutCurrentWindow->ftabletButton;
  case GLUT_FCB_JOYSTICK:
#ifdef _WIN32
    return (void *) __glutCurrentWindow->fjoystick;
#else
    return NULL;
#endif
  case GLUT_FCB_OVERLAY_DISPLAY:
    return (void *) __glutCurrentWindow->overlay->fdisplay;
  case GLUT_FCB_SELECT:
    return (void *) __glutCurrentMenu->fselect;
  case GLUT_FCB_TIMER:
    return (void *) __glutTimerList->ffunc;
  default:
    return NULL;
  }
#else
  return NULL;
#endif
}
