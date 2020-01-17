/* GStreamer
 * Copyright (C) <2008> Wim Taymans <wim.taymans@gmail.com>
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

/**
 * SECTION:element-smptealpha
 * @title: smptealpha
 *
 * smptealpha can accept an I420 or AYUV video stream. An alpha channel is added
 * using an effect specific SMPTE mask in the I420 input case. In the AYUV case,
 * the alpha channel is modified using the effect specific SMPTE mask.
 *
 * The #GstSMPTEAlpha:position property is a controllabe double between 0.0 and
 * 1.0 that specifies the position in the transition. 0.0 is the start of the
 * transition with the alpha channel to complete opaque where 1.0 has the alpha
 * channel set to completely transparent.
 *
 * The #GstSMPTEAlpha:depth property defines the precision in bits of the mask.
 * A higher presision will create a mask with smoother gradients in order to
 * avoid banding.
 *
 * ## Sample pipelines
 *
 * Here is a pipeline to demonstrate the smpte transition :
 * |[
 * gst-launch-1.0 -v videotestsrc ! smptealpha border=20000 type=44
 * position=0.5 ! videomixer ! videoconvert ! ximagesink
 * ]|
 * This shows a midway bowtie-h transition a from a videotestsrc to a
 * transparent image. The edges of the transition are smoothed with a
 * 20000 big border.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "gstsmptealpha.h"
#include "paint.h"

GST_DEBUG_CATEGORY_STATIC (gst_smpte_alpha_debug);
#define GST_CAT_DEFAULT gst_smpte_alpha_debug

static GstStaticPadTemplate gst_smpte_alpha_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("AYUV") ";"
        GST_VIDEO_CAPS_MAKE ("ARGB") ";" GST_VIDEO_CAPS_MAKE ("BGRA") ";"
        GST_VIDEO_CAPS_MAKE ("RGBA") ";" GST_VIDEO_CAPS_MAKE ("ARGB"))
    );

static GstStaticPadTemplate gst_smpte_alpha_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("I420") ";"
        GST_VIDEO_CAPS_MAKE ("YV12")
        ";" GST_VIDEO_CAPS_MAKE ("AYUV")
        ";" GST_VIDEO_CAPS_MAKE ("ARGB") ";" GST_VIDEO_CAPS_MAKE ("BGRA")
        ";" GST_VIDEO_CAPS_MAKE ("RGBA") ";" GST_VIDEO_CAPS_MAKE ("ARGB"))
    );

/* SMPTE signals and properties */

#define DEFAULT_PROP_TYPE	1
#define DEFAULT_PROP_BORDER	0
#define DEFAULT_PROP_DEPTH	16
#define DEFAULT_PROP_POSITION	0.0
#define DEFAULT_PROP_INVERT   FALSE

enum
{
  PROP_0,
  PROP_TYPE,
  PROP_BORDER,
  PROP_DEPTH,
  PROP_POSITION,
  PROP_INVERT
};

#define AYUV_SIZE(w,h)     ((w) * (h) * 4)

#define GST_TYPE_SMPTE_TRANSITION_TYPE (gst_smpte_alpha_transition_type_get_type())
static GType
gst_smpte_alpha_transition_type_get_type (void)
{
  static GType smpte_transition_type = 0;
  GEnumValue *smpte_transitions;

  if (!smpte_transition_type) {
    const GList *definitions;
    gint i = 0;

    definitions = gst_mask_get_definitions ();
    smpte_transitions =
        g_new0 (GEnumValue, g_list_length ((GList *) definitions) + 1);

    while (definitions) {
      GstMaskDefinition *definition = (GstMaskDefinition *) definitions->data;

      definitions = g_list_next (definitions);

      smpte_transitions[i].value = definition->type;
      /* older GLib versions have the two fields as non-const, hence the cast */
      smpte_transitions[i].value_nick = (gchar *) definition->short_name;
      smpte_transitions[i].value_name = (gchar *) definition->long_name;

      i++;
    }

    smpte_transition_type =
        g_enum_register_static ("GstSMPTEAlphaTransitionType",
        smpte_transitions);
  }
  return smpte_transition_type;
}


static void gst_smpte_alpha_finalize (GstSMPTEAlpha * smpte);

