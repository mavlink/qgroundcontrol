/*  monoscope.cpp
 *  Copyright (C) 2002 Richard Boulton <richard@tartarus.org>
 *  Copyright (C) 1998-2001 Andy Lo A Foe <andy@alsaplayer.org>
 *  Original code by Tinic Uro
 *
 *  This code is copied from Alsaplayer. The original code was by Tinic Uro and under
 *  the BSD license without a advertisig clause. Andy Lo A Foe then relicensed the
 *  code when he used it for Alsaplayer to GPL with Tinic's permission. Richard Boulton
 *  then took this code and made a GPL plugin out of it.
 *
 *  7th December 2004 Christian Schaller: Richard Boulton and Andy Lo A Foe gave
 *  permission to relicense their changes under BSD license so we where able to restore the
 *  code to Tinic's original BSD license.
 *
 * This file is under what is known as the BSD license:
 *
 * Redistribution and use in source and binary forms, with or without modification, i
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "monoscope.h"

#include <string.h>
#include <stdlib.h>

static void
colors_init (guint32 * colors)
{
  int i;
  int hq = (scope_height / 4);
  int hq1 = hq - 1;
  int hh1 = (scope_height / 2) - 1;
  double scl = (256.0 / (double) hq);

  for (i = 0; i < hq; i++) {
    /* green to yellow */
    colors[i] = ((int) (i * scl) << 16) + (255 << 8);
    /* yellow to red */
    colors[i + hq1] = (255 << 16) + ((int) ((hq1 - i) * scl) << 8);
  }
  colors[hh1] = (40 << 16) + (75 << 8);
}

struct monoscope_state *
monoscope_init (guint32 resx, guint32 resy)
{
  struct monoscope_state *stateptr;

  /* I didn't program monoscope to only do 256*128, but it works that way */
  g_return_val_if_fail (resx == scope_width, 0);
  g_return_val_if_fail (resy == scope_height, 0);
  stateptr = calloc (1, sizeof (struct monoscope_state));
  if (stateptr == 0)
    return 0;
  stateptr->cstate = convolve_init (convolver_depth);
  colors_init (stateptr->colors);
  return stateptr;
}

void
monoscope_close (struct monoscope_state *stateptr)
{
  convolve_close (stateptr->cstate);
  free (stateptr);
}

guint32 *
monoscope_update (struct monoscope_state *stateptr, gint16 data[convolver_big])
{
  /* Really, we want samples evenly spread over the available data.
   * Just taking a continuous chunk will do for now, though. */
  int i;
  int foo, bar;
  int avg;
  int h;
  int hh = (scope_height / 2);
  int hh1 = hh - 1;
  guint32 *loc;

  double factor;
  int max = 1;
  short *thisEq = stateptr->copyEq;

  memcpy (thisEq, data, sizeof (short) * convolver_big);
  thisEq += convolve_match (stateptr->avgEq, thisEq, stateptr->cstate);

  memset (stateptr->display, 0, scope_width * scope_height * sizeof (guint32));
  for (i = 0; i < convolver_small; i++) {
    avg = (thisEq[i] + stateptr->avgEq[i]) >> 1;
    stateptr->avgEq[i] = avg;
    avg = abs (avg);
    max = MAX (max, avg);
  }
  /* running average, 4 values is enough to make it follow volume changes
   * if this value is too large it will converge slowly
   */
  stateptr->avgMax += (max / 4) - (stateptr->avgMax / 4);

  /* input is +/- avgMax, output is +/- hh */
  if (stateptr->avgMax) {
    factor = (gdouble) hh / stateptr->avgMax;
  } else {
    factor = 1.0;
  }

  for (i = 0; i < scope_width; i++) {
    /* scale 16bit signed audio values to scope_height */
    foo = stateptr->avgEq[i] * factor;
    foo = CLAMP (foo, -hh1, hh1);
    bar = (i + ((foo + hh) * scope_width));
    if ((bar > 0) && (bar < (scope_width * scope_height))) {
      loc = stateptr->display + bar;
      /* draw up / down bars */
      if (foo < 0) {
        for (h = 0; h <= (-foo); h++) {
          *loc = stateptr->colors[h];
          loc += scope_width;
        }
      } else {
        for (h = 0; h <= foo; h++) {
          *loc = stateptr->colors[h];
          loc -= scope_width;
        }
      }
    }
  }

  /* Draw grid. */
  {
    guint32 gray = stateptr->colors[hh1];

    for (i = 16; i < scope_height; i += 16) {
      for (h = 0; h < scope_width; h += 2) {
        stateptr->display[(i * scope_width) + h] = gray;
        if (i == hh)
          stateptr->display[(i * scope_width) + h + 1] = gray;
      }
    }
    for (i = 16; i < scope_width; i += 16) {
      for (h = 0; h < scope_height; h += 2) {
        stateptr->display[i + (h * scope_width)] = gray;
      }
    }
  }
  return stateptr->display;
}
