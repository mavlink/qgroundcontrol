/* GStreamer
 * Cradioacyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * EffecTV - Realtime Digital Video Effector
 * Cradioacyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * RadioacTV - motion-enlightment effect.
 * Cradioacyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * EffecTV is free software. This library is free software;
 * you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your radioaction) any later version.
 *
 * This library is distributed in the hradioace that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a cradioacy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:element-radioactv
 * @title: radioactv
 *
 * RadioacTV does *NOT* detect a radioactivity. It detects a difference
 * from previous frame and blurs it.
 *
 * RadioacTV has 4 mode, normal, strobe1, strobe2 and trigger.
 * In trigger mode, effect appears only when the trigger property is %TRUE.
 *
 * strobe1 and strobe2 mode drops some frames. strobe1 mode uses the difference between
 * current frame and previous frame dropped, while strobe2 mode uses the difference from
 * previous frame displayed. The effect of strobe2 is stronger than strobe1.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v videotestsrc ! radioactv ! videoconvert ! autovideosink
 * ]| This pipeline shows the effect of radioactv on a test stream.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "gstradioac.h"
#include "gsteffectv.h"

enum
{
  RADIOAC_NORMAL = 0,
  RADIOAC_STROBE,
  RADIOAC_STROBE2,
  RADIOAC_TRIGGER
};

enum
{
  COLOR_RED = 0,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_WHITE
};

#define GST_TYPE_RADIOACTV_MODE (gst_radioactv_mode_get_type())
static GType
gst_radioactv_mode_get_type (void)
{
  static GType type = 0;

  static const GEnumValue enumvalue[] = {
    {RADIOAC_NORMAL, "Normal", "normal"},
    {RADIOAC_STROBE, "Strobe 1", "strobe1"},
    {RADIOAC_STROBE2, "Strobe 2", "strobe2"},
    {RADIOAC_TRIGGER, "Trigger", "trigger"},
    {0, NULL, NULL},
  };

  if (!type) {
    type = g_enum_register_static ("GstRadioacTVMode", enumvalue);
  }
  return type;
}

#define GST_TYPE_RADIOACTV_COLOR (gst_radioactv_color_get_type())
static GType
gst_radioactv_color_get_type (void)
{
  static GType type = 0;

  static const GEnumValue enumvalue[] = {
    {COLOR_RED, "Red", "red"},
    {COLOR_GREEN, "Green", "green"},
    {COLOR_BLUE, "Blue", "blue"},
    {COLOR_WHITE, "White", "white"},
    {0, NULL, NULL},
  };

  if (!type) {
    type = g_enum_register_static ("GstRadioacTVColor", enumvalue);
  }
  return type;
}

#define DEFAULT_MODE RADIOAC_NORMAL
#define DEFAULT_COLOR COLOR_WHITE
#define DEFAULT_INTERVAL 3
#define DEFAULT_TRIGGER FALSE

enum
{
  PROP_0,
  PROP_MODE,
  PROP_COLOR,
  PROP_INTERVAL,
  PROP_TRIGGER
};

#define COLORS 32
#define PATTERN 4
#define MAGIC_THRESHOLD 40
#define RATIO 0.95

static guint32 palettes[COLORS * PATTERN];
static const gint swap_tab[] = { 2, 1, 0, 3 };

#define gst_radioactv_parent_class parent_class
G_DEFINE_TYPE (GstRadioacTV, gst_radioactv, GST_TYPE_VIDEO_FILTER);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAPS_STR GST_VIDEO_CAPS_MAKE ("{ RGBx, BGRx }")
#else
#define CAPS_STR GST_VIDEO_CAPS_MAKE ("{ xBGR, xRGB }")
#endif

static GstStaticPadTemplate gst_radioactv_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static GstStaticPadTemplate gst_radioactv_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static void
makePalette (void)
{
  gint i;

#define DELTA (255/(COLORS/2-1))

  /* red, gree, blue */
  for (i = 0; i < COLORS / 2; i++) {
    palettes[i] = i * DELTA;
    palettes[COLORS + i] = (i * DELTA) << 8;
    palettes[COLORS * 2 + i] = (i * DELTA) << 16;
  }
  for (i = 0; i < COLORS / 2; i++) {
    palettes[i + COLORS / 2] = 255 | (i * DELTA) << 16 | (i * DELTA) << 8;
    palettes[COLORS + i + COLORS / 2] =
        (255 << 8) | (i * DELTA) << 16 | i * DELTA;
    palettes[COLORS * 2 + i + COLORS / 2] =
        (255 << 16) | (i * DELTA) << 8 | i * DELTA;
  }
  /* white */
  for (i = 0; i < COLORS; i++) {
    palettes[COLORS * 3 + i] = (255 * i / COLORS) * 0x10101;
  }
  for (i = 0; i < COLORS * PATTERN; i++) {
    palettes[i] = palettes[i] & 0xfefeff;
  }