static void gst_smpte_alpha_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_smpte_alpha_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_smpte_alpha_set_info (GstVideoFilter * vfilter,
    GstCaps * incaps, GstVideoInfo * in_info,
    GstCaps * outcaps, GstVideoInfo * out_info);
static GstFlowReturn gst_smpte_alpha_transform_frame (GstVideoFilter * vfilter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame);
static void gst_smpte_alpha_before_transform (GstBaseTransform * trans,
    GstBuffer * buf);
static GstCaps *gst_smpte_alpha_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * from, GstCaps * filter);

#define gst_smpte_alpha_parent_class parent_class
G_DEFINE_TYPE (GstSMPTEAlpha, gst_smpte_alpha, GST_TYPE_VIDEO_FILTER);

static void
gst_smpte_alpha_class_init (GstSMPTEAlphaClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) (klass);
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) klass;

  gobject_class->set_property = gst_smpte_alpha_set_property;
  gobject_class->get_property = gst_smpte_alpha_get_property;

  gobject_class->finalize = (GObjectFinalizeFunc) gst_smpte_alpha_finalize;

  _gst_mask_init ();

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TYPE,
      g_param_spec_enum ("type", "Type", "The type of transition to use",
          GST_TYPE_SMPTE_TRANSITION_TYPE, DEFAULT_PROP_TYPE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BORDER,
      g_param_spec_int ("border", "Border",
          "The border width of the transition", 0, G_MAXINT,
          DEFAULT_PROP_BORDER,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DEPTH,
      g_param_spec_int ("depth", "Depth", "Depth of the mask in bits", 1, 24,
          DEFAULT_PROP_DEPTH, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_POSITION,
      g_param_spec_double ("position", "Position",
          "Position of the transition effect", 0.0, 1.0, DEFAULT_PROP_POSITION,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstSMPTEAlpha:invert:
   *
   * Set to TRUE to invert the transition mask (ie. flip it horizontally).
   */
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_INVERT,
      g_param_spec_boolean ("invert", "Invert",
          "Invert transition mask", DEFAULT_PROP_POSITION,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  trans_class->before_transform =
      GST_DEBUG_FUNCPTR (gst_smpte_alpha_before_transform);
  trans_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_smpte_alpha_transform_caps);

  vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_smpte_alpha_set_info);
  vfilter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_smpte_alpha_transform_frame);

  gst_element_class_add_static_pad_template (element_class,
      &gst_smpte_alpha_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_smpte_alpha_src_template);
  gst_element_class_set_static_metadata (element_class, "SMPTE transitions",
      "Filter/Editor/Video",
      "Apply the standard SMPTE transitions as alpha on video images",
      "Wim Taymans <wim.taymans@gmail.com>");
}

static gboolean
gst_smpte_alpha_update_mask (GstSMPTEAlpha * smpte, gint type,
    gboolean invert, gint depth, gint width, gint height)
{
  GstMask *newmask;

  /* try to avoid regenerating the mask if we already have one that is
   * correct */
  if (smpte->mask) {
    if (smpte->type == type &&
        smpte->invert == invert &&
        smpte->depth == depth &&
        smpte->width == width && smpte->height == height)
      return TRUE;
  }

  smpte->type = type;
  smpte->invert = invert;
  smpte->depth = depth;
  smpte->width = width;
  smpte->height = height;

  /* Not negotiated yet */
  if (width == 0 || height == 0) {
    return TRUE;
  }

  newmask = gst_mask_factory_new (type, invert, depth, width, height);
  if (!newmask)
    goto mask_failed;

  if (smpte->mask)
    gst_mask_destroy (smpte->mask);

  smpte->mask = newmask;

  return TRUE;

  /* ERRORS */
mask_failed:
  {
    GST_ERROR_OBJECT (smpte, "failed to create a mask");
    return FALSE;
  }
}

static void
gst_smpte_alpha_init (GstSMPTEAlpha * smpte)
{
  smpte->type = DEFAULT_PROP_TYPE;
  smpte->border = DEFAULT_PROP_BORDER;
  smpte->depth = DEFAULT_PROP_DEPTH;
  smpte->position = DEFAULT_PROP_POSITION;
  smpte->invert = DEFAULT_PROP_INVERT;
}

#define CREATE_ARGB_FUNC(name, A, R, G, B) \
static void \
gst_smpte_alpha_process_##name##_##name (GstSMPTEAlpha * smpte, \
    const GstVideoFrame * in_frame, GstVideoFrame * out_frame, GstMask * mask, \
    gint border, gint pos) \
{ \
  gint i, j; \
  const guint32 *maskp; \
  gint value; \
  gint min, max; \
  gint width, height; \
  guint8 *in, *out; \
  gint src_wrap, dest_wrap; \
  \
  if (border == 0) \
    border++; \
  \
  min = pos - border; \
  max = pos; \
  GST_DEBUG_OBJECT (smpte, "pos %d, min %d, max %d, border %d", pos, min, max, \
      border); \
  \
  maskp = mask->data; \
  \
  width = GST_VIDEO_FRAME_WIDTH (out_frame); \
  height = GST_VIDEO_FRAME_HEIGHT (out_frame); \
  \
  in = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0); \
  out = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0); \
  src_wrap = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0) - (width << 2); \
  dest_wrap = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0) - (width << 2); \
  \
  /* we basically copy the source to dest but we scale the alpha channel with \
   * the mask */ \
  for (i = 0; i < height; i++) { \
    for (j = 0; j < width; j++) { \
      value = *maskp++; \
      out[A] = (in[A] * ((CLAMP (value, min, max) - min) << 8) / border) >> 8; \
      out[R] = in[R]; \
      out[G] = in[G]; \
      out[B] = in[B]; \
      out += 4; \
      in += 4; \
    } \
    in += src_wrap; \
    out += dest_wrap; \
  } \
}

