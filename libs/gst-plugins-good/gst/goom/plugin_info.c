/* Goom Project
 * Copyright (C) <2003> iOS-Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "goom_config.h"

#include "goom_plugin_info.h"
#include "goom_fx.h"
#include "drawmethods.h"
#include <math.h>
#include <stdio.h>
#ifdef HAVE_ORC
#include <orc/orc.h>
#endif


#if defined (HAVE_CPU_PPC64) || defined (HAVE_CPU_PPC)
#include "ppc_zoom_ultimate.h"
#include "ppc_drawings.h"
#endif /* HAVE_CPU_PPC64 || HAVE_CPU_PPC */


#ifdef HAVE_MMX
#include "mmx.h"
#endif /* HAVE_MMX */

#include <string.h>

GST_DEBUG_CATEGORY_EXTERN (goom_debug);
#define GST_CAT_DEFAULT goom_debug

static void
setOptimizedMethods (PluginInfo * p)
{
#ifdef HAVE_ORC
  unsigned int cpuFlavour =
      orc_target_get_default_flags (orc_target_get_by_name ("mmx"));
#else
  unsigned int cpuFlavour = 0;
#endif

  /* set default methods */
  p->methods.draw_line = draw_line;
  p->methods.zoom_filter = zoom_filter_c;
/*    p->methods.create_output_with_brightness = create_output_with_brightness;*/

  GST_INFO ("orc cpu flags: 0x%08x", cpuFlavour);

/* FIXME: what about HAVE_CPU_X86_64 ? */
#ifdef HAVE_CPU_I386
#ifdef HAVE_MMX
#ifdef HAVE_ORC
  GST_INFO ("have an x86");
  if (cpuFlavour & ORC_TARGET_MMX_MMXEXT) {
    GST_INFO ("Extended MMX detected. Using the fastest methods!");
    p->methods.draw_line = draw_line_xmmx;
    p->methods.zoom_filter = zoom_filter_xmmx;
  } else if (cpuFlavour & ORC_TARGET_MMX_MMX) {
    GST_INFO ("MMX detected. Using fast methods!");
    p->methods.draw_line = draw_line_mmx;
    p->methods.zoom_filter = zoom_filter_mmx;
  } else {
    GST_INFO ("Too bad ! No SIMD optimization available for your CPU.");
  }
#endif
#endif
#endif /* HAVE_CPU_I386 */

/* disable all PPC stuff until someone finds out what to use here instead of
 * CPU_OPTION_64_BITS, and until someone fixes the assembly build for ppc */
#if 0
#ifdef HAVE_CPU_PPC64
  if ((cpuFlavour & CPU_OPTION_64_BITS) != 0) {
/*            p->methods.create_output_with_brightness = ppc_brightness_G5;        */
    p->methods.zoom_filter = ppc_zoom_generic;
  }
#endif /* HAVE_CPU_PPC64 */

#ifdef HAVE_CPU_PPC
  if ((cpuFlavour & ORC_TARGET_ALTIVEC_ALTIVEC) != 0) {
/*            p->methods.create_output_with_brightness = ppc_brightness_G4;        */
    p->methods.zoom_filter = ppc_zoom_G4;
  } else {
/*            p->methods.create_output_with_brightness = ppc_brightness_generic;*/
    p->methods.zoom_filter = ppc_zoom_generic;
  }
#endif /* HAVE_CPU_PPC */
#endif
}

