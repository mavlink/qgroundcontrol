/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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
 * SECTION:element-smpte
 * @title: smpte
 *
 * smpte can accept I420 video streams with the same width, height and
 * framerate. The two incoming buffers are blended together using an effect
 * specific alpha mask. 
 *
 * The #GstSMPTE:depth property defines the presision in bits of the mask. A
 * higher presision will create a mask with smoother gradients in order to avoid
 * banding.
 *
 * ## Sample pipelines
 * |[
 * gst-launch-1.0 -v videotestsrc pattern=1 ! smpte name=s border=20000 type=234 duration=2000000000 ! videoconvert ! ximagesink videotestsrc ! s.
 * ]| A pipeline to demonstrate the smpte transition.
 * It shows a pinwheel transition a from a snow videotestsrc to an smpte
 * pattern videotestsrc. The transition will take 2 seconds to complete. The
 * edges of the transition are smoothed with a 20000 big border.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include "gstsmpte.h"
#include "paint.h"

GST_DEBUG_CATEGORY_STATIC (gst_smpte_debug);
#define GST_CAT_DEFAULT gst_smpte_debug

static GstStaticPadTemplate gst_smpte_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("I420")
    )
    );

static GstStaticPadTemplate gst_smpte_sink1_template =
GST_STATIC_PAD_TEMPLATE ("sink1",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("I420")
    )
    );

static GstStaticPadTemplate gst_smpte_sink2_template =
GST_STATIC_PAD_TEMPLATE ("sink2",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("I420")
    )
    );


/* SMPTE signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

#define DEFAULT_PROP_TYPE	1
#define DEFAULT_PROP_BORDER	0
#define DEFAULT_PROP_DEPTH	16
#define DEFAULT_PROP_DURATION	GST_SECOND
#define DEFAULT_PROP_INVERT   FALSE

enum
{
  PROP_0,
  PROP_TYPE,
  PROP_BORDER,
  PROP_DEPTH,
  PROP_DURATION,
  PROP_INVERT
};

#define GST_TYPE_SMPTE_TRANSITION_TYPE (gst_smpte_transition_type_get_type())
static GType
gst_smpte_transition_type_get_type (void)
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
        g_enum_register_static ("GstSMPTETransitionType", smpte_transitions);
  }
  return smpte_transition_type;
}


static void gst_smpte_finalize (GstSMPTE * smpte);

static GstFlowReturn gst_smpte_collected (GstCollectPads * pads,
    GstSMPTE * smpte);

static void gst_smpte_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_smpte_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn gst_smpte_change_state (GstElement * element,
    GstStateChange transition);

/*static guint gst_smpte_signals[LAST_SIGNAL] = { 0 }; */

#define gst_smpte_parent_class parent_class
G_DEFINE_TYPE (GstSMPTE, gst_smpte, GST_TYPE_ELEMENT);