CREATE_ARGB_FUNC (argb, 0, 1, 2, 3);
CREATE_ARGB_FUNC (bgra, 3, 2, 1, 0);
CREATE_ARGB_FUNC (abgr, 0, 3, 2, 1);
CREATE_ARGB_FUNC (rgba, 3, 0, 1, 2);

static void
gst_smpte_alpha_process_ayuv_ayuv (GstSMPTEAlpha * smpte,
    const GstVideoFrame * in_frame, GstVideoFrame * out_frame, GstMask * mask,
    gint border, gint pos)
{
  gint i, j;
  const guint32 *maskp;
  gint value;
  gint min, max;
  gint width, height;
  guint8 *in, *out;
  gint src_wrap, dest_wrap;

  if (border == 0)
    border++;

  min = pos - border;
  max = pos;
  GST_DEBUG_OBJECT (smpte, "pos %d, min %d, max %d, border %d", pos, min, max,
      border);

  maskp = mask->data;

  width = GST_VIDEO_FRAME_WIDTH (out_frame);
  height = GST_VIDEO_FRAME_HEIGHT (out_frame);

  in = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
  out = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);
  src_wrap = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0) - (width << 2);
  dest_wrap = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0) - (width << 2);

  /* we basically copy the source to dest but we scale the alpha channel with
   * the mask */
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      value = *maskp++;
      *out++ = (*in++ * ((CLAMP (value, min, max) - min) << 8) / border) >> 8;
      *out++ = *in++;
      *out++ = *in++;
      *out++ = *in++;
    }
    in += src_wrap;
    out += dest_wrap;
  }
}

