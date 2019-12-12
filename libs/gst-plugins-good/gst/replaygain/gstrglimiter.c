/* GStreamer ReplayGain limiter
 *
 * Copyright (C) 2007 Rene Stadler <mail@renestadler.de>
 * 
 * gstrglimiter.c: Element to apply signal compression to raw audio data
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

/**
 * SECTION:element-rglimiter
 * @title: rglimiter
 * @see_also: #GstRgVolume
 *
 * This element applies signal compression/limiting to raw audio data.  It
 * performs strict hard limiting with soft-knee characteristics, using a
 * threshold of -6 dB.  This type of filter is mentioned in the proposed
 * [ReplayGain standard](https://wiki.hydrogenaud.io/index.php?title=ReplayGain).
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=filename.ext ! decodebin ! audioconvert \
 *            ! rgvolume pre-amp=6.0 headroom=10.0 ! rglimiter \
 *            ! audioconvert ! audioresample ! alsasink
 * ]|Playback of a file
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <math.h>
#include <gst/audio/audio.h>

#include "gstrglimiter.h"

GST_DEBUG_CATEGORY_STATIC (gst_rg_limiter_debug);
#define GST_CAT_DEFAULT gst_rg_limiter_debug

enum
{
  PROP_0,
  PROP_ENABLED,
};

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F32) ", "
        "layout = (string) { interleaved, non-interleaved }, "
        "channels = (int) [1, MAX], " "rate = (int) [1, MAX]"));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F32) ", "
        "layout = (string) { interleaved, non-interleaved}, "
        "channels = (int) [1, MAX], " "rate = (int) [1, MAX]"));

#define gst_rg_limiter_parent_class parent_class
G_DEFINE_TYPE (GstRgLimiter, gst_rg_limiter, GST_TYPE_BASE_TRANSFORM);

static void gst_rg_limiter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rg_limiter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_rg_limiter_transform_ip (GstBaseTransform * base,
    GstBuffer * buf);

static void
gst_rg_limiter_class_init (GstRgLimiterClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseTransformClass *trans_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_rg_limiter_set_property;
  gobject_class->get_property = gst_rg_limiter_get_property;

  g_object_class_install_property (gobject_class, PROP_ENABLED,
      g_param_spec_boolean ("enabled", "Enabled", "Enable processing", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  trans_class = GST_BASE_TRANSFORM_CLASS (klass);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_rg_limiter_transform_ip);
  trans_class->passthrough_on_same_caps = FALSE;

  gst_element_class_add_static_pad_template (element_class, &src_factory);
  gst_element_class_add_static_pad_template (element_class, &sink_factory);
  gst_element_class_set_static_metadata (element_class, "ReplayGain limiter",
      "Filter/Effect/Audio",
      "Apply signal compression to raw audio data",
      "Ren\xc3\xa9 Stadler <mail@renestadler.de>");

  GST_DEBUG_CATEGORY_INIT (gst_rg_limiter_debug, "rglimiter", 0,
      "ReplayGain limiter element");
}

static void
gst_rg_limiter_init (GstRgLimiter * filter)
{
  GstBaseTransform *base = GST_BASE_TRANSFORM (filter);

  gst_base_transform_set_passthrough (base, FALSE);
  gst_base_transform_set_gap_aware (base, TRUE);

  filter->enabled = TRUE;
}

static void
gst_rg_limiter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRgLimiter *filter = GST_RG_LIMITER (object);

  switch (prop_id) {
    case PROP_ENABLED:
      filter->enabled = g_value_get_boolean (value);
      gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (filter),
          !filter->enabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rg_limiter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRgLimiter *filter = GST_RG_LIMITER (object);

  switch (prop_id) {
    case PROP_ENABLED:
      g_value_set_boolean (value, filter->enabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

#define LIMIT 1.0
#define THRES 0.5               /* ca. -6 dB */
#define COMPL 0.5               /* LIMIT - THRESH */

static GstFlowReturn
gst_rg_limiter_transform_ip (GstBaseTransform * base, GstBuffer * buf)
{
  GstRgLimiter *filter = GST_RG_LIMITER (base);
  gfloat *input;
  GstMapInfo map;
  guint count;
  guint i;

  if (!filter->enabled)
    return GST_FLOW_OK;

  if (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_GAP))
    return GST_FLOW_OK;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  input = (gfloat *) map.data;
  count = gst_buffer_get_size (buf) / sizeof (gfloat);

  for (i = count; i--;) {
    if (*input > THRES)
      *input = tanhf ((*input - THRES) / COMPL) * COMPL + THRES;
    else if (*input < -THRES)
      *input = tanhf ((*input + THRES) / COMPL) * COMPL - THRES;
    input++;
  }

  gst_buffer_unmap (buf, &map);

  return GST_FLOW_OK;
}