#undef DELTA
}

#define VIDEO_HWIDTH (filter->buf_width/2)
#define VIDEO_HHEIGHT (filter->buf_height/2)

/* this table assumes that video_width is times of 32 */
static void
setTable (GstRadioacTV * filter)
{
  guint bits;
  gint x, y, tx, ty, xx;
  gint ptr, prevptr;

  prevptr = (gint) (0.5 + RATIO * (-VIDEO_HWIDTH) + VIDEO_HWIDTH);
  for (xx = 0; xx < (filter->buf_width_blocks); xx++) {
    bits = 0;
    for (x = 0; x < 32; x++) {
      ptr = (gint) (0.5 + RATIO * (xx * 32 + x - VIDEO_HWIDTH) + VIDEO_HWIDTH);
      bits = bits >> 1;
      if (ptr != prevptr)
        bits |= 0x80000000;
      prevptr = ptr;
    }
    filter->blurzoomx[xx] = bits;
  }

  ty = (gint) (0.5 + RATIO * (-VIDEO_HHEIGHT) + VIDEO_HHEIGHT);
  tx = (gint) (0.5 + RATIO * (-VIDEO_HWIDTH) + VIDEO_HWIDTH);
  xx = (gint) (0.5 + RATIO * (filter->buf_width - 1 - VIDEO_HWIDTH) +
      VIDEO_HWIDTH);
  filter->blurzoomy[0] = ty * filter->buf_width + tx;
  prevptr = ty * filter->buf_width + xx;
  for (y = 1; y < filter->buf_height; y++) {
    ty = (gint) (0.5 + RATIO * (y - VIDEO_HHEIGHT) + VIDEO_HHEIGHT);
    filter->blurzoomy[y] = ty * filter->buf_width + tx - prevptr;
    prevptr = ty * filter->buf_width + xx;
  }
}

#undef VIDEO_HWIDTH
#undef VIDEO_HHEIGHT

static void
blur (GstRadioacTV * filter)
{
  gint x, y;
  gint width;
  guint8 *p, *q;
  guint8 v;
  GstVideoInfo *info;

  info = &GST_VIDEO_FILTER (filter)->in_info;

  width = filter->buf_width;
  p = filter->blurzoombuf + GST_VIDEO_INFO_WIDTH (info) + 1;
  q = p + filter->buf_area;

  for (y = filter->buf_height - 2; y > 0; y--) {
    for (x = width - 2; x > 0; x--) {
      v = (*(p - width) + *(p - 1) + *(p + 1) + *(p + width)) / 4 - 1;
      if (v == 255)
        v = 0;
      *q = v;
      p++;
      q++;
    }
    p += 2;
    q += 2;
  }
}

static void
zoom (GstRadioacTV * filter)
{
  gint b, x, y;
  guint8 *p, *q;
  gint blocks, height;
  gint dx;

  p = filter->blurzoombuf + filter->buf_area;
  q = filter->blurzoombuf;
  height = filter->buf_height;
  blocks = filter->buf_width_blocks;

  for (y = 0; y < height; y++) {
    p += filter->blurzoomy[y];
    for (b = 0; b < blocks; b++) {
      dx = filter->blurzoomx[b];
      for (x = 0; x < 32; x++) {
        p += (dx & 1);
        *q++ = *p;
        dx = dx >> 1;
      }
    }
  }
}

static void
blurzoomcore (GstRadioacTV * filter)
{
  blur (filter);
  zoom (filter);
}