static void
gst_smpte_class_init (GstSMPTEClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = gst_smpte_set_property;
  gobject_class->get_property = gst_smpte_get_property;
  gobject_class->finalize = (GObjectFinalizeFunc) gst_smpte_finalize;

  _gst_mask_init ();

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TYPE,
      g_param_spec_enum ("type", "Type", "The type of transition to use",
          GST_TYPE_SMPTE_TRANSITION_TYPE, DEFAULT_PROP_TYPE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BORDER,
      g_param_spec_int ("border", "Border",
          "The border width of the transition", 0, G_MAXINT,
          DEFAULT_PROP_BORDER, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DEPTH,
      g_param_spec_int ("depth", "Depth", "Depth of the mask in bits", 1, 24,
          DEFAULT_PROP_DEPTH, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DURATION,
      g_param_spec_uint64 ("duration", "Duration",
          "Duration of the transition effect in nanoseconds", 0, G_MAXUINT64,
          DEFAULT_PROP_DURATION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_INVERT,
      g_param_spec_boolean ("invert", "Invert",
          "Invert transition mask", DEFAULT_PROP_INVERT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_smpte_change_state);

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_smpte_sink1_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_smpte_sink2_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_smpte_src_template);
  gst_element_class_set_static_metadata (gstelement_class, "SMPTE transitions",
      "Filter/Editor/Video",
      "Apply the standard SMPTE transitions on video images",
      "Wim Taymans <wim.taymans@chello.be>");
}

/*                              wht  yel  cya  grn  mag  red  blu  blk   -I    Q */
static const int y_colors[] = { 255, 226, 179, 150, 105, 76, 29, 16, 16, 0 };
static const int u_colors[] = { 128, 0, 170, 46, 212, 85, 255, 128, 0, 128 };
static const int v_colors[] = { 128, 155, 0, 21, 235, 255, 107, 128, 128, 255 };

static void
fill_i420 (GstVideoInfo * vinfo, guint8 * data, gint height, gint color)
{
  gint size = GST_VIDEO_INFO_COMP_STRIDE (vinfo, 0) * GST_ROUND_UP_2 (height);
  gint size4 = size >> 2;
  guint8 *yp = data;
  guint8 *up = data + GST_VIDEO_INFO_COMP_OFFSET (vinfo, 1);
  guint8 *vp = data + GST_VIDEO_INFO_COMP_OFFSET (vinfo, 2);

  memset (yp, y_colors[color], size);
  memset (up, u_colors[color], size4);
  memset (vp, v_colors[color], size4);
}

static gboolean
gst_smpte_update_mask (GstSMPTE * smpte, gint type, gboolean invert,
    gint depth, gint width, gint height)
{
  GstMask *newmask;

  if (smpte->mask) {
    if (smpte->type == type &&
        smpte->invert == invert &&
        smpte->depth == depth &&
        smpte->width == width && smpte->height == height)
      return TRUE;
  }

  newmask = gst_mask_factory_new (type, invert, depth, width, height);
  if (newmask) {
    if (smpte->mask) {
      gst_mask_destroy (smpte->mask);
    }
    smpte->mask = newmask;
    smpte->type = type;
    smpte->invert = invert;
    smpte->depth = depth;
    smpte->width = width;
    smpte->height = height;

    return TRUE;
  }
  return FALSE;
}

static gboolean
gst_smpte_setcaps (GstPad * pad, GstCaps * caps)
{
  GstSMPTE *smpte;
  gboolean ret;
  GstVideoInfo vinfo;

  smpte = GST_SMPTE (GST_PAD_PARENT (pad));

  gst_video_info_init (&vinfo);
  if (!gst_video_info_from_caps (&vinfo, caps))
    return FALSE;

  smpte->width = GST_VIDEO_INFO_WIDTH (&vinfo);
  smpte->height = GST_VIDEO_INFO_HEIGHT (&vinfo);
  smpte->fps_num = GST_VIDEO_INFO_FPS_N (&vinfo);
  smpte->fps_denom = GST_VIDEO_INFO_FPS_D (&vinfo);

  /* figure out the duration in frames */
  smpte->end_position = gst_util_uint64_scale (smpte->duration,
      smpte->fps_num, GST_SECOND * smpte->fps_denom);

  GST_DEBUG_OBJECT (smpte, "duration: %d frames", smpte->end_position);

  ret =
      gst_smpte_update_mask (smpte, smpte->type, smpte->invert, smpte->depth,
      smpte->width, smpte->height);

  if (pad == smpte->sinkpad1) {
    GST_DEBUG_OBJECT (smpte, "setting pad1 info");
    smpte->vinfo1 = vinfo;
  } else {
    GST_DEBUG_OBJECT (smpte, "setting pad2 info");
    smpte->vinfo2 = vinfo;
  }

  return ret;
}

static gboolean
gst_smpte_sink_event (GstCollectPads * pads,
    GstCollectData * data, GstEvent * event, gpointer user_data)
{
  GstPad *pad;
  gboolean ret = FALSE;

  pad = data->pad;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      ret = gst_smpte_setcaps (pad, caps);
      gst_event_unref (event);
      event = NULL;
      break;
    }
    default:
      break;
  }

  if (event != NULL)
    return gst_collect_pads_event_default (pads, data, event, FALSE);

  return ret;
}

static void
gst_smpte_init (GstSMPTE * smpte)
{
  smpte->sinkpad1 =
      gst_pad_new_from_static_template (&gst_smpte_sink1_template, "sink1");
  GST_PAD_SET_PROXY_CAPS (smpte->sinkpad1);
  gst_element_add_pad (GST_ELEMENT (smpte), smpte->sinkpad1);

  smpte->sinkpad2 =
      gst_pad_new_from_static_template (&gst_smpte_sink2_template, "sink2");
  GST_PAD_SET_PROXY_CAPS (smpte->sinkpad2);
  gst_element_add_pad (GST_ELEMENT (smpte), smpte->sinkpad2);

  smpte->srcpad =
      gst_pad_new_from_static_template (&gst_smpte_src_template, "src");
  gst_element_add_pad (GST_ELEMENT (smpte), smpte->srcpad);

  smpte->collect = gst_collect_pads_new ();
  gst_collect_pads_set_function (smpte->collect,
      (GstCollectPadsFunction) GST_DEBUG_FUNCPTR (gst_smpte_collected), smpte);
  gst_collect_pads_set_event_function (smpte->collect,
      GST_DEBUG_FUNCPTR (gst_smpte_sink_event), smpte);

  gst_collect_pads_add_pad (smpte->collect, smpte->sinkpad1,
      sizeof (GstCollectData), NULL, TRUE);
  gst_collect_pads_add_pad (smpte->collect, smpte->sinkpad2,
      sizeof (GstCollectData), NULL, TRUE);

  smpte->type = DEFAULT_PROP_TYPE;
  smpte->border = DEFAULT_PROP_BORDER;
  smpte->depth = DEFAULT_PROP_DEPTH;
  smpte->duration = DEFAULT_PROP_DURATION;
  smpte->invert = DEFAULT_PROP_INVERT;
  smpte->fps_num = 0;
  smpte->fps_denom = 1;
}

static void
gst_smpte_finalize (GstSMPTE * smpte)
{
  if (smpte->collect) {
    gst_object_unref (smpte->collect);
  }
  if (smpte->mask) {
    gst_mask_destroy (smpte->mask);
  }

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) smpte);
}