static void
gst_smpte_alpha_process_i420_ayuv (GstSMPTEAlpha * smpte,
    const GstVideoFrame * in_frame, GstVideoFrame * out_frame, GstMask * mask,
    gint border, gint pos)
{
  const guint8 *srcY;
  const guint8 *srcU;
  const guint8 *srcV;
  guint8 *out;
  gint i, j;
  gint src_wrap, src_u_wrap, src_v_wrap, dest_wrap;
  gint y_stride, u_stride, v_stride;
  gboolean odd_width;
  const guint32 *maskp;
  gint value;
  gint min, max;
  gint width, height;

  if (border == 0)
    border++;

  min = pos - border;
  max = pos;
  GST_DEBUG_OBJECT (smpte, "pos %d, min %d, max %d, border %d", pos, min, max,
      border);

  maskp = mask->data;

  width = GST_VIDEO_FRAME_WIDTH (out_frame);
  height = GST_VIDEO_FRAME_HEIGHT (out_frame);

  y_stride = GST_VIDEO_FRAME_COMP_STRIDE (in_frame, 0);
  u_stride = GST_VIDEO_FRAME_COMP_STRIDE (in_frame, 1);
  v_stride = GST_VIDEO_FRAME_COMP_STRIDE (in_frame, 2);

  src_wrap = y_stride - width;
  src_u_wrap = u_stride - (width / 2);
  src_v_wrap = v_stride - (width / 2);

  srcY = GST_VIDEO_FRAME_COMP_DATA (in_frame, 0);
  srcU = GST_VIDEO_FRAME_COMP_DATA (in_frame, 1);
  srcV = GST_VIDEO_FRAME_COMP_DATA (in_frame, 2);

  out = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);
  dest_wrap = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0) - (width << 2);

  odd_width = (width % 2 != 0);

  for (i = 0; i < height; i++) {
    for (j = 0; j < width / 2; j++) {
      value = *maskp++;
      *out++ = (0xff * ((CLAMP (value, min, max) - min) << 8) / border) >> 8;
      *out++ = *srcY++;
      *out++ = *srcU;
      *out++ = *srcV;
      value = *maskp++;
      *out++ = (0xff * ((CLAMP (value, min, max) - min) << 8) / border) >> 8;
      *out++ = *srcY++;
      *out++ = *srcU++;
      *out++ = *srcV++;
    }
    /* Might have one odd column left to do */
    if (odd_width) {
      value = *maskp++;
      *out++ = (0xff * ((CLAMP (value, min, max) - min) << 8) / border) >> 8;
      *out++ = *srcY++;
      *out++ = *srcU;
      *out++ = *srcV;
    }
    if (i % 2 == 0) {
      srcU -= width / 2;
      srcV -= width / 2;
    } else {
      srcU += src_u_wrap;
      srcV += src_v_wrap;
    }
    srcY += src_wrap;
    out += dest_wrap;
  }
}