/* Background image is refreshed every frame */
static void
image_bgsubtract_update_y (guint32 * src, gint16 * background, guint8 * diff,
    gint video_area, gint y_threshold)
{
  gint i;
  gint R, G, B;
  guint32 *p;
  gint16 *q;
  guint8 *r;
  gint v;

  p = src;
  q = background;
  r = diff;
  for (i = 0; i < video_area; i++) {
    R = ((*p) & 0xff0000) >> (16 - 1);
    G = ((*p) & 0xff00) >> (8 - 2);
    B = (*p) & 0xff;
    v = (R + G + B) - (gint) (*q);
    *q = (gint16) (R + G + B);
    *r = ((v + y_threshold) >> 24) | ((y_threshold - v) >> 24);

    p++;
    q++;
    r++;
  }
}

static GstFlowReturn
gst_radioactv_transform_frame (GstVideoFilter * vfilter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
  GstRadioacTV *filter = GST_RADIOACTV (vfilter);
  guint32 *src, *dest;
  GstClockTime timestamp, stream_time;
  gint x, y, width, height;
  guint32 a, b;
  guint8 *diff, *p;
  guint32 *palette;

  timestamp = GST_BUFFER_TIMESTAMP (in_frame->buffer);
  stream_time =
      gst_segment_to_stream_time (&GST_BASE_TRANSFORM (filter)->segment,
      GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (filter, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (filter), stream_time);

  src = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  dest = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

  width = GST_VIDEO_FRAME_WIDTH (in_frame);
  height = GST_VIDEO_FRAME_HEIGHT (in_frame);

  GST_OBJECT_LOCK (filter);
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  if (GST_VIDEO_FRAME_FORMAT (in_frame) == GST_VIDEO_FORMAT_RGBx) {
    palette = &palettes[COLORS * filter->color];
  } else {
    palette = &palettes[COLORS * swap_tab[filter->color]];
  }
#else
  if (GST_VIDEO_FRAME_FORMAT (in_frame) == GST_VIDEO_FORMAT_xBGR) {
    palette = &palettes[COLORS * filter->color];
  } else {
    palette = &palettes[COLORS * swap_tab[filter->color]];
  }
#endif
  diff = filter->diff;

  if (filter->mode == 3 && filter->trigger)
    filter->snaptime = 0;
  else if (filter->mode == 3 && !filter->trigger)
    filter->snaptime = 1;

  if (filter->mode != 2 || filter->snaptime <= 0) {
    image_bgsubtract_update_y (src, filter->background, diff,
        width * height, MAGIC_THRESHOLD * 7);
    if (filter->mode == 0 || filter->snaptime <= 0) {
      diff += filter->buf_margin_left;
      p = filter->blurzoombuf;
      for (y = 0; y < filter->buf_height; y++) {
        for (x = 0; x < filter->buf_width; x++) {
          p[x] |= diff[x] >> 3;
        }
        diff += width;
        p += filter->buf_width;
      }
      if (filter->mode == 1 || filter->mode == 2) {
        memcpy (filter->snapframe, src, width * height * 4);
      }
    }
  }
  blurzoomcore (filter);

  if (filter->mode == 1 || filter->mode == 2) {
    src = filter->snapframe;
  }
  p = filter->blurzoombuf;
  for (y = 0; y < height; y++) {
    for (x = 0; x < filter->buf_margin_left; x++) {
      *dest++ = *src++;
    }
    for (x = 0; x < filter->buf_width; x++) {
      a = *src++ & 0xfefeff;
      b = palette[*p++];
      a += b;
      b = a & 0x1010100;
      *dest++ = a | (b - (b >> 8));
    }
    for (x = 0; x < filter->buf_margin_right; x++) {
      *dest++ = *src++;
    }
  }

  if (filter->mode == 1 || filter->mode == 2) {
    filter->snaptime--;
    if (filter->snaptime < 0) {
      filter->snaptime = filter->interval;
    }
  }
  GST_OBJECT_UNLOCK (filter);

  return GST_FLOW_OK;
}

static gboolean
gst_radioactv_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstRadioacTV *filter = GST_RADIOACTV (vfilter);
  gint width, height;

  width = GST_VIDEO_INFO_WIDTH (in_info);
  height = GST_VIDEO_INFO_HEIGHT (in_info);

  filter->buf_width_blocks = width / 32;
  if (filter->buf_width_blocks > 255)
    goto too_wide;

  filter->buf_width = filter->buf_width_blocks * 32;
  filter->buf_height = height;
  filter->buf_area = filter->buf_height * filter->buf_width;
  filter->buf_margin_left = (width - filter->buf_width) / 2;
  filter->buf_margin_right =
      height - filter->buf_width - filter->buf_margin_left;

  g_free (filter->blurzoombuf);
  filter->blurzoombuf = g_new0 (guint8, filter->buf_area * 2);

  g_free (filter->blurzoomx);
  filter->blurzoomx = g_new0 (gint, filter->buf_width);

  g_free (filter->blurzoomy);
  filter->blurzoomy = g_new0 (gint, filter->buf_height);

  g_free (filter->snapframe);
  filter->snapframe = g_new (guint32, width * height);

  g_free (filter->diff);
  filter->diff = g_new (guint8, width * height);

  g_free (filter->background);
  filter->background = g_new0 (gint16, width * height);

  setTable (filter);

  return TRUE;

  /* ERRORS */
too_wide:
  {
    GST_DEBUG_OBJECT (filter, "frame too wide");
    return FALSE;
  }
}

