/* GStreamer ReplayGain analysis
 *
 * Copyright (C) 2006 Rene Stadler <mail@renestadler.de>
 * Copyright (C) 2001 David Robinson <David@Robinson.org>
 *                    Glen Sawyer <glensawyer@hotmail.com>
 *
 * rganalysis.h: Analyze raw audio data in accordance with ReplayGain
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __RG_ANALYSIS_H__
#define __RG_ANALYSIS_H__

#include <glib.h>
#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _RgAnalysisCtx RgAnalysisCtx;

RgAnalysisCtx *rg_analysis_new (void);
gboolean rg_analysis_set_sample_rate (RgAnalysisCtx * ctx, gint sample_rate);
void rg_analysis_analyze_mono_float (RgAnalysisCtx * ctx, gconstpointer data,
    gsize size, guint depth);
void rg_analysis_analyze_stereo_float (RgAnalysisCtx * ctx, gconstpointer data,
    gsize size, guint depth);
void rg_analysis_analyze_mono_int16 (RgAnalysisCtx * ctx, gconstpointer data,
    gsize size, guint depth);
void rg_analysis_analyze_stereo_int16 (RgAnalysisCtx * ctx, gconstpointer data,
    gsize size, guint depth);
void rg_analysis_analyze (RgAnalysisCtx * ctx, const gfloat * samples_l,
    const gfloat * samples_r, guint n_samples);
gboolean rg_analysis_track_result (RgAnalysisCtx * ctx, gdouble * gain,
    gdouble * peak);
gboolean rg_analysis_album_result (RgAnalysisCtx * ctx, gdouble * gain,
    gdouble * peak);
void rg_analysis_init_silence_detection (
    RgAnalysisCtx * ctx,
    void (*post_message) (gpointer analysis, GstClockTime timestamp, GstClockTime duration, gdouble rglevel),
    gpointer analysis);
void rg_analysis_start_buffer (RgAnalysisCtx * ctx,
                               GstClockTime buffer_timestamp);
void rg_analysis_reset_album (RgAnalysisCtx * ctx);
void rg_analysis_reset (RgAnalysisCtx * ctx);
void rg_analysis_destroy (RgAnalysisCtx * ctx);

G_END_DECLS

#endif /* __RG_ANALYSIS_H__ */
