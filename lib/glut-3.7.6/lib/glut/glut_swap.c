
/* Copyright (c) Mark J. Kilgard, 1994, 1997.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glutint.h"

/* CENTRY */
void APIENTRY
glutSwapBuffers(void)
{
  GLUTwindow *window = __glutCurrentWindow;

  if (window->renderWin == window->win) {
    if (__glutCurrentWindow->treatAsSingle) {
      /* Pretend the double buffered window is single buffered,
         so treat glutSwapBuffers as a no-op. */
      return;
    }
  } else {
    if (__glutCurrentWindow->overlay->treatAsSingle) {
      /* Pretend the double buffered overlay is single
         buffered, so treat glutSwapBuffers as a no-op. */
      return;
    }
  }

  /* For the MESA_SWAP_HACK. */
  window->usedSwapBuffers = 1;

  SWAP_BUFFERS_LAYER(__glutCurrentWindow);

  /* I considered putting the window being swapped on the
     GLUT_FINISH_WORK work list because you could call
     glutSwapBuffers from an idle callback which doesn't call
     __glutSetWindow which normally adds indirect rendering
     windows to the GLUT_FINISH_WORK work list.  Not being put
     on the list could lead to the buffering up of multiple
     redisplays and buffer swaps and hamper interactivity.  I
     consider this an application bug due to not using
     glutPostRedisplay to trigger redraws.  If
     glutPostRedisplay were used, __glutSetWindow would be
     called and a glFinish to throttle buffering would occur. */
}
/* ENDCENTRY */