static void
gst_smpte_alpha_before_transform (GstBaseTransform * trans, GstBuffer * buf)
{
  GstSMPTEAlpha *smpte = GST_SMPTE_ALPHA (trans);
  GstClockTime timestamp, stream_time;

  /* first sync the controller to the current stream_time of the buffer */
  timestamp = GST_BUFFER_TIMESTAMP (buf);
  stream_time =
      gst_segment_to_stream_time (&trans->segment, GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (smpte, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (smpte), stream_time);
}

static GstFlowReturn
gst_smpte_alpha_transform_frame (GstVideoFilter * vfilter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
  GstSMPTEAlpha *smpte = GST_SMPTE_ALPHA (vfilter);
  gdouble position;
  gint border;

  if (G_UNLIKELY (!smpte->process))
    goto not_negotiated;

  GST_OBJECT_LOCK (smpte);
  position = smpte->position;
  border = smpte->border;

  /* run the type specific filter code */
  smpte->process (smpte, in_frame, out_frame,
      smpte->mask, border, ((1 << smpte->depth) + border) * position);
  GST_OBJECT_UNLOCK (smpte);

  return GST_FLOW_OK;

  /* ERRORS */
not_negotiated:
  {
    GST_ELEMENT_ERROR (smpte, CORE, NEGOTIATION, (NULL),
        ("No input format negotiated"));
    return GST_FLOW_NOT_NEGOTIATED;
  }
}

static GstCaps *
gst_smpte_alpha_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * from, GstCaps * filter)
{
  GstCaps *result, *tmp_caps, *tmpl_caps = NULL;
  gint i, j;

  tmp_caps = gst_caps_new_empty ();

  for (i = 0; i < gst_caps_get_size (from); i++) {
    GstStructure *structure;
    const GValue *val, *lval;
    GValue list = { 0, };
    GValue aval = { 0, };
    const gchar *str;

    structure = gst_structure_copy (gst_caps_get_structure (from, i));
    /* we can transform I420 to AYUV,
     * so need to locate and substitute AYUV for the both of them */
    val = gst_structure_get_value (structure, "format");
    if (val && GST_VALUE_HOLDS_LIST (val)) {
      gboolean seen_ayuv = FALSE, seen_i420 = FALSE;

      g_value_init (&list, GST_TYPE_LIST);
      for (j = 0; j < gst_value_list_get_size (val); j++) {
        lval = gst_value_list_get_value (val, j);
        if ((str = g_value_get_string (lval))) {
          if (strcmp (str, "AYUV") == 0) {
            seen_ayuv = TRUE;
          } else if (strcmp (str, "I420") == 0) {
            seen_i420 = TRUE;
          }
        }
      }
      if (seen_ayuv && !seen_i420) {
        str = "I420";
      } else if (seen_i420 && !seen_ayuv) {
        str = "AYUV";
      } else
        str = NULL;
      if (str) {
        g_value_copy (val, &list);
        g_value_init (&aval, G_TYPE_STRING);
        g_value_set_string (&aval, str);
        gst_value_list_append_value (&list, &aval);
        g_value_reset (&aval);
        gst_structure_set_value (structure, "format", &list);
        g_value_unset (&list);
      }
    } else if (val && G_VALUE_HOLDS_STRING (val)) {
      if ((str = g_value_get_string (val)) &&
          ((strcmp (str, "AYUV") == 0) || (strcmp (str, "I420") == 0))) {
        g_value_init (&list, GST_TYPE_LIST);
        g_value_init (&aval, G_TYPE_STRING);
        g_value_set_string (&aval, "AYUV");
        gst_value_list_append_value (&list, &aval);
        g_value_reset (&aval);
        g_value_set_string (&aval, "I420");
        gst_value_list_append_value (&list, &aval);
        g_value_reset (&aval);
        gst_structure_set_value (structure, "format", &list);
        g_value_unset (&list);
      }
    } else {
      gst_structure_remove_field (structure, "format");
    }

    gst_structure_remove_field (structure, "colorimetry");
    gst_structure_remove_field (structure, "chroma-site");

    gst_caps_append_structure (tmp_caps, structure);
  }

  /* Get the appropriate template */
  if (direction == GST_PAD_SINK) {
    tmpl_caps =
        gst_static_pad_template_get_caps (&gst_smpte_alpha_src_template);
  } else if (direction == GST_PAD_SRC) {
    tmpl_caps =
        gst_static_pad_template_get_caps (&gst_smpte_alpha_sink_template);
  } else {
    g_assert_not_reached ();
  }

  /* Intersect with our template caps */
  result = gst_caps_intersect (tmp_caps, tmpl_caps);
  gst_caps_unref (tmpl_caps);
  gst_caps_unref (tmp_caps);

  result = gst_caps_simplify (result);

  GST_LOG_OBJECT (trans, "transformed %" GST_PTR_FORMAT " to %" GST_PTR_FORMAT,
      from, result);

  if (filter) {
    GstCaps *intersection;

    GST_DEBUG_OBJECT (trans, "Using filter caps %" GST_PTR_FORMAT, filter);
    intersection =
        gst_caps_intersect_full (filter, result, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (result);
    result = intersection;
    GST_DEBUG_OBJECT (trans, "Intersection %" GST_PTR_FORMAT, result);
  }

  return result;
}

static gboolean
gst_smpte_alpha_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstSMPTEAlpha *smpte = GST_SMPTE_ALPHA (vfilter);
  gboolean ret;

  smpte->process = NULL;
  smpte->in_format = GST_VIDEO_INFO_FORMAT (in_info);
  smpte->out_format = GST_VIDEO_INFO_FORMAT (out_info);

  /* try to update the mask now, this will also adjust the width/height on
   * success */
  GST_OBJECT_LOCK (smpte);
  ret =
      gst_smpte_alpha_update_mask (smpte, smpte->type, smpte->invert,
      smpte->depth, GST_VIDEO_INFO_WIDTH (out_info),
      GST_VIDEO_INFO_HEIGHT (out_info));
  GST_OBJECT_UNLOCK (smpte);

  if (!ret)
    goto mask_failed;

  switch (smpte->out_format) {
    case GST_VIDEO_FORMAT_AYUV:
      switch (smpte->in_format) {
        case GST_VIDEO_FORMAT_AYUV:
          smpte->process = gst_smpte_alpha_process_ayuv_ayuv;
          break;
        case GST_VIDEO_FORMAT_I420:
          smpte->process = gst_smpte_alpha_process_i420_ayuv;
          break;
        default:
          break;
      }
      break;
    case GST_VIDEO_FORMAT_ARGB:
      switch (smpte->in_format) {
        case GST_VIDEO_FORMAT_ARGB:
          smpte->process = gst_smpte_alpha_process_argb_argb;
          break;
        default:
          break;
      }
      break;
    case GST_VIDEO_FORMAT_RGBA:
      switch (smpte->in_format) {
        case GST_VIDEO_FORMAT_RGBA:
          smpte->process = gst_smpte_alpha_process_rgba_rgba;
          break;
        default:
          break;
      }
      break;
    case GST_VIDEO_FORMAT_ABGR:
      switch (smpte->in_format) {
        case GST_VIDEO_FORMAT_ABGR:
          smpte->process = gst_smpte_alpha_process_abgr_abgr;
          break;
        default:
          break;
      }
      break;
    case GST_VIDEO_FORMAT_BGRA:
      switch (smpte->in_format) {
        case GST_VIDEO_FORMAT_BGRA:
          smpte->process = gst_smpte_alpha_process_bgra_bgra;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  return ret;

  /* ERRORS */
mask_failed:
  {
    GST_ERROR_OBJECT (smpte, "failed creating the mask");
    return FALSE;
  }
}

static void
gst_smpte_alpha_finalize (GstSMPTEAlpha * smpte)
{
  if (smpte->mask)
    gst_mask_destroy (smpte->mask);
  smpte->mask = NULL;

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) smpte);
}

