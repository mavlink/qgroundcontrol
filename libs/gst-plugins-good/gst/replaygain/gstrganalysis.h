/* GStreamer ReplayGain analysis
 *
 * Copyright (C) 2006 Rene Stadler <mail@renestadler.de>
 *
 * gstrganalysis.h: Element that performs the ReplayGain analysis
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

#ifndef __GST_RG_ANALYSIS_H__
#define __GST_RG_ANALYSIS_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

#include "rganalysis.h"

G_BEGIN_DECLS

#define GST_TYPE_RG_ANALYSIS \
  (gst_rg_analysis_get_type())
#define GST_RG_ANALYSIS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RG_ANALYSIS,GstRgAnalysis))
#define GST_RG_ANALYSIS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RG_ANALYSIS,GstRgAnalysisClass))
#define GST_IS_RG_ANALYSIS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RG_ANALYSIS))
#define GST_IS_RG_ANALYSIS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RG_ANALYSIS))
typedef struct _GstRgAnalysis GstRgAnalysis;
typedef struct _GstRgAnalysisClass GstRgAnalysisClass;

/**
 * GstRgAnalysis:
 *
 * Opaque data structure.
 */
struct _GstRgAnalysis
{
  GstBaseTransform element;

  /*< private >*/

  RgAnalysisCtx *ctx;
  void (*analyze) (RgAnalysisCtx * ctx, gconstpointer data, gsize size,
      guint depth);
  gint depth;

  /* Property values. */
  guint num_tracks;
  gdouble reference_level;
  gboolean forced;
  gboolean message;

  /* State machinery for skipping. */
  gboolean ignore_tags;
  gboolean skip;
  gboolean has_track_gain;
  gboolean has_track_peak;
  gboolean has_album_gain;
  gboolean has_album_peak;
};

struct _GstRgAnalysisClass
{
  GstBaseTransformClass parent_class;
};

GType gst_rg_analysis_get_type (void);

G_END_DECLS

#endif /* __GST_RG_ANALYSIS_H__ */