void
plugin_info_init (PluginInfo * pp, int nbVisuals)
{

  int i;

  memset (pp, 0, sizeof (PluginInfo));

  pp->sound.speedvar = pp->sound.accelvar = pp->sound.totalgoom = 0;
  pp->sound.prov_max = 0;
  pp->sound.goom_limit = 1;
  pp->sound.allTimesMax = 1;
  pp->sound.timeSinceLastGoom = 1;
  pp->sound.timeSinceLastBigGoom = 1;
  pp->sound.cycle = 0;

  secure_f_feedback (&pp->sound.volume_p, "Sound Volume");
  secure_f_feedback (&pp->sound.accel_p, "Sound Acceleration");
  secure_f_feedback (&pp->sound.speed_p, "Sound Speed");
  secure_f_feedback (&pp->sound.goom_limit_p, "Goom Limit");
  secure_f_feedback (&pp->sound.last_goom_p, "Goom Detection");
  secure_f_feedback (&pp->sound.last_biggoom_p, "Big Goom Detection");
  secure_f_feedback (&pp->sound.goom_power_p, "Goom Power");

  secure_i_param (&pp->sound.biggoom_speed_limit_p, "Big Goom Speed Limit");
  IVAL (pp->sound.biggoom_speed_limit_p) = 10;
  IMIN (pp->sound.biggoom_speed_limit_p) = 0;
  IMAX (pp->sound.biggoom_speed_limit_p) = 100;
  ISTEP (pp->sound.biggoom_speed_limit_p) = 1;

  secure_i_param (&pp->sound.biggoom_factor_p, "Big Goom Factor");
  IVAL (pp->sound.biggoom_factor_p) = 10;
  IMIN (pp->sound.biggoom_factor_p) = 0;
  IMAX (pp->sound.biggoom_factor_p) = 100;
  ISTEP (pp->sound.biggoom_factor_p) = 1;

  plugin_parameters (&pp->sound.params, "Sound", 11);

  pp->nbParams = 0;
  pp->params = NULL;
  pp->nbVisuals = nbVisuals;
  pp->visuals = (VisualFX **) malloc (sizeof (VisualFX *) * nbVisuals);

  pp->sound.params.params[0] = &pp->sound.biggoom_speed_limit_p;
  pp->sound.params.params[1] = &pp->sound.biggoom_factor_p;
  pp->sound.params.params[2] = 0;
  pp->sound.params.params[3] = &pp->sound.volume_p;
  pp->sound.params.params[4] = &pp->sound.accel_p;
  pp->sound.params.params[5] = &pp->sound.speed_p;
  pp->sound.params.params[6] = 0;
  pp->sound.params.params[7] = &pp->sound.goom_limit_p;
  pp->sound.params.params[8] = &pp->sound.goom_power_p;
  pp->sound.params.params[9] = &pp->sound.last_goom_p;
  pp->sound.params.params[10] = &pp->sound.last_biggoom_p;

  pp->statesNumber = 8;
  pp->statesRangeMax = 510;
  {
    GoomState states[8] = {
      {1, 0, 0, 1, 4, 0, 100}
      ,
      {1, 0, 0, 0, 1, 101, 140}
      ,
      {1, 0, 0, 1, 2, 141, 200}
      ,
      {0, 1, 0, 1, 2, 201, 260}
      ,
      {0, 1, 0, 1, 0, 261, 330}
      ,
      {0, 1, 1, 1, 4, 331, 400}
      ,
      {0, 0, 1, 0, 5, 401, 450}
      ,
      {0, 0, 1, 1, 1, 451, 510}
    };
    for (i = 0; i < 8; ++i)
      pp->states[i] = states[i];
  }
  pp->curGState = &(pp->states[6]);

  /* datas for the update loop */
  pp->update.lockvar = 0;
  pp->update.goomvar = 0;
  pp->update.loopvar = 0;
  pp->update.stop_lines = 0;
  pp->update.ifs_incr = 1;      /* dessiner l'ifs (0 = non: > = increment) */
  pp->update.decay_ifs = 0;     /* disparition de l'ifs */
  pp->update.recay_ifs = 0;     /* dedisparition de l'ifs */
  pp->update.cyclesSinceLastChange = 0;
  pp->update.drawLinesDuration = 80;
  pp->update.lineMode = pp->update.drawLinesDuration;

  pp->update.switchMultAmount = (29.0f / 30.0f);
  pp->update.switchIncrAmount = 0x7f;
  pp->update.switchMult = 1.0f;
  pp->update.switchIncr = pp->update.switchIncrAmount;

  pp->update.stateSelectionRnd = 0;
  pp->update.stateSelectionBlocker = 0;
  pp->update.previousZoomSpeed = 128;

  {
    ZoomFilterData zfd = {
      127, 8, 16,
      1, 1, 0, NORMAL_MODE,
      0, 0, 0, 0, 0
    };
    pp->update.zoomFilterData = zfd;
  }

  setOptimizedMethods (pp);

  for (i = 0; i < 0xffff; i++) {
    pp->sintable[i] =
        (int) (1024 * sin ((double) i * 360 / (sizeof (pp->sintable) /
                sizeof (pp->sintable[0]) - 1) * 3.141592 / 180) + .5);
    /* sintable [us] = (int)(1024.0f * sin (us*2*3.31415f/0xffff)) ; */
  }
}

void
plugin_info_add_visual (PluginInfo * p, int i, VisualFX * visual)
{
  p->visuals[i] = visual;
  if (i == p->nbVisuals - 1) {
    ++i;
    p->nbParams = 1;
    while (i--) {
      if (p->visuals[i]->params)
        p->nbParams++;
    }
    p->params =
        (PluginParameters *) malloc (sizeof (PluginParameters) * p->nbParams);
    i = p->nbVisuals;
    p->nbParams = 1;
    p->params[0] = p->sound.params;
    while (i--) {
      if (p->visuals[i]->params)
        p->params[p->nbParams++] = *(p->visuals[i]->params);
    }
  }
}

void
plugin_info_free (PluginInfo * p)
{
  goom_plugin_parameters_free (&p->sound.params);

  if (p->params)
    free (p->params);
  free (p->visuals);
}