static void
gst_smpte_alpha_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSMPTEAlpha *smpte = GST_SMPTE_ALPHA (object);

  switch (prop_id) {
    case PROP_TYPE:{
      gint type;

      type = g_value_get_enum (value);

      GST_OBJECT_LOCK (smpte);
      gst_smpte_alpha_update_mask (smpte, type, smpte->invert,
          smpte->depth, smpte->width, smpte->height);
      GST_OBJECT_UNLOCK (smpte);
      break;
    }
    case PROP_BORDER:
      GST_OBJECT_LOCK (smpte);
      smpte->border = g_value_get_int (value);
      GST_OBJECT_UNLOCK (smpte);
      break;
    case PROP_DEPTH:{
      gint depth;

      depth = g_value_get_int (value);

      GST_OBJECT_LOCK (smpte);
      gst_smpte_alpha_update_mask (smpte, smpte->type, smpte->invert,
          depth, smpte->width, smpte->height);
      GST_OBJECT_UNLOCK (smpte);
      break;
    }
    case PROP_POSITION:
      GST_OBJECT_LOCK (smpte);
      smpte->position = g_value_get_double (value);
      GST_OBJECT_UNLOCK (smpte);
      break;
    case PROP_INVERT:{
      gboolean invert;

      invert = g_value_get_boolean (value);
      GST_OBJECT_LOCK (smpte);
      gst_smpte_alpha_update_mask (smpte, smpte->type, invert,
          smpte->depth, smpte->width, smpte->height);
      GST_OBJECT_UNLOCK (smpte);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_smpte_alpha_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSMPTEAlpha *smpte;

  smpte = GST_SMPTE_ALPHA (object);

  switch (prop_id) {
    case PROP_TYPE:
      GST_OBJECT_LOCK (smpte);
      g_value_set_enum (value, smpte->type);
      GST_OBJECT_UNLOCK (smpte);
      break;
    case PROP_BORDER:
      GST_OBJECT_LOCK (smpte);
      g_value_set_int (value, smpte->border);
      GST_OBJECT_UNLOCK (smpte);
      break;
    case PROP_DEPTH:
      GST_OBJECT_LOCK (smpte);
      g_value_set_int (value, smpte->depth);
      GST_OBJECT_UNLOCK (smpte);
      break;
    case PROP_POSITION:
      GST_OBJECT_LOCK (smpte);
      g_value_set_double (value, smpte->position);
      GST_OBJECT_UNLOCK (smpte);
      break;
    case PROP_INVERT:
      GST_OBJECT_LOCK (smpte);
      g_value_set_boolean (value, smpte->invert);
      GST_OBJECT_UNLOCK (smpte);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_smpte_alpha_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_smpte_alpha_debug, "smptealpha", 0,
      "SMPTE alpha effect");

  return gst_element_register (plugin, "smptealpha", GST_RANK_NONE,
      GST_TYPE_SMPTE_ALPHA);
}