static void
gst_smpte_reset (GstSMPTE * smpte)
{
  smpte->width = -1;
  smpte->height = -1;
  smpte->position = 0;
  smpte->end_position = 0;
  smpte->send_stream_start = TRUE;
}

static void
gst_smpte_blend_i420 (GstVideoFrame * frame1, GstVideoFrame * frame2,
    GstVideoFrame * oframe, GstMask * mask, gint border, gint pos)
{
  guint32 *maskp;
  gint value;
  gint i, j;
  gint min, max;
  guint8 *in1, *in2, *out, *in1u, *in1v, *in2u, *in2v, *outu, *outv;
  gint width, height;

  if (border == 0)
    border++;

  min = pos - border;
  max = pos;

  width = GST_VIDEO_FRAME_WIDTH (frame1);
  height = GST_VIDEO_FRAME_HEIGHT (frame1);

  in1 = GST_VIDEO_FRAME_COMP_DATA (frame1, 0);
  in2 = GST_VIDEO_FRAME_COMP_DATA (frame2, 0);
  out = GST_VIDEO_FRAME_COMP_DATA (oframe, 0);

  in1u = GST_VIDEO_FRAME_COMP_DATA (frame1, 1);
  in1v = GST_VIDEO_FRAME_COMP_DATA (frame1, 2);
  in2u = GST_VIDEO_FRAME_COMP_DATA (frame2, 1);
  in2v = GST_VIDEO_FRAME_COMP_DATA (frame2, 2);
  outu = GST_VIDEO_FRAME_COMP_DATA (oframe, 1);
  outv = GST_VIDEO_FRAME_COMP_DATA (oframe, 2);

  maskp = mask->data;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      value = *maskp++;
      value = ((CLAMP (value, min, max) - min) << 8) / border;

      out[j] = ((in1[j] * value) + (in2[j] * (256 - value))) >> 8;
      if (!(i & 1) && !(j & 1)) {
        outu[j / 2] =
            ((in1u[j / 2] * value) + (in2u[j / 2] * (256 - value))) >> 8;
        outv[j / 2] =
            ((in1v[j / 2] * value) + (in2v[j / 2] * (256 - value))) >> 8;
      }
    }

    in1 += GST_VIDEO_FRAME_COMP_STRIDE (frame1, 0);
    in2 += GST_VIDEO_FRAME_COMP_STRIDE (frame2, 0);
    out += GST_VIDEO_FRAME_COMP_STRIDE (oframe, 0);

    if (!(i & 1)) {
      in1u += GST_VIDEO_FRAME_COMP_STRIDE (frame1, 1);
      in2u += GST_VIDEO_FRAME_COMP_STRIDE (frame2, 1);
      in1v += GST_VIDEO_FRAME_COMP_STRIDE (frame1, 2);
      in2v += GST_VIDEO_FRAME_COMP_STRIDE (frame1, 2);
      outu += GST_VIDEO_FRAME_COMP_STRIDE (oframe, 1);
      outv += GST_VIDEO_FRAME_COMP_STRIDE (oframe, 2);
    }
  }
}