static gboolean
gst_radioactv_start (GstBaseTransform * trans)
{
  GstRadioacTV *filter = GST_RADIOACTV (trans);

  filter->snaptime = 0;

  return TRUE;
}

static void
gst_radioactv_finalize (GObject * object)
{
  GstRadioacTV *filter = GST_RADIOACTV (object);

  g_free (filter->snapframe);
  filter->snapframe = NULL;

  g_free (filter->blurzoombuf);
  filter->blurzoombuf = NULL;

  g_free (filter->diff);
  filter->diff = NULL;

  g_free (filter->background);
  filter->background = NULL;

  g_free (filter->blurzoomx);
  filter->blurzoomx = NULL;

  g_free (filter->blurzoomy);
  filter->blurzoomy = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_radioactv_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRadioacTV *filter = GST_RADIOACTV (object);

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_MODE:
      filter->mode = g_value_get_enum (value);
      if (filter->mode == 3)
        filter->snaptime = 1;
      break;
    case PROP_COLOR:
      filter->color = g_value_get_enum (value);
      break;
    case PROP_INTERVAL:
      filter->interval = g_value_get_uint (value);
      break;
    case PROP_TRIGGER:
      filter->trigger = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static void
gst_radioactv_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRadioacTV *filter = GST_RADIOACTV (object);

  switch (prop_id) {
    case PROP_MODE:
      g_value_set_enum (value, filter->mode);
      break;
    case PROP_COLOR:
      g_value_set_enum (value, filter->color);
      break;
    case PROP_INTERVAL:
      g_value_set_uint (value, filter->interval);
      break;
    case PROP_TRIGGER:
      g_value_set_boolean (value, filter->trigger);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_radioactv_class_init (GstRadioacTVClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_radioactv_set_property;
  gobject_class->get_property = gst_radioactv_get_property;

  gobject_class->finalize = gst_radioactv_finalize;

  g_object_class_install_property (gobject_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode",
          "Mode", GST_TYPE_RADIOACTV_MODE, DEFAULT_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_COLOR,
      g_param_spec_enum ("color", "Color",
          "Color", GST_TYPE_RADIOACTV_COLOR, DEFAULT_COLOR,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INTERVAL,
      g_param_spec_uint ("interval", "Interval",
          "Snapshot interval (in strobe mode)", 0, G_MAXINT, DEFAULT_INTERVAL,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TRIGGER,
      g_param_spec_boolean ("trigger", "Trigger",
          "Trigger (in trigger mode)", DEFAULT_TRIGGER,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class, "RadioacTV effect",
      "Filter/Effect/Video",
      "motion-enlightment effect",
      "FUKUCHI, Kentarou <fukuchi@users.sourceforge.net>, "
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_radioactv_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_radioactv_src_template);

  trans_class->start = GST_DEBUG_FUNCPTR (gst_radioactv_start);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_radioactv_set_info);
  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_radioactv_transform_frame);

  makePalette ();
}

static void
gst_radioactv_init (GstRadioacTV * filter)
{
  filter->mode = DEFAULT_MODE;
  filter->color = DEFAULT_COLOR;
  filter->interval = DEFAULT_INTERVAL;
  filter->trigger = DEFAULT_TRIGGER;
}
