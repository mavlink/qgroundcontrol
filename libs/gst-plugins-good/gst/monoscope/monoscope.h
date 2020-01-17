#ifndef _MONOSCOPE_H
#define _MONOSCOPE_H

#include <glib.h>
#include "convolve.h"

#define convolver_depth 8
#define convolver_small (1 << convolver_depth)
#define convolver_big (2 << convolver_depth)
#define scope_width 256
#define scope_height 128

struct monoscope_state {
  short copyEq[convolver_big];
  int avgEq[convolver_small];      /* a running average of the last few. */
  int avgMax;                     /* running average of max sample. */
  guint32 display[scope_width * scope_height];

  convolve_state *cstate;
  guint32 colors[scope_height / 2];
};

struct monoscope_state * monoscope_init (guint32 resx, guint32 resy);
guint32 * monoscope_update (struct monoscope_state * stateptr, gint16 data [convolver_big]);
void monoscope_close (struct monoscope_state * stateptr);

#endif