static GstFlowReturn
gst_smpte_collected (GstCollectPads * pads, GstSMPTE * smpte)
{
  GstBuffer *outbuf;
  GstClockTime ts;
  GstBuffer *in1 = NULL, *in2 = NULL;
  GSList *collected;
  GstMapInfo map;
  GstVideoFrame frame1, frame2, oframe;

  if (G_UNLIKELY (smpte->fps_num == 0))
    goto not_negotiated;

  if (!gst_pad_has_current_caps (smpte->sinkpad1) ||
      !gst_pad_has_current_caps (smpte->sinkpad2))
    goto not_negotiated;

  if (!gst_video_info_is_equal (&smpte->vinfo1, &smpte->vinfo2))
    goto input_formats_do_not_match;

  if (smpte->send_stream_start) {
    gchar s_id[32];

    /* stream-start (FIXME: create id based on input ids) */
    g_snprintf (s_id, sizeof (s_id), "smpte-%08x", g_random_int ());
    gst_pad_push_event (smpte->srcpad, gst_event_new_stream_start (s_id));
    smpte->send_stream_start = FALSE;
  }

  ts = gst_util_uint64_scale_int (smpte->position * GST_SECOND,
      smpte->fps_denom, smpte->fps_num);

  for (collected = pads->data; collected; collected = g_slist_next (collected)) {
    GstCollectData *data;

    data = (GstCollectData *) collected->data;

    if (data->pad == smpte->sinkpad1)
      in1 = gst_collect_pads_pop (pads, data);
    else if (data->pad == smpte->sinkpad2)
      in2 = gst_collect_pads_pop (pads, data);
  }

  if (in1 == NULL) {
    /* if no input, make picture black */
    in1 = gst_buffer_new_and_alloc (GST_VIDEO_INFO_SIZE (&smpte->vinfo1));
    gst_buffer_map (in1, &map, GST_MAP_WRITE);
    fill_i420 (&smpte->vinfo1, map.data, smpte->height, 7);
    gst_buffer_unmap (in1, &map);
  }
  if (in2 == NULL) {
    /* if no input, make picture white */
    in2 = gst_buffer_new_and_alloc (GST_VIDEO_INFO_SIZE (&smpte->vinfo2));
    gst_buffer_map (in2, &map, GST_MAP_WRITE);
    fill_i420 (&smpte->vinfo2, map.data, smpte->height, 0);
    gst_buffer_unmap (in2, &map);
  }

  if (smpte->position < smpte->end_position) {
    outbuf = gst_buffer_new_and_alloc (GST_VIDEO_INFO_SIZE (&smpte->vinfo1));

    /* set caps if not done yet */
    if (!gst_pad_has_current_caps (smpte->srcpad)) {
      GstCaps *caps;
      GstSegment segment;

      caps = gst_video_info_to_caps (&smpte->vinfo1);

      gst_pad_set_caps (smpte->srcpad, caps);
      gst_caps_unref (caps);

      gst_segment_init (&segment, GST_FORMAT_TIME);
      gst_pad_push_event (smpte->srcpad, gst_event_new_segment (&segment));
    }

    gst_video_frame_map (&frame1, &smpte->vinfo1, in1, GST_MAP_READ);
    gst_video_frame_map (&frame2, &smpte->vinfo2, in2, GST_MAP_READ);
    /* re-use either info, now know they are essentially identical */
    gst_video_frame_map (&oframe, &smpte->vinfo1, outbuf, GST_MAP_WRITE);
    gst_smpte_blend_i420 (&frame1, &frame2, &oframe, smpte->mask, smpte->border,
        ((1 << smpte->depth) + smpte->border) *
        smpte->position / smpte->end_position);
    gst_video_frame_unmap (&frame1);
    gst_video_frame_unmap (&frame2);
    gst_video_frame_unmap (&oframe);
  } else {
    outbuf = in2;
    gst_buffer_ref (in2);
  }

  smpte->position++;

  if (in1)
    gst_buffer_unref (in1);
  if (in2)
    gst_buffer_unref (in2);

  GST_BUFFER_TIMESTAMP (outbuf) = ts;

  return gst_pad_push (smpte->srcpad, outbuf);

  /* ERRORS */
not_negotiated:
  {
    GST_ELEMENT_ERROR (smpte, CORE, NEGOTIATION, (NULL),
        ("No input format negotiated"));
    return GST_FLOW_NOT_NEGOTIATED;
  }
input_formats_do_not_match:
  {
    GstCaps *caps1, *caps2;

    caps1 = gst_pad_get_current_caps (smpte->sinkpad1);
    caps2 = gst_pad_get_current_caps (smpte->sinkpad2);
    GST_ELEMENT_ERROR (smpte, CORE, NEGOTIATION, (NULL),
        ("input formats don't match: %" GST_PTR_FORMAT " vs. %" GST_PTR_FORMAT,
            caps1, caps2));
    if (caps1)
      gst_caps_unref (caps1);
    if (caps2)
      gst_caps_unref (caps2);
    return GST_FLOW_ERROR;
  }
}

static void
gst_smpte_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSMPTE *smpte;

  smpte = GST_SMPTE (object);

  switch (prop_id) {
    case PROP_TYPE:
      smpte->type = g_value_get_enum (value);
      break;
    case PROP_BORDER:
      smpte->border = g_value_get_int (value);
      break;
    case PROP_DEPTH:
      smpte->depth = g_value_get_int (value);
      break;
    case PROP_DURATION:
      smpte->duration = g_value_get_uint64 (value);
      break;
    case PROP_INVERT:
      smpte->invert = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_smpte_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSMPTE *smpte;

  smpte = GST_SMPTE (object);

  switch (prop_id) {
    case PROP_TYPE:
      g_value_set_enum (value, smpte->type);
      break;
    case PROP_BORDER:
      g_value_set_int (value, smpte->border);
      break;
    case PROP_DEPTH:
      g_value_set_int (value, smpte->depth);
      break;
    case PROP_DURATION:
      g_value_set_uint64 (value, smpte->duration);
      break;
    case PROP_INVERT:
      g_value_set_boolean (value, smpte->invert);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_smpte_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstSMPTE *smpte;

  smpte = GST_SMPTE (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_smpte_reset (smpte);
      GST_LOG_OBJECT (smpte, "starting collectpads");
      gst_collect_pads_start (smpte->collect);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_LOG_OBJECT (smpte, "stopping collectpads");
      gst_collect_pads_stop (smpte->collect);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_smpte_reset (smpte);
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_smpte_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_smpte_debug, "smpte", 0,
      "SMPTE transition effect");

  return gst_element_register (plugin, "smpte", GST_RANK_NONE, GST_TYPE_SMPTE);
}
